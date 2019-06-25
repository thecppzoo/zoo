//
//  AnyMovable.cpp
//  Zoo wrap
//
//  Created by Eduardo Madrid on 6/24/19.
//  Copyright © 2019 Eduardo Madrid. All rights reserved.
//

#include "zoo/AnyMovable.h"

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

TEST_CASE("AnyMovable", "[any][type-erasure][contract]") {
    struct DefaultsTo77 {
        int value_;
        DefaultsTo77(): value_{77} {}
    };
    zoo::AnyMovable<zoo::CanonicalPolicy> z(std::in_place_type<DefaultsTo77>);
    REQUIRE(typeid(DefaultsTo77) == z.type());
    CHECK(77 == z.state<DefaultsTo77>()->value_);
}
