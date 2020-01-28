#include "zoo/FundamentalOperationTracing.h"

#include "zoo/FunctionPolicy.h"

#include "catch2/catch.hpp"

namespace zoo {

static_assert(impl::MayBeCalled<double(char *, int), int(*)(void *, int)>::value);
static_assert(
    !impl::MayBeCalled<char *(int), void (*)(int)>::value,
    "A void * is not convertible to char *"
);
static_assert(impl::MayBeCalled<void *(), char *(*)()>::value);

}

TEST_CASE("New zoo function") {
    auto doubler = [&](int a) { return 2.0 * a; };
    SECTION("VTable executor") {
        zoo::NewCopyableFunction<2, double(int)> cf = doubler;
        static_assert(std::is_copy_constructible_v<zoo::NewCopyableFunction<2, double(int)>>);
        // notice: a move-only callable reference is bound to a copyable one,
        // without having to make a new object!
        zoo::NewZooFunction<2, double(int)> &di = cf;
        static_assert(!std::is_copy_constructible_v<zoo::NewZooFunction<2, double(int)>>);
        REQUIRE(6.0 == di(3));
        // this also works, a move-only has all it needs from a copyable
        di = std::move(cf);
        REQUIRE(8.0 == di(4));
        // the following will *not* compile, as intented:
        // A copyable container does not know how to copy a move-only thing
        // so it can't generate what it needs for the affordance of copyability
        //cf = std::move(di);
    }
    SECTION("Use instance-affordance Function") {
        using MOP = zoo::Policy<void *, zoo::Destroy, zoo::Move>;
        using MOAC = zoo::AnyContainer<MOP>;
        using F = zoo::Function<MOAC, int(double)>;
        static_assert(std::is_nothrow_move_constructible_v<F>);
        static_assert(!std::is_copy_constructible_v<F>);
        using CopyableFunctionPolicy = zoo::DerivedVTablePolicy<F, zoo::Copy>;
        using CF = zoo::AnyContainer<CopyableFunctionPolicy>;
        static_assert(std::is_nothrow_move_constructible_v<CF>);
        static_assert(std::is_copy_constructible_v<CF>);

        SECTION("Executor is instance member Function") {
            F f = doubler;
            CF aCopy = doubler;
            REQUIRE(14.0 == aCopy(7));
            F &reference = aCopy;
            REQUIRE(12.0 == reference(6));
            SECTION("Swapping") {
                int trace1 = 2, trace2 = 3;
                auto callable1 = [tm = zoo::TracesMoves(&trace1)](int arg) {
                    return *tm.resource_ * arg * 2.0;
                };
                auto callable2 = [tm = zoo::TracesMoves(&trace2)](int arg) {
                    return *tm.resource_ * arg * 3.0;
                };
                REQUIRE(4 == callable1(1));
                REQUIRE(9 == callable2(1));
                { // the following block proves only moves are used in swap
                    F f1(std::move(callable1)), f2(std::move(callable2));
                    REQUIRE(4 == f1(1));
                    REQUIRE(9 == f2(1));
                    swap(f1, f2);
                    CHECK(9 == f1(1));
                    CHECK(4 == f2(1));
                }
                REQUIRE(0 == trace1);
                REQUIRE(0 == trace2);
                SECTION("Copiable swap") {
                    CF empty;
                    CF doubles(doubler);
                    REQUIRE_THROWS_AS(empty(33), zoo::bad_function_call);
                    REQUIRE(6.0 == doubles(3));
                }
            }
        }
        SECTION("Second nesting") {
            using RTTI_CF_P = zoo::DerivedVTablePolicy<CF, zoo::RTTI>;
            using RCF = zoo::AnyContainer<RTTI_CF_P>;
            using namespace std;
            static_assert(is_base_of_v<CF, RCF>);
            static_assert(is_same_v<CF, typename RTTI_CF_P::Base>);
            static_assert(is_constructible_v<RCF, decltype(doubler)>);
            static_assert(zoo::detail::AffordsCopying<RTTI_CF_P>::value);
            RCF withRTTI(std::move(doubler));
            REQUIRE(2.0 == withRTTI(1));
            auto &type = withRTTI.type2();
            REQUIRE(typeid(decltype(doubler)) == type);
        }
    }
}
