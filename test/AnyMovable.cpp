//
//  AnyMovable.cpp
//
//  Created by Eduardo Madrid on 6/24/19.
//

#include "zoo/Any/VTable.h"
#include "zoo/any.h"

#include "catch2/catch.hpp"

namespace zoo {

template<typename T>
constexpr auto Movable =
    std::is_nothrow_move_constructible_v<T> &&
    std::is_nothrow_move_assignable_v<T>
;

template<typename What, typename Other>
constexpr auto ConvertibleOnlyFromRValue =
    std::is_constructible_v<What, Other &&> &&
    !std::is_constructible_v<What, const Other &> &&
    std::is_assignable_v<What, Other &&> &&
    !std::is_assignable_v<What, const Other &>
;

template<typename T>
constexpr auto MoveOnly =
    Movable<T> &&
    ConvertibleOnlyFromRValue<T, T>
;

}

using CP = zoo::CanonicalPolicy;

static_assert(zoo::Movable<int>, "");
static_assert(!zoo::MoveOnly<int>, "");
static_assert(zoo::Movable<zoo::detail::AnyContainerBase<CP>>);

using ACP = zoo::AnyContainer<CP>;
static_assert(std::is_default_constructible_v<ACP>);
static_assert(std::is_move_constructible_v<ACP>);

static_assert(zoo::Movable<zoo::AnyContainer<CP>>, "");
static_assert(!zoo::MoveOnly<zoo::AnyContainer<CP>>, "");
static_assert(zoo::Movable<zoo::AnyMovable<CP>>, "");
static_assert(zoo::MoveOnly<zoo::AnyMovable<CP>>, "");

using AnyMovableCanonical = zoo::AnyMovable<CP>;

static_assert(std::is_default_constructible_v<AnyMovableCanonical>, "");
static_assert(zoo::MoveOnly<AnyMovableCanonical>, "");
static_assert(zoo::ConvertibleOnlyFromRValue<AnyMovableCanonical, int>, "");
static_assert(
    std::is_constructible_v<
        AnyMovableCanonical,
        std::in_place_type_t<int>
    >, ""
);


namespace test_RMO_impl {

struct DoesNotHaveMember {};
static_assert(!zoo::detail::PRMO_impl<DoesNotHaveMember>::value, "");
struct HasMember { constexpr static auto RequireMoveOnly = true; };
static_assert(
    std::is_same_v<const bool, decltype(HasMember::RequireMoveOnly)>, ""
);
static_assert(zoo::detail::PRMO_impl<HasMember>::value, "");

}

using MoveOnlyPolicy = zoo::RTTI::RTTI<sizeof(void *), alignof(void *)>;

static_assert(!zoo::detail::RequireMoveOnly_v<zoo::CanonicalPolicy>, "");
static_assert(zoo::detail::RequireMoveOnly_v<MoveOnlyPolicy>, "");
struct RequireMoveOnlyFalse {
    constexpr static auto RequireMoveOnly = false;
};
static_assert(!zoo::detail::RequireMoveOnly_v<RequireMoveOnlyFalse>, "");

TEST_CASE("AnyMovable", "[any][type-erasure][contract]") {
    SECTION("May construct with single argument l-value in_place_type") {
        struct Def77 {
            int value_;
            Def77(): value_{77} {}
        };
        zoo::AnyMovable<zoo::CanonicalPolicy> z(std::in_place_type<Def77>);
        REQUIRE(typeid(Def77) == z.type());
        CHECK(77 == z.state<Def77>()->value_);
    }
    using Small =
        zoo::AnyMovable<zoo::VTablePolicy<sizeof(void *), alignof(void *)>>;
    SECTION("Destroys") {
        int v = 95;
        struct TracesDestruction {
            TracesDestruction(int &who): who_(who) {}
            ~TracesDestruction() { who_ = 83; }
            int &who_;
        };
        {
            Small inPlace(std::in_place_type<TracesDestruction>, v);
            CHECK(95 == v);
        }
        REQUIRE(83 == v);
    }
    SECTION("Moves") {
        struct TracesMovement {
            int trace_ = 94;
            TracesMovement() = default;
            TracesMovement(const TracesMovement &) = delete;
            TracesMovement(TracesMovement &&m) noexcept: trace_(31)
            { m.trace_ = 76; }
        };
        Small
            originallyEmpty,
            defaulted(std::in_place_type<TracesMovement>);
        auto defaultedPtr = defaulted.state<TracesMovement>();
        CHECK(94 == defaultedPtr->trace_);
        originallyEmpty = std::move(defaulted);
        CHECK(31 == originallyEmpty.state<TracesMovement>()->trace_);
        CHECK(76 == defaultedPtr->trace_);
    }
}

#include "zoo/AnyCallable.h"
#include "zoo/Any/VTable.h"

TEST_CASE("AnyContainer<MoveOnlyPolicy>") {
    using AC = zoo::AnyCallable<zoo::AnyContainer<MoveOnlyPolicy>, int(void *)>;
    AC def;
}
