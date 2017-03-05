#include "ep/Poker_io.h"
#include "obsolete/Poker.h"
#include "ep/Poker.h"

#ifdef TESTS
    #define CATCH_CONFIG_MAIN
#else
    #define CATCH_CONFIG_RUNNER
#endif
#include "catch.hpp"

constexpr uint64_t royalFlush() {
    using namespace ep::abbreviations;
    return hA | hK | hQ | hJ | hT;
}

int whole(uint64_t);

TEST_CASE("Poker operations", "[basic]") {
    SECTION("Flush") {
        using namespace ep::abbreviations;
        auto noFlush = sQ;
        REQUIRE(0 == ep::flush(noFlush));

        auto flushOfDiamonds = dK | dT | d7 | d6 | d2;
        auto sevenCardFlush5x2 = noFlush | sA | flushOfDiamonds;
        auto flush = ep::flush(sevenCardFlush5x2);
        auto flushOfSpades = flushOfDiamonds >> 3;
        REQUIRE(flush == flushOfSpades);
    }
}

TEST_CASE("Simple operations", "[basic]") {
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

        auto fivesOverKingThree = ep::SWARRank(s5 | h5 | d5 | sK | c3 | h2);
        ep::CSet toak = { ep::convert(fivesOverKingThree), fivesOverKingThree };
        auto toakHR = ep::handRank(toak);
        REQUIRE(ep::THREE_OF_A_KIND == toakHR.hand);
        REQUIRE((0 | r5) == toakHR.high);
        REQUIRE((rK | r3) == toakHR.low); // successfully ignores the h2

        auto tensAndSixesWithEightHigh = ep::SWARRank(sT | dT | c6 | s6 | h8 | s2);
        ep::CSet twoPairs = { ep::convert(tensAndSixesWithEightHigh), tensAndSixesWithEightHigh };
        auto twoPHR = ep::handRank(twoPairs);
        REQUIRE(ep::TWO_PAIRS == twoPHR.hand);
        REQUIRE((rT | r6) == twoPHR.high);
        REQUIRE((0 | r8) == twoPHR.low);

        auto threesOverKQJ = ep::SWARRank(hK | dQ | cJ | s3 | d3 | s2);
        ep::CSet pair = { ep::convert(threesOverKQJ), threesOverKQJ };
        auto pairHR = ep::handRank(pair);
        REQUIRE(ep::PAIR == pairHR.hand);
        REQUIRE((0 | r3) == pairHR.high);
        REQUIRE((rK | rQ | rJ) == pairHR.low);

        auto KT874 = ep::SWARRank(hK | dT | c8 | c7 | c4 | d3 | s2);
        ep::CSet kingHigh = { ep::convert(KT874), KT874 };
        auto kingHighT874 = ep::handRank(kingHigh);
        REQUIRE(ep::HIGH_CARDS == kingHighT874.hand);
        REQUIRE((rK | rT | r8 | r7 | r4) == kingHighT874.high);
        REQUIRE(0 == kingHighT874.low);
    }
}

/*

TEST_CASE("Fast representation") {
    ep::SWARRank royal(royalFlush());
    SECTION("Poker hands") {
        auto swarSuits = ep::convert(royal);
        //auto ranks = swarSuits.at(ep::abbreviations::sH);
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

        auto fivesOverKingThree = ep::SWARRank(s5 | h5 | d5 | sK | c3 | h2);
        ep::CSet toak = { ep::convert(fivesOverKingThree), fivesOverKingThree };
        auto toakHR = ep::handRank(toak);
        REQUIRE(ep::THREE_OF_A_KIND == toakHR.hand);
        REQUIRE((0 | r5) == toakHR.high);
        REQUIRE((rK | r3) == toakHR.low); // successfully ignores the h2

        auto tensAndSixesWithEightHigh = ep::SWARRank(sT | dT | c6 | s6 | h8 | s2);
        ep::CSet twoPairs = { ep::convert(tensAndSixesWithEightHigh), tensAndSixesWithEightHigh };
        auto twoPHR = ep::handRank(twoPairs);
        REQUIRE(ep::TWO_PAIRS == twoPHR.hand);
        REQUIRE((rT | r6) == twoPHR.high);
        REQUIRE((0 | r8) == twoPHR.low);

        auto threesOverKQJ = ep::SWARRank(hK | dQ | cJ | s3 | d3 | s2);
        ep::CSet pair = { ep::convert(threesOverKQJ), threesOverKQJ };
        auto pairHR = ep::handRank(pair);
        REQUIRE(ep::PAIR == pairHR.hand);
        REQUIRE((0 | r3) == pairHR.high);
        REQUIRE((rK | rQ | rJ) == pairHR.low);

        auto KT874 = ep::SWARRank(hK | dT | c8 | c7 | c4 | d3 | s2);
        ep::CSet kingHigh = { ep::convert(KT874), KT874 };
        auto kingHighT874 = ep::handRank(kingHigh);
        REQUIRE(ep::HIGH_CARDS == kingHighT874.hand);
        REQUIRE((rK | rT | r8 | r7 | r4) == kingHighT874.high);
        REQUIRE(0 == kingHighT874.low);
    }
}

*/

