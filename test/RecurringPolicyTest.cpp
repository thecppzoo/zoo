//
//  RecurringPolicyTest.cpp
//
//  Created by Eduardo Madrid on 7/1/19.
//

#include "RecurringPolicy.h"

#include "GenericAnyTests.h"

TEST_CASE("Recurring policy", "[recurring][any][contract]") {
    testAnyImplementation<zoo::AnyContainer<RecurringPolicy>>();
}

TEST_CASE("Cheap RTTI", "") {
    using Z = zoo::AnyContainer<RecurringPolicy>;
    Z a(5), b(5.5);
    SECTION("The type index for integer and double must be different") {
        CHECK(a.cheap() != b.cheap());
    }
    SECTION("The type index for another int must be the same as it was") {
        Z anotherInt(2);
        CHECK(a.cheap() == anotherInt.cheap());
    }
    SECTION("The type indices are shared among the held value type") {

    }
}
