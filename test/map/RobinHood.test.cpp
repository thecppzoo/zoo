#include "zoo/map/RobinHood.h"

#include <catch2/catch.hpp>

// Robin Hood, canonical test
using RHC = zoo::swar::rh::RH<5, 3>;

static_assert(
    0xE9E8E7E6E5E4E3E2ull == RHC::makeNeedle(1, 7).value()
);

TEST_CASE("Robin Hood", "[api][mapping][swar][robin-hood]") {



}

