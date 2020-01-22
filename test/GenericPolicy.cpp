#include "zoo/Any/DerivedVTablePolicy.h"
#include "zoo/Any/AnyMovable.hpp"
#include "zoo/AlignedStorage.h"

#include <utility>
#include <type_traits>

namespace zoo {

using namespace std;

// containers without copy are move-only
using MoveOnlyPolicy = Policy<void *, Destroy, Move>;
static_assert(!detail::AffordsCopying<MoveOnlyPolicy>::value);
using MOAC = AnyContainer<MoveOnlyPolicy>;
static_assert(2 * sizeof(void *) == sizeof(MOAC));
static_assert(!is_copy_constructible_v<MOAC>);
static_assert(is_nothrow_move_constructible_v<MOAC>);

// containers with copy are copyable
using CopyableNonComposedPolicy = Policy<void *, Destroy, Move, Copy>;
static_assert(detail::AffordsCopying<CopyableNonComposedPolicy>::value);
using CopyableNonComposedAC = AnyContainer<CopyableNonComposedPolicy>;
static_assert(is_copy_constructible_v<CopyableNonComposedAC>);
static_assert(is_nothrow_move_constructible_v<CopyableNonComposedAC>);

using CopyableAnyContainerBasedPolicy = DerivedVTablePolicy<MOAC, Copy>;
static_assert(detail::AffordsCopying<CopyableAnyContainerBasedPolicy>::value);
using DerivedCopyableAC = AnyContainer<CopyableAnyContainerBasedPolicy>;
static_assert(is_copy_constructible_v<DerivedCopyableAC>);
static_assert(is_nothrow_move_constructible_v<DerivedCopyableAC>);

// Container compatibility based on policy is not yet implemented
using LargeMoveOnlyPolicy = Policy<void *[2], Destroy, Move>;
static_assert(
    !std::is_constructible_v<
        zoo::AnyContainerBase<MoveOnlyPolicy>,
        const zoo::AnyContainerBase<LargeMoveOnlyPolicy> &
    >
);

namespace detail { // IsAnyContainer_impl trait

struct Unrelated {};
static_assert(!IsAnyContainer_impl<Unrelated>::value);

struct HasPolicyType { using Policy = int; };
static_assert(!IsAnyContainer_impl<HasPolicyType>::value);

struct MoacDerivative: MOAC {};
static_assert(IsAnyContainer_impl<MoacDerivative>::value);

}}

#include "catch2/catch.hpp"

#include <sstream>

/// Builds a type trait that determines if the __VA_ARGS__ is callable with
/// a const-reference
#define MAY_CALL_EXPRESSION(name, ...) \
template<typename, typename = void> \
struct name: std::false_type {}; \
template<typename T> \
struct name<T, std::void_t<decltype(__VA_ARGS__(std::declval<const T &>()))>>: \
    std::true_type \
{};

MAY_CALL_EXPRESSION(HasToString, to_string)
#define INSERTION_OPERATION_EXPRESSION(arg) std::declval<std::ostream &>() << arg
MAY_CALL_EXPRESSION(HasOperatorInsertion, INSERTION_OPERATION_EXPRESSION)

struct Complex { double real, imaginary; };

std::ostream &operator<<(std::ostream &out, Complex in) {
    return out << in.real << " + i" << in.imaginary;
}

std::string to_string(Complex cx) {
    std::ostringstream ostr;
    ostr << '(' << cx << ')';
    return ostr.str();
}

struct Uninsertable {};

static_assert(!HasToString<int>::value);
static_assert(HasToString<Complex>::value);
static_assert(HasOperatorInsertion<int>::value);
static_assert(HasOperatorInsertion<Complex>::value);
static_assert(!HasOperatorInsertion<Uninsertable>::value);

template<typename T>
std::string stringize(const T &t) {
    if constexpr(HasToString<T>::value) { return to_string(t); }
    else if constexpr(HasOperatorInsertion<T>::value) {
        std::ostringstream ostr;
        ostr << t;
        return ostr.str();
    }
    else return "";
}

template<typename ConcreteValueManager>
std::string stringizeAffordanceImplementation(const void *ptr) {
    return stringize(*static_cast<const ConcreteValueManager *>(ptr)->value());
}

struct Stringize {
    struct VTableEntry { std::string (*str)(const void *); };

