//
//  AnyCallSignature.cpp
//
//  Created by Eduardo Madrid on 7/22/19.
//

#include "zoo/AnyCallSignature.h"
#include "zoo/AnyContainer.h"
#include "zoo/PolymorphicContainer.h"

#include "Catch2/catch.hpp"

int doubler(int a) { return 2*a; }

int multiply(int f1, int f2) { return f1*f2; }

TEST_CASE("AnyCallSignature") {
    using Canonical =
        zoo::AnyContainer<
            zoo::RuntimePolymorphicAnyPolicy<sizeof(void *), alignof(void *)>
        >;
    using ACS = zoo::AnyCallSignature<Canonical>;
    ACS acs(doubler, std::in_place_type<int(int)>);
    SECTION("Calling") {
        CHECK(6 == acs.call<int(int)>(3));
    }
    SECTION("Constructing from an existing value") {
        ACS copy(acs);
        CHECK(8 == copy.call<int(int)>(4));
    }
    SECTION("Rebinding, to a different signature") {
        acs = ACS(multiply, std::in_place_type<int(int, int)>);
        CHECK(15 == acs.call<int(int, int)>(3, 5));
    }
}
