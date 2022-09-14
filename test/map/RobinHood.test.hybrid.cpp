#include "zoo/map/RobinHood.h"
#include "zoo/map/RobinHoodAlt.h"
#include "zoo/map/RobinHoodUtil.h"
#include "zoo/map/RobinHoodTest.h"
#include "zoo/map/RobinHood_straw.h"

#include <catch2/catch.hpp>

#include <algorithm>
#include <regex>
#include <map>

using namespace zoo;
using namespace zoo::swar;
using namespace zoo::rh;

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

bool intCheck(int n, int h) { return n == h; }

// We'll be able to convert data from straw to real as long as we stay out of
// the Skarupke tail.
using Robin =
    zoo::rh::RH_Frontend_WithSkarupkeTail<int, int, 16, 5, 3,
        std::hash<int>, std::equal_to<int>, u32>;
using Straw = StrawmanMap<5, 3, u32>;

TEST_CASE("Robin Hood Metadata straw hybrid",
          "[api][mapping][swar][robin-hood]") {
    Robin rh;
    Straw straw(16);

    CHECK(straw.insert(5, &intCheck).first == 1);
    CHECK(straw.insert(7, &intCheck).first == 2);
    CHECK(straw.insert(9, &intCheck).first == 3);

    CHECK(3 == copyMetadata(straw, rh));
}
