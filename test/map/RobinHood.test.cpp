#include "zoo/map/RobinHood.h"
#include "zoo/map/RobinHoodAlt.h"
#include "zoo/map/RobinHoodUtil.h"

#include <catch2/catch.hpp>

using namespace zoo;
using namespace zoo::swar;
using namespace zoo::rh;

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

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

using FrontendExample =
    zoo::rh::RH_Frontend_WithSkarupkeTail<int, int, 1024, 5, 3>;

auto instantiateFrontEndDefConst() {
    return new FrontendExample;
}

auto instantiateFED(FrontendExample *ptr) {
    delete ptr;
}

auto instantiateFind(int v, FrontendExample &f) {
    return f.find(v);
}

auto instantiateInsert(int v, FrontendExample &f) {
    return f.insert(v, 0);
}


static_assert(
    0xE9E8E7E6E5E4E3E2ull == RHC::makeNeedle(1, 7).value()
);


TEST_CASE("Robin Hood", "[api][mapping][swar][robin-hood]") {



}


using RH35u32 = zoo::rh::RH_Backend<3,5,u32>;
//makeNeedle
//template<int PSL_Bits, int HashBits, typename U = std::uint64_t>


TEST_CASE("RobinHood basic needle", "[api][mapping][swar][robin-hood]") {

CHECK(0x0403'0201u == RH35u32::makeNeedle(0, 0).value());
CHECK(0x1615'1413u == RH35u32::makeNeedle(2, 2).value());
CHECK(0x1817'1615u == RH35u32::makeNeedle(4, 2).value());

}

TEST_CASE("RobinHood potentialMatches", "[api][mapping][swar][robin-hood]") {
    //U deadline, Metadata<PSL_Bits, HashBits, U> potentialMatches;
    auto needle = RH35u32::Metadata{RH35u32::makeNeedle(2, 2).value()};

    // If haystack is identical to needle, we get no deadline and 4 potential
    // matches.
    auto haystack = RH35u32::Metadata{0x1615'1413u};
    CHECK(haystack.value() == needle.value());
    auto m1 = RH35u32::potentialMatches(needle, haystack);
    CHECK(0x0000'0000u == m1.deadline);
    CHECK(0x8080'8080u == m1.potentialMatches.value());

    // If haystack has no matches and is poorer than needle, we should get no
    // deadline and no potential matches.
    auto missHaystack = RH35u32::Metadata{0x0707'0707u};
    auto m2 = RH35u32::potentialMatches(needle, missHaystack);
    CHECK(0x0000'0000u == m2.potentialMatches.value());
    // We get a deadline here, incorrect.
    CHECK(0x0000'0080u == m2.deadline);

    // If haystack has no matches and has a richer element, we should get a
    // deadline and no matches.
    auto deadlineHaystack = RH35u32::Metadata{0x0515'0403u};
    auto m3 = RH35u32::potentialMatches(needle, deadlineHaystack);
    CHECK(0x0080'0000u == m3.potentialMatches.value());
    // Position 4 has needle psl 6 vs haystack psl 5, which should result in a
    // deadline.
    CHECK(0x0000'0000u == m3.deadline);
}

TEST_CASE(
    "BadMix",
    "[hash]"
) {
    u64 v = 0x0000'0000'0000'0001ull;
    // badmix turns to all ones, chopped to type width.
    CHECK(0xffff'ffff'ffff'ffffull == zoo::rh::badMixer<64>(v));
    CHECK(0x0000'ffff'ffff'ffffull == zoo::rh::badMixer<48>(v));
    CHECK(0x0000'0000'ffff'ffffull == zoo::rh::badMixer<32>(v));
    CHECK(0x0000'0000'0000'ffffull == zoo::rh::badMixer<16>(v));
}

TEST_CASE(
    "SlotMetadataBasic",
    "[robinhood]"
) {
    using SM = SWAR<8, u32>;
    using SO35u32Ops = SlotOperations<3,5,u32>;
    using MD35u32Ops = SlotMetadata<3,5,u32>;
    CHECK(0x0504'0302u == SO35u32Ops::needlePSL(1).value());

    // 0 psl is reserved.
    const auto psl1 = 0x0403'0201u;
    const auto hash1 = 0x8080'8080u;
    {
    MD35u32Ops m;
    m.data_ = MD35u32Ops::SSL{0x0403'8201};
    CHECK(0x0000'8000u == m.attemptMatch(SM{hash1}, SM{psl1}).value());
    CHECK(0x0000'8000u == SO35u32Ops::attemptMatch(m.data_, SM{hash1}, SM{psl1}).value());
    }
    {
    MD35u32Ops m;
    m.data_ = MD35u32Ops::SSL{0x0401'8201};
    CHECK(0x0000'8001u == m.attemptMatch(SM{hash1}, SM{psl1}).value());
    CHECK(0x0000'8001u == SO35u32Ops::attemptMatch(m.data_, SM{hash1}, SM{psl1}).value());
    } 
}
