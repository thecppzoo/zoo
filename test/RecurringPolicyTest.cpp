//
//  RecurringPolicyTest.cpp
//
//  Created by Eduardo Madrid on 7/1/19.
//

#include "RecurringPolicy.h"

#include "GenericAnyTests.h"

TEST_CASE("Recurring policy", "[recurring][any]") {
    testAnyImplementation<zoo::AnyContainer<RecurringPolicy>>();
}
