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

TEST_CASE("AnyCallSignature") {
    using Canonical = zoo::AnyContainer<zoo::RuntimePolymorphicAnyPolicy<sizeof(void *), alignof(void *)>>;
    using ACS = zoo::AnyCallSignature<Canonical>;
    ACS acs(doubler, std::in_place_type<int(int)>);
    CHECK(6 == acs.call<int(int)>(3));
    ACS copy(acs);
    CHECK(8 == copy.call<int(int)>(4));
}