    template<typename>
    constexpr static inline VTableEntry Default = {
        [](const void *) { return std::string(); }
    };

    template<typename ConcreteValueManager>
    constexpr static inline VTableEntry Operation = {
        stringizeAffordanceImplementation<ConcreteValueManager>
    };

    // No extra state/functions needed in the memory layout
    template<typename Container>
    struct Mixin {};

    template<typename AnyC>
    struct UserAffordance {
        std::string stringize() const {
            auto container =
                const_cast<AnyC *>(static_cast<const AnyC *>(this))->container();
            return container->template vTable<Stringize>()->str(container);
        }
    };
};

TEST_CASE("Composed Policies", "[type-erasure][any][composed-policy][vtable-policy]") {
    using MoveOnlyAnyContainer = zoo::AnyContainer<zoo::MoveOnlyPolicy>;
    using ComposedPolicy = zoo::DerivedVTablePolicy<MoveOnlyAnyContainer, zoo::Copy>;
    using ComposedAC = zoo::AnyContainer<ComposedPolicy>;

    SECTION("Derived defaults preserved") {
        ComposedAC defaulted;
        REQUIRE(&ComposedPolicy::DefaultImplementation::Default == defaulted.container()->ptr_);
    }
    SECTION("Operational") {
        ComposedAC operational = 34;
        REQUIRE(&ComposedPolicy::Builder<int>::Operations == operational.container()->ptr_);
    }
    SECTION("Compatibility with super container") {
        ComposedAC operational = 34;
        zoo::AnyContainer<zoo::MoveOnlyPolicy> &moacReference = operational, moac;
        REQUIRE(&ComposedPolicy::Builder<int>::Operations == moacReference.container()->ptr_);
        ComposedAC copy = operational;
        REQUIRE(34 == *copy.state<int>());
        moac = std::move(copy);
        REQUIRE(34 == *moac.state<int>());
        // this line won't compile, exactly as intended: a copyable "any" requires copyability
        //copy = std::move(moac);
    }
}

TEST_CASE("VTable/Composed Policies contract", "[type-erasure][any][composed-policy][vtable-policy][contract]") {
    using Policy = zoo::Policy<void *, zoo::Destroy, zoo::Move, zoo::Copy>;
    using Implementations = zoo::GenericPolicy<void *, zoo::Destroy, zoo::Move, zoo::Copy>;
    static_assert(std::is_same_v<Policy, Implementations::Policy>);
    using VTA = zoo::AnyContainer<Policy>;
    static_assert(std::is_default_constructible_v<VTA>);

    SECTION("Destruction") {
        SECTION("Destruction of default constructed") {
            VTA a;
        }
        int trace = 0;

        struct TracesDestruction {
            int &trace_;

            TracesDestruction(int &trace): trace_(trace) {}
            ~TracesDestruction() { trace_ = 1; }
        };

        SECTION("By value") {
            REQUIRE(0 == trace);
            {
                VTA a(std::in_place_type<TracesDestruction>, trace);
                REQUIRE(
                    &Implementations::ByValue<TracesDestruction>::Operations ==
                    a.container()->ptr_
                );
            }
            REQUIRE(1 == trace);
        }
        SECTION("By reference") {
            struct Large: TracesDestruction {
                using TracesDestruction::TracesDestruction;
                double whatever[3];
            };
            REQUIRE(0 == trace);
            {
                VTA a(std::in_place_type<Large>, trace);
                REQUIRE(
                    &Implementations::ByReference<Large>::Operations ==
                    a.container()->ptr_
                );
            }
            REQUIRE(1 == trace);
        }
    }
    SECTION("Moving") {
        struct TracesMoves {
            int *resource_;

            TracesMoves(int *p): resource_(p) {}

            TracesMoves(const TracesMoves &) {
                throw std::runtime_error("unreachable called");
            }

            TracesMoves(TracesMoves &&m) noexcept: resource_(m.resource_) {
                m.resource_ = nullptr;
            }

            ~TracesMoves() {
                if(resource_) { *resource_ = 0; }
            }
        };
        int something = 1;
        SECTION("By Value") {
            {
                VTA a;
                {
                    VTA tmp(std::in_place_type<TracesMoves>, &something);
                    REQUIRE(1 == something);
                    REQUIRE(
                        &Implementations::ByValue<TracesMoves>::Operations ==
                        tmp.container()->ptr_
                    );
                    a = std::move(tmp);
                }
                REQUIRE(1 == something); // if tmp was not "moved-from" in the
                // previous scope, then the destruction would have cleared
                // something
            }
            REQUIRE(0 == something); // a referred to something, cleared
        }
        SECTION("By Reference") {
            // moving for objects not suitable for the local buffer is
            // the transfer of the pointer
            struct Big {
                void *myself_;
                int *padding_;
                Big(): myself_(this) {}
            };
            static_assert(
                std::is_same_v<
                    Implementations::ByReference<Big>,
                    typename Policy::Builder<Big>
                >
            );
            VTA a(std::in_place_type<Big>);
            auto originalPlace = a.state<Big>()->myself_;
            VTA other(std::move(a));
            a.reset();
            REQUIRE(other.state<Big>()->myself_ == originalPlace);
        }
    }
    SECTION("Flexible affordances") {
        using RTTIMO = zoo::Policy<void *, zoo::Destroy, zoo::Move, zoo::RTTI>;
        using RTTIPolicy = zoo::ExtendedAffordancePolicy<RTTIMO, zoo::RTTI>;
        using RTTIMOAC = zoo::AnyContainer<RTTIPolicy>;
        RTTIMOAC mo;
        SECTION("Defaulted") {
            auto &typeidVoid = typeid(void);
            auto &mot = mo.type();
            REQUIRE(typeidVoid == mot);
        }
        SECTION("Value") {
            mo = 4;
            REQUIRE(typeid(int) == mo.type());
        }
        SECTION("Reference") {
            struct Complex { double real; double img; };
            mo = Complex{};
            REQUIRE(typeid(Complex) == mo.type());
        }
    }
    SECTION("User affordance") {
        zoo::AnyContainer<
            zoo::Policy<void *, zoo::Destroy, zoo::Move, Stringize>
        > stringizable;
        REQUIRE("" == stringizable.stringize());
        stringizable = 88;
        auto str = stringizable.stringize();
        REQUIRE("88" == stringizable.stringize());
        Complex c{0, 1};
        stringizable = c;
        REQUIRE(to_string(c) == stringizable.stringize());
    }
}

