#include <catch2/catch.hpp>
#include "zoo/map/RobinHood_util.h"
#include "zoo/swar/SWAR.h"

using namespace zoo;
using namespace zoo::swar;
using namespace zoo::rh;

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

using MD35u32Ops = SlotOperations<3,5,u32>;

TEST_CASE("RobinHood basic needle", "[api][mapping][swar][robin-hood]") {

CHECK(0x0403'0201u == MD35u32Ops::needlePSL(0).value());
CHECK(0x0000'0000u == 
        MD35u32Ops::deadline(MD35u32Ops::SM{0x0403'0402u}, MD35u32Ops::needlePSL(0)));
CHECK(0x0080'0000u == 
        MD35u32Ops::deadline(MD35u32Ops::SM{0x0402'0402u}, MD35u32Ops::needlePSL(0)));

CHECK(0x0605'0403u == MD35u32Ops::needlePSL(2).value());
CHECK(0x0080'0000u == 
        MD35u32Ops::deadline(MD35u32Ops::SM{0x0403'0503u}, MD35u32Ops::needlePSL(2)));
CHECK(0x8000'0000u == 
        MD35u32Ops::deadline(MD35u32Ops::SM{0x0205'0504u}, MD35u32Ops::needlePSL(2)));
}

TEST_CASE("Robin Hood", "[api][mapping][swar][robin-hood]") {

CHECK(0x0403'0201u == MD35u32Ops::needlePSL(0).value());
CHECK(0x0000'0000u == 
        MD35u32Ops::deadline(MD35u32Ops::SM{0x0403'0402u}, MD35u32Ops::needlePSL(0)));
CHECK(0x0080'0000u == 
        MD35u32Ops::deadline(MD35u32Ops::SM{0x0402'0402u}, MD35u32Ops::needlePSL(0)));

CHECK(0x0605'0403u == MD35u32Ops::needlePSL(2).value());
CHECK(0x0080'0000u == 
        MD35u32Ops::deadline(MD35u32Ops::SM{0x0403'0503u}, MD35u32Ops::needlePSL(2)));
CHECK(0x8000'0000u == 
        MD35u32Ops::deadline(MD35u32Ops::SM{0x0205'0504u}, MD35u32Ops::needlePSL(2)));
}

using MD35u32 = SlotMetadata<3,5,u32>;

TEST_CASE(
    "BadMix",
    "[hash]"
) {
    u64 v = 0x0000'0000'0000'0001ull;
    // badmix turns to all ones, chopped to 32.
    CHECK(0xffff'ffff'ffff'ffffull == badMixer<64>(v));
    CHECK(0x0000'ffff'ffff'ffffull == badMixer<48>(v));
    CHECK(0x0000'0000'ffff'ffffull == badMixer<32>(v));
    CHECK(0x0000'0000'0000'ffffull == badMixer<16>(v));
}

TEST_CASE(
    "SlotMetadataBasic",
    "[robinhood]"
) {
    using SM = SWAR<8, u32>;

    CHECK(0x0504'0302u == MD35u32Ops::needlePSL(1).value());

    // 0 psl is reserved.
    const auto psl1 = 0x0403'0201u;
    const auto hash1 = 0x8080'8080u;
    {
    MD35u32 m;
    m.data_ = MD35u32::SSL{0x0403'8201};
    CHECK(0x0000'8000u == m.attemptMatch(SM{hash1}, SM{psl1}).value());
    CHECK(0x0000'8000u == MD35u32Ops::attemptMatch(m.data_, SM{hash1}, SM{psl1}).value());
    }
    {
    MD35u32 m;
    m.data_ = MD35u32::SSL{0x0401'8201};
    CHECK(0x0000'8001u == m.attemptMatch(SM{hash1}, SM{psl1}).value());
    CHECK(0x0000'8001u == MD35u32Ops::attemptMatch(m.data_, SM{hash1}, SM{psl1}).value());
    } 

}
