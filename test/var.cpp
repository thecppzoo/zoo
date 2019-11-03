#include "zoo/var.h"
#include <catch2/catch.hpp>

namespace {
struct HasDestructor {
    int *ip_;
    HasDestructor(int *ip): ip_{ip} { *ip_ = 1; }
    ~HasDestructor() { *ip_ = 0; }
};

struct IntReturns1 {
    template<typename T>
    int operator()(T) { return 0; }
    int operator()(int) { return 1; }
};

struct MoveThrows {
    MoveThrows() = default;
    MoveThrows(const MoveThrows &) = default;
    MoveThrows(MoveThrows &&) noexcept(false);
};

struct CountsConstructionDestruction {
    static int counter_;
    CountsConstructionDestruction() { ++counter_; }
    CountsConstructionDestruction(const CountsConstructionDestruction &) { ++counter_; }
    ~CountsConstructionDestruction() { --counter_; }
};

int CountsConstructionDestruction::counter_ = 0;
}

static_assert(std::is_nothrow_default_constructible_v<zoo::Var<int>>);
static_assert(!std::is_default_constructible_v<HasDestructor>);
static_assert(
    !std::is_default_constructible_v<zoo::Var<HasDestructor>>,
    "Not default constructible first alternative implies not default constructible Var"
);
struct ThrowingDefaultConstructor {
    ThrowingDefaultConstructor() noexcept(false);
};
static_assert(!std::is_nothrow_default_constructible_v<ThrowingDefaultConstructor>);
static_assert(
    !std::is_default_constructible_v<zoo::Var<ThrowingDefaultConstructor>>,
    "may-throw default constructible first alternative implies not default constructible Var"
);

struct NonCopiable {
    NonCopiable() = default;
    NonCopiable(const NonCopiable &) = delete;
};
static_assert(!std::is_copy_constructible_v<zoo::Var<int, NonCopiable, char>>);

static_assert(!std::is_nothrow_move_constructible_v<MoveThrows>);
static_assert(!std::is_move_constructible_v<zoo::Var<int, MoveThrows, char>>);
static_assert(std::is_nothrow_move_constructible_v<zoo::Var<int, char, long>>);

TEST_CASE("Var", "[var]") {
    int value = 4;
    using V2 = zoo::Var<int, char>;
    static_assert(std::is_nothrow_move_constructible_v<V2>, "");
    using V3 = zoo::Var<int, MoveThrows, char>;
    static_assert(!std::is_nothrow_move_constructible_v<V3>, "");
    static_assert(
        noexcept(std::swap(std::declval<V2 &>(), std::declval<V2 &>())), ""
    );

    using V = zoo::Var<int, HasDestructor>;
    SECTION("Proper construction") {
        V var{std::in_place_index_t<0>{}, 77};
        REQUIRE(77 == *var.as<int>());
    }
    SECTION("Destructor Called") {
        {
            V var{std::in_place_index_t<1>{}, &value};
            REQUIRE(1 == value);
        }
        REQUIRE(0 == value);
    }
    SECTION("visit") {
        V instance;
        auto result = visit(IntReturns1{}, instance);
        REQUIRE(1 == result);
        instance = V{std::in_place_index_t<1>{}, &value};
        result = visit(IntReturns1{}, instance);
        REQUIRE(0 == result);
        value = 5;
        instance = V{std::in_place_index_t<0>{}, 77};
        SECTION("Internally held object is destroyed on move assignment") {
            REQUIRE(0 == value);
        }
        result = visit(IntReturns1{}, instance);
        REQUIRE(1 == result);

        SECTION("Value category and constness preserved") {
            // get preserves the value category
            static_assert(
                std::is_same_v<HasDestructor &&, decltype(zoo::get<1>(V{}))>
            );

            auto usedAnRValueReference = visit(
                [](auto &&arg) {
                    return std::is_rvalue_reference_v<decltype(arg)>;
                },
                V{}
            );
            CHECK(usedAnRValueReference);

            auto constnessDetector = [](auto &arg) {
                return std::is_const_v<std::remove_reference_t<decltype(arg)>>;
            };
            const V constant{};
            CHECK(visit(constnessDetector, constant));
        }
    }
    SECTION("GCC Visit") {
        int dummy;
        V instance{std::in_place_index<1>, &dummy};
        REQUIRE(0 == GCC_visit(IntReturns1{}, instance));
        instance = V{std::in_place_index<0>, 99};
        REQUIRE(1 == GCC_visit(IntReturns1{}, instance));
    }
    /*SECTION("Array support") {
        using VArr = zoo::Var<int, CountsConstructionDestruction[4]>;
        CountsConstructionDestruction::counter_ = 0;
        {
            VArr v{std::in_place_index_t<1>{}};
            CHECK(4 == CountsConstructionDestruction::counter_);
            VArr copy{v};
            CHECK(8 == CountsConstructionDestruction::counter_);
        }
        CHECK(0 == CountsConstructionDestruction::counter_);
    }*/
    SECTION("Match") {
        using V = zoo::Var<int, double>;
        V vid;
        auto doMatch = [](auto &&v) {
            return match(
                std::forward<decltype(v)>(v),
                [](const int &) { return -2.0; },
                [](int &&) { return -1.0; },
                [](double d) { return d; }
            );
        };
        SECTION("Preserves value category") {
            CHECK(-2.0 == doMatch(vid));
            CHECK(-1.0 == doMatch(V{}));
        }
        SECTION("Normal activation") {
            vid = V{std::in_place_index_t<0>{}, 3};
            CHECK(3.14 == doMatch(V{std::in_place_index_t<1>{}, 3.14}));
        }
    }
}
