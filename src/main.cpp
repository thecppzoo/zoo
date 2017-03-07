#include "ep/Poker_io.h"
#include "ep/Poker.h"
#include "ep/Floyd.h"

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

TEST_CASE("Bit operations", "[bit]") {
    auto sevenNibbles = ep::core::makeBitmask<4, uint64_t>(7);
    auto octals = 01234567ull;
    auto deposited = __builtin_ia32_pdep_di(octals, sevenNibbles);
    REQUIRE(0x1234567 == deposited);

    auto alreadySet = 0xC63; // 1 1 0 0 .0 1 1 0 .0 0 1 1
    auto interleave = 0x2A; //  1a0b.1c0d1e0f
    auto expected = 0xEEB; //   1 1 1a0b.1c1 1 0d.1e0f1 1
    auto byEp = ep::deposit(alreadySet, interleave);
    REQUIRE(expected == byEp);
}

TEST_CASE("Poker operations", "[basic]") {
    using namespace ep::abbreviations;
    SECTION("HandRank equality") {
        ep::HandRank hr1(ep::HIGH_CARDS, 1, 0);
        ep::HandRank hr2(ep::HIGH_CARDS, 2, 0);
        auto equal = hr1 == hr2;
        REQUIRE(!equal);
    }
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
        auto threysOverAceKing = (royalFlush() ^ hT) | s3 | d3 | c3;
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
        ep::HandRank comparator(ep::PAIR, 0 | rJ, rK | rQ | rT);
        REQUIRE(comparator == r);
    }

    SECTION("High Cards") {
        auto nothing = (royalFlush() ^ hA) | c8 | c7 | c6;
        auto r = ep::handRank(nothing);
        REQUIRE(ep::HandRank(ep::HIGH_CARDS, 0, rK | rQ | rJ | rT | r8) == r);
    }
}