// What follows is work-in-progress
namespace zoo {

template<std::size_t LocalBufferInPointers, typename Signature>
using CallableViaVTablePolicy =
    typename GenericPolicy<
        void *[LocalBufferInPointers],
        Destroy, Move, CallableViaVTable<Signature>
    >::Policy;

template<std::size_t Pointers, typename Signature>
using NewZooFunction = AnyContainer<CallableViaVTablePolicy<Pointers, Signature>>;
template<std::size_t Pointers, typename Signature>
using NewCopyableFunction =
    AnyContainer<DerivedVTablePolicy<NewZooFunction<Pointers, Signature>, Copy>>;

template<typename>
struct Executor;

template<typename R, typename... As>
struct Executor<R(As...)> {
    R (*executor_)(As..., void *) = [](As..., void *) -> R { throw 5; };

    Executor() = default;
    Executor(R (*ptr)(As..., void *)): executor_(ptr) {}
};

template<typename>
struct Function;

template<typename R, typename... As>
struct Function<R(As...)>: Executor<R(As...)>, AnyContainer<MoveOnlyPolicy> {
    using ContainerBase = AnyContainer<MoveOnlyPolicy>;

    template<typename A>
    static R doA(As... args, void *p) {
        return (*static_cast<Function *>(p)->template state<A>())(args...);
    }

    Function() = default;

    using ContainerBase::ContainerBase;

    template<typename A>
    Function(A &&a): Executor<R(As...)>(doA<std::decay_t<A>>), ContainerBase(std::forward<A>(a)) {}

    template<typename... CallArguments>
    R operator()(CallArguments &&...cas) { return this->executor_(std::forward<CallArguments>(cas)..., this); }
};

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
    SECTION("Executor is instance member Function") {
        using F = zoo::Function<int(double)>;
        F f = doubler;
        //REQUIRE(6.0 == f(3));
        using CopyableFunctionPolicy = zoo::DerivedVTablePolicy<F, zoo::Copy>;
        using CF = zoo::AnyContainer<CopyableFunctionPolicy>;
        CF aCopy = doubler;
        F &reference = aCopy;
    }
}

