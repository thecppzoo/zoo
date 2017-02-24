#include "ep/Poker_io.h"
#include "ep/Poker.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

constexpr uint64_t royalFlush() {
    using namespace ep::abbreviations;
    return hA | hK | hQ | hJ | hT;
}

TEST_CASE("Simple operations", "[basic]") {
    using namespace ep::abbreviations;
    ep::SWARRank royal(royalFlush());
    SECTION("Fundamental") {
        auto asSuits = ep::convert(royal);
        auto expected = 0x1Full << (16 + 8);
        REQUIRE(expected == asSuits.value());

        auto others = h3 | royalFlush() | sK;
        auto expected2 = expected | (1 << 17) | (1 << 11);
        ep::SWARRank ranks(others);
        auto sevenCards = ep::convert(ranks);
        REQUIRE(sevenCards.value() == expected2);

        ep::CSet hand = { sevenCards, ranks };
        auto suitCounts = hand.suitCounts();
        auto suit6 = suitCounts.greaterEqual<6>();
        REQUIRE(bool(suit6));
        bool hearts6 = sH == suit6.best();
        REQUIRE(int(sH) == suit6.best());

        auto rankCounts = hand.rankCounts();
        bool rank2 = rankCounts.greaterEqual<2>();
        REQUIRE(rank2);
    }
    //auto v = h3 | royalFlush() | cK; // has six hearts, a pair of kings
}
