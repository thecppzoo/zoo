//
//  PolicyAffordances.cpp
//
//  Created by Eduardo Madrid on 6/27/19.
//

#include "zoo/AnyContainer.h"
#include "zoo/Any/VTable.h"

#include "catch2/catch.hpp"

namespace test_affordances {

/// \brief this should give the containers a method "thy" through CRTP
template<typename C>
struct MockAffordances {
    void *thy() {
        return static_cast<C *>(this);
    }
};

struct MockPolicy {
    template<typename C>
    using Affordances = MockAffordances<C>;
};

struct MockContainer;

static_assert(std::is_same_v<
    MockAffordances<MockContainer>,
    zoo::detail::PolicyAffordances<MockContainer, MockPolicy>
>, "");

/// \brief should get the operation "thy" from the affordances
struct MockContainer:
    zoo::detail::PolicyAffordances<MockContainer, MockPolicy>
{};

}

TEST_CASE("Mock Affordances", "[any][mostly-compile-time]") {
    test_affordances::MockContainer mc;
    REQUIRE(&mc == mc.thy());
}

constexpr auto
    VPSize = sizeof(void *),
    VPAlignment = alignof(void *);

TEST_CASE("Affordances", "[any]") {
    zoo::AnyMovable<zoo::RTTI::RTTI<VPSize, VPAlignment>> rttiAny;
    REQUIRE(typeid(void) == rttiAny.type());
}

