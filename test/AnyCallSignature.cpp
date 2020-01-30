//
//  AnyCallSignature.cpp
//
//  Created by Eduardo Madrid on 7/22/19.
//

#include "zoo/AnyCallSignature.h"
#include "zoo/FunctionPolicy.h"
#include "zoo/AnyContainer.h"

#include "catch2/catch.hpp"

int doubler(int a) { return 2*a; }

int multiply(int f1, int f2) { return f1*f2; }

TEST_CASE("AnyCallSignature", "[any][type-erasure][functional][undefined-behavior]") {
    using Canonical =
        zoo::AnyContainer<
            //zoo::RuntimePolymorphicAnyPolicy<sizeof(void *), alignof(void *)>
            zoo::Policy<void *, zoo::Destroy, zoo::Move, zoo::Copy>
        >;
    using ACS = zoo::AnyCallSignature<Canonical>;
    ACS acs(std::in_place_type<int(int)>, doubler);
    SECTION("Calling") {
        CHECK(6 == acs.as<int(int)>()(3));
    }
    SECTION("Constructing from an existing value") {
        ACS copy(acs);
        CHECK(8 == copy.as<int(int)>()(4));
    }
    SECTION("Rebinding, to a different signature") {
        acs = ACS(std::in_place_type<int(int, int)>, multiply);
        CHECK(15 == acs.as<int(int, int)>()(3, 5));
    }
}
