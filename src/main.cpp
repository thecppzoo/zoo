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

ep::HandRank fh(uint64_t c) {
    using namespace ep;
    SWARRank sr(c);
    RankCounts rc(sr);
    auto toaks = rc.greaterEqual<3>();
    if(toaks) {
        auto best = toaks.top();
        auto without = rc.clearAt(best);
        auto pairs = without.greaterEqual<2>();
        if(pairs) {
            return { FULL_HOUSE, 1 << best, 1 << pairs.top() };
        }
    }
    return { HIGH_CARDS, 0, 0 };
}

TEST_CASE("Poker operations", "[basic]") {
    using namespace ep::abbreviations;
    SECTION("Flush") {
        auto noFlush = sQ;
        REQUIRE(0 == ep::flush(noFlush));

        auto flushOfDiamonds = dK | dT | d7 | d6 | d2;
        auto sevenCardFlush5x2 = noFlush | sA | flushOfDiamonds;
        auto flush = ep::flush(sevenCardFlush5x2);
        auto flushOfClubs = flushOfDiamonds >> (sD - sS);
        REQUIRE(flush == flushOfClubs);

        auto heartsRoyalFlush = royalFlush();
        auto hrf = ep::SWARRank(heartsRoyalFlush);
        auto hrcounted = ep::RankCounts(hrf);
        auto present = hrcounted.greaterEqual<1>();
        auto straightsResult = ep::straights_rankRepresentation(present);
        REQUIRE(rA == straightsResult.top());
    }
    
    SECTION("Straight") {
        auto straightTo5 = (royalFlush() ^ hT) | d2 | c3 | d4 | s5;
        auto sr5 = ep::SWARRank(straightTo5);
        auto p5 = ep::RankCounts(sr5);
        auto p5Present = p5.greaterEqual<1>();
        auto straightTo5Result = ep::straights_rankRepresentation(p5Present);
        REQUIRE(r5 == straightTo5Result.top());

        auto no = straightTo5 ^ s5;
        auto noRanks = ep::RankCounts(ep::SWARRank(no)).greaterEqual<1>();
        auto notStraight = ep::straights_rankRepresentation(noRanks);
        REQUIRE(!notStraight);
    }
}

TEST_CASE("Hands", "[basic]") {
    using namespace ep::abbreviations;

    SECTION("Straight Flush") {
        auto straightFlushCards = h4 | h5 | h6 | h7 | h8 | c8 | d8;
        auto result = ep::handRank(straightFlushCards);
        REQUIRE(ep::STRAIGHT_FLUSH == result.hand);
        REQUIRE(0 == result.high);
        REQUIRE((1 << r8) == result.low);   
    }

    SECTION("Four of a kind") {
        auto foakCards = s5 | h5 | d5 | c5 | sK | cK | hK;
        auto result = ep::handRank(foakCards);
        REQUIRE(ep::FOUR_OF_A_KIND == result.hand);
        REQUIRE((1 << r5) == result.high);
        REQUIRE((1 << rK) == result.low);
    }

    SECTION("Full House") {
        auto threysOverKings = s3 | d3 | h3 | dK | cK | sA | dQ;
        auto r = ep::handRank(threysOverKings);
        REQUIRE(ep::HandRank(ep::FULL_HOUSE, 0 | r3, 0 | rK) == r);
        auto doubleToak = s3 | d3 | h3 | hA | dA | cA | dK;
        auto acesOverThreys = ep::handRank(doubleToak);
        REQUIRE(ep::HandRank(ep::FULL_HOUSE, 0 | rA, 0 | r3) == acesOverThreys);
    }

    SECTION("Flush") {
        auto baseFlush = (royalFlush() ^ hA) | h8;
        auto extraKings = sK | cK;
        auto wholeHand = baseFlush | extraKings;
        auto r = ep::handRank(wholeHand);
        auto kqjt8 = ep::HandRank(ep::FLUSH, 0, rK | rQ | rJ | rT | r8);
        REQUIRE(kqjt8 == r);

        auto withoutTOAK = baseFlush | h5 | h4;
        auto without = ep::handRank(withoutTOAK);
        REQUIRE(kqjt8 == without);
    }

    SECTION("Straight") {
        auto fromAce = sA | h2 | d3 | c4 | s5 | d5 | c5;
        auto r = ep::handRank(fromAce);
        REQUIRE(ep::HandRank(ep::STRAIGHT, 0, 0 | r5) == r);
    }

    SECTION("Three of a kind") {
        auto threysOverAceKing = (royalFlush() ^ hT) | h3 | d3 | c3;
        auto r = ep::handRank(threysOverAceKing);
        REQUIRE(ep::HandRank(ep::THREE_OF_A_KIND, 0 | r3, rA | rK) == r);
    }

    SECTION("Two Pairs") {
        auto kingsOverTensAceHigh = (royalFlush() ^ hJ) | cK | cT | c9;
        auto r = ep::handRank(kingsOverTensAceHigh);
        REQUIRE(ep::HandRank(ep::TWO_PAIRS, rK | rT, 0 | rA) == r);
    }

    SECTION("Pairs") {
        auto hooks = (royalFlush() ^ hA) | cJ | s2 | s3;
        auto r = ep::handRank(hooks);
        REQUIRE(ep::HandRank(ep::PAIR, 0 | rJ, rK | rQ | rT) == r);
    }

    SECTION("High Cards") {
        auto nothing = (royalFlush() ^ hA) | c8 | c7 | c6;
        auto r = ep::handRank(nothing);
        REQUIRE(ep::HandRank(ep::HIGH_CARDS, 0, rK | rQ | rJ | rT | r8) == r);
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

