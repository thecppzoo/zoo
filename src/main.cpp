#include "ep/Poker_io.h"
#include "ep/Poker.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

constexpr uint64_t royalFlush() {
    using namespace ep::abbreviations;
    return hA | hK | hQ | hJ | hT;
}


TEST_CASE("Simple operations", "[basic]") {
    int v2;
    ep::SWARRank royal(royalFlush());
    SECTION("Fundamental") {
        using namespace ep::abbreviations;
        auto asSuits = ep::convert(royal);
        auto expected = 0x1Full << (16 + 8);
        REQUIRE(expected == asSuits.value());

        auto others = h3 | royalFlush() | sK; // has six hearts, a pair of kings
        auto expected2 = expected | (1 << 17) | (1 << 11);
        ep::SWARRank ranks(others);
        auto sevenCards = ep::convert(ranks);
        REQUIRE(sevenCards.value() == expected2);

        ep::CSet hand = { sevenCards, ranks };
        auto suitCounts = hand.suitCounts();
        auto suit6 = suitCounts.greaterEqual<6>();
        REQUIRE(suit6);
        bool hearts6 = sH == suit6.best();
        REQUIRE(int(sH) == suit6.best());

        auto rankCounts = hand.rankCounts();
        auto rank2 = rankCounts.greaterEqual<2>();
        REQUIRE(rank2);
        REQUIRE(rK == rank2.best());
        auto swarSuits = ep::convert(ranks);
        auto bySuitBits = rA | rK | rQ | rJ | rT | r3;
        bySuitBits <<= 16;
        bySuitBits |= rK;
        REQUIRE(bySuitBits == swarSuits.value());
    }
    SECTION("Poker hands") {
        auto swarSuits = ep::convert(royal);
        auto ranks = swarSuits.at(ep::abbreviations::sH);
        auto straight = ep::straights(ranks);
        REQUIRE(straight);
        auto Ace = 1 << ep::abbreviations::rA;
        REQUIRE(Ace == straight);
        using namespace ep::abbreviations;
        auto from2 = rA | r2 | r3 | r4 | r5;
        REQUIRE((1 << r5) == ep::straights(from2));
        auto joint = from2 | ranks;
        auto AceOr5 = Ace | r5;
        REQUIRE(AceOr5 == ep::straights(joint));

        ep::CSet cs = { swarSuits, royal };
        auto sf = ep::handRank(cs);
        REQUIRE(ep::STRAIGHT_FLUSH == sf.hand);
        REQUIRE(Ace == sf.high);
        cs.include(r3, sH);
        sf = ep::handRank(cs);
        REQUIRE(Ace == sf.high);
        REQUIRE(0 == sf.low);

        auto foakBits = h3 | s3 | c3 | d3 | hT | d9 | s8;
        auto foakRanks = ep::SWARRank(foakBits);
        ep::CSet foakHand = { ep::convert(foakRanks), foakRanks };
        auto foakResult = handRank(foakHand);
        REQUIRE(ep::FOUR_OF_A_KIND == foakResult.hand);
        REQUIRE(1 << r3 == foakResult.high);
        REQUIRE(1 << rT == foakResult.low);

        REQUIRE(foakResult.code < sf.code);

        auto fullHouseBits = h3 | c3 | d3 | sA | hA | h2 | d2;
        auto fhr = ep::SWARRank(fullHouseBits);
        ep::CSet fhh = { ep::convert(fhr), fhr };
        auto fhhr = handRank(fhh);
        REQUIRE(ep::FULL_HOUSE == fhhr.hand);
        REQUIRE((0 | r3) == fhhr.high);
        REQUIRE((0 | rA) == fhhr.low);

        auto fhr2 = ep::SWARRank(fullHouseBits | dA);
        ep::CSet fullHouseDoubleThreeOfAKind= { ep::convert(fhr2), fhr2 };
        auto dtfh = ep::handRank(fullHouseDoubleThreeOfAKind);
        REQUIRE(ep::FULL_HOUSE == dtfh.hand);
        REQUIRE((0 | rA) == dtfh.high);
        REQUIRE((0 | r3) == dtfh.low);
        REQUIRE(fhhr.code < dtfh.code);
    }
}
