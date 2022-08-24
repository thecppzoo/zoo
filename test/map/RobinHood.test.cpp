#include "zoo/map/RobinHood.h"

#include <catch2/catch.hpp>

// Robin Hood, canonical test
using RHC = zoo::swar::rh::RH<5, 3>;

static_assert(
    0xE9E8E7E6E5E4E3E2ull == RHC::makeNeedle(1, 7).value()
);

auto blue(RHC &rh, int sPSL, int hh, zoo::swar::GeneratorFromPointer<RHC::Metadata> &mp) {
    return rh.startMatch(sPSL, hh, mp);
}

TEST_CASE("Robin Hood", "[api][mapping][swar][robin-hood]") {



}

