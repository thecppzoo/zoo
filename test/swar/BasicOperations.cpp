#include "zoo/swar/metaLog.h"
#include "zoo/swar/SWAR.h"

#include "catch2/catch.hpp"

#include <type_traits>

using namespace zoo;

/// TODO(scottbruceheart) This test is wretched, fix it.
TEST_CASE(
    "Basic SWAR",
    "[swar]"
) {
    uint8_t v = 0x8;
    SECTION("Simple Mask") {
      auto mask = swar::makeBitmask<8, uint32_t>(v);
      CHECK(mask == 0x0808'0808ull);
    }
}
