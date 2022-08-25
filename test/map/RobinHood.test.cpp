#include "zoo/map/RobinHood.h"

#include <catch2/catch.hpp>

// Robin Hood, canonical test
using RHC = zoo::rh::RH_Backend<5, 3>;

int *collectionOfKeys;
RHC::Metadata *md;

auto blue(int sPSL, int hh, int key, int homeIndex) {
    RHC fromPointer{md};
    auto r =
        fromPointer.findMisaligned_assumesSkarupkeTail(
            hh,
            homeIndex,
            [&](int matchIndex) { return collectionOfKeys[matchIndex] == key; }
        );
}

auto instantiateFrontEndDefConst() {
    return new zoo::rh::RH_Frontend_WithSkarupkeTail<int, int, 1024, 5, 3>;
}

auto instantiateFED(zoo::rh::RH_Frontend_WithSkarupkeTail<int, int, 1024, 5, 3> *ptr) {
    delete ptr;
}


static_assert(
    0xE9E8E7E6E5E4E3E2ull == RHC::makeNeedle(1, 7).value()
);


TEST_CASE("Robin Hood", "[api][mapping][swar][robin-hood]") {



}

