#pragma once

#include "Poker.h"

namespace ep {

inline int bestFlush(unsigned p1, unsigned p2) {
    return positiveIndex1Better(p1, p2);
}

inline int bestStraight(unsigned s1, unsigned s2) {
    return positiveIndex1Better(s1, s2);
}

inline int bestKickers(
    RankCounts p1s, RankCounts p2s
) {
    return
        positiveIndex1Better(
            p1s.greaterEqual<1>().counts().value(),
            p2s.greaterEqual<1>().counts().value()
        );
}

inline int bestKicker(RankCounts p1s, RankCounts p2s) {
    return bestKickers(p1s, p2s);
}

inline int bestFourOfAKind(
    RankCounts p1s, RankCounts p2s,
    RankCounts p1foaks, RankCounts p2foaks
) {
    auto p1BestNdx = p1foaks.best();
    auto p2BestNdx = p2foaks.best();
    auto diff = positiveIndex1Better(p1BestNdx, p2BestNdx);
    if(diff) { return diff; }
    return bestKicker(p1s.clearAt(p1BestNdx), p2s.clearAt(p2BestNdx));
}

inline unsigned straightFlush(CSet cards, SuitCounts ss) {
    static_assert(TotalHand < 2*5, "Assumes only one flush");
    auto ndx = ss.best();
    return straights(cards.m_bySuit.at(ndx));
}

struct ComparisonResult {
    int64_t ifDecided;
    bool decided;
};

struct FullHouseResult {
    bool isFullHouse;
    int bestThreeOfAKind, bestPair;
};

inline FullHouseResult isFullHouse(RankCounts counts) {
    auto toaks = counts.greaterEqual<3>();
    RARE(toaks) {
        auto bestTOAK = toaks.best();
        auto withoutBestTOAK = counts.clearAt(bestTOAK);
        auto fullPairs = withoutBestTOAK.greaterEqual<2>();
        RARE(fullPairs) { return { true, bestTOAK, fullPairs.best() }; }
    }
    return { false, 0, 0 };
}

unsigned straightsDoingChecks(unsigned rankSet) {
    constexpr auto NumberOfHandCards = 7;
    auto rv = rankSet;
    // 2 1 0 9 8 7 6 5 4 3 2 1 0 bit index
    // =========================
    // 2 3 4 5 6 7 8 9 T J Q K A
    // - 2 3 4 5 6 7 8 9 T J Q K &
    // - - 2 3 4 6 5 6 7 8 9 T J &
    // - - - 2 3 4 5 6 7 8 9 T J &
    // - - - A - - - - - - - - - |
    // - - - A 2 3 4 5 6 7 8 9 T &
    
    
    rv &= (rv >> 1);  // two
    if(!rv) { return rv; }
    rv &= (rv >> 1); // three in sequence
    if(!rv) { return rv; }
    rv &= (rv >> 1); // four in sequence
    if(!rv) { return rv; }
    RARE(1 & rankSet) {
        // the argument had an ace, insert it as if for card number 1
        rv |= (1 << (NRanks - 4));
    }
    rv &= (rv >> 1);
    return rv;
}

inline unsigned straightFrontCheck(unsigned rankSet) {
    constexpr auto mask = (1 << 9) | (1 << 4);
    if(rankSet & mask) { return straights(rankSet); }
    /*auto notStraight = straights(rankSet);
    if(notStraight) {
        static auto count = 5;
        if(5 == count) { pRanks(std::cerr, mask) << '\n' << std::endl; }
        if(--count < 0) { return 0; }
        pRanks(std::cerr, rankSet) << " | ";
        pRanks(std::cerr, notStraight) << std::endl;
    }*/
    return 0;
}
    
inline ComparisonResult winnerPotentialFullHouseGivenThreeOfAKind(
    RankCounts p1s, RankCounts p2s,
    RankCounts p1toaks, RankCounts p2toaks
) {
    auto p1BestThreeOfAKindIndex = p1toaks.best();
    auto p1WithoutBestThreeOfAKind = p1s.clearAt(p1BestThreeOfAKindIndex);
    auto p1FullPairs = p1WithoutBestThreeOfAKind.greaterEqual<2>();
    RARE(p1FullPairs) { // p1 is full house
        RARE(p2toaks) {
            auto p2BestThreeOfAKindIndex = p2toaks.best();
            auto bestDominantIndex =
                positiveIndex1Better(
                    p1BestThreeOfAKindIndex, p2BestThreeOfAKindIndex
                );
            if(0 < bestDominantIndex) {
                // p2's best three of a kind is not as good as p1's,
                // even if a full house itself will lose
                return { 1, true };
            }
            auto p2WithoutBestThreeOfAKind =
                p2s.clearAt(p2BestThreeOfAKindIndex);
            auto p2FullPairs = p2WithoutBestThreeOfAKind.greaterEqual<2>();
            RARE(p2FullPairs) {
                if(bestDominantIndex) { return { bestDominantIndex, true }; }
                return {
                    positiveIndex1Better(
                        p1FullPairs.best(), p2FullPairs.best()
                    ),
                    true
                };
            }
        }
        return { 1, true };
    }
    // p1 is not a full-house
    auto p2BestThreeOfAKindIndex = p2toaks.best();
    auto p2WithoutBestThreeOfAKind = p2s.clearAt(p2BestThreeOfAKindIndex);
    auto p2FullPairs = p2WithoutBestThreeOfAKind.greaterEqual<2>();
    RARE(p2FullPairs) { return { -1, true }; }
    // neither is a full house
    return { 0, false };
}

inline ComparisonResult winnerPotentialFullHouseOrFourOfAKind(
    RankCounts p1s, RankCounts p2s
) {
    // xoptx How can any be full house or four of a kind?
    // xoptx What is the optimal sequence of checks?
    auto p1ThreeOfAKinds = p1s.greaterEqual<3>();
    auto p2ThreeOfAKinds = p2s.greaterEqual<3>();
    RARE(p1ThreeOfAKinds) { // p1 has a three of a kind
        RARE(p2ThreeOfAKinds) { // p2 also has a three of a kind
            auto p1FourOfAKinds = p1s.greaterEqual<4>();
            auto p2FourOfAKinds = p2s.greaterEqual<4>();
            RARE(p1FourOfAKinds) {
                RARE(p2FourOfAKinds) {
                    return {
                        bestFourOfAKind(p1s, p2s, p1FourOfAKinds, p2FourOfAKinds),
                        true
                    };
                }
                return { 1, true };
            } else RARE(p2FourOfAKinds) { return { -1, true }; }
            // No four of a kind
            return
                winnerPotentialFullHouseGivenThreeOfAKind(
                    p1s, p1ThreeOfAKinds, p2s, p2ThreeOfAKinds
                );
        }
        // p2 lacks three of a kind, can not be full-house nor four-of-a-kind
        auto p1BestThreeOfAKindIndex = p1ThreeOfAKinds.best();
        auto p1WithoutBestThreeOfAKind = p1s.clearAt(p1BestThreeOfAKindIndex);
        auto p1FullPairs = p1WithoutBestThreeOfAKind.greaterEqual<2>();
        RARE(p1FullPairs) { return { 1, true }; }
        // not a full house, but perhaps four of a kind?
        RARE(p1s.greaterEqual<4>()) { return { 1, true }; }
        // neither is full house or better, undecided
    } // p1 lacks three of a kind, can not be full-house nor four-of-a-kind
    else RARE(p2ThreeOfAKinds) {
        auto p2BestThreeOfAKindIndex = p2ThreeOfAKinds.best();
        auto p2WithoutBestThreeOfAKind = p2s.clearAt(p2BestThreeOfAKindIndex);
        auto p2FullPairs = p2s.greaterEqual<2>();
        RARE(p2FullPairs) { return { -1, true }; }
        // p2 is not a full house, but perhaps a four of a kind?
        RARE(p2s.greaterEqual<4>()) { return { -1, true }; }
    }
    return { 0, false };
}


/// \todo Handle two pairs and pairs
/// \todo optimize this.  Look for comments with the word xoptx
int winnerCascade(CSet community, CSet p1, CSet p2) {
    // There are three independent criteria
    // n-of-a-kind, straights and flushes
    // since flushes dominate straigts, and have in texas holdem about the
    // same probability than straights, the first inconditional check is
    // for flushes.
    // xoptx all the comparison operations should not be unary but binary
    // in the player cards and community, this would allow calculating the
    // community cards only once.
    p1 = p1 | community;
    auto p1Suits = p1.suitCounts();
    auto p1Flushes = flushes(p1Suits);
    p2 = p2 | community ;
    auto p2Suits = p2.suitCounts();
    auto p2Flushes = flushes(p2Suits);
    // xoptx in the case of flushes, three of a kind, etc., there
    // should be a constexpr template predicate that indicates whether
    // current classification allows for flush, straight, four of a kind, etc
    // xoptx also the community cards determine whether flush, full house can
    // happen
    RARE(p1Flushes) {
        RARE(p2Flushes) {
            // p2 also has a flush, there is the need to check for either being
            // a straight flush
            auto p1FlushSuit = p1Flushes.best();
            auto p1Flush = p1.m_bySuit.at(p1FlushSuit);
            auto p1Straights = straights(p1Flush);
            auto p2FlushSuit = p2Flushes.best();
            auto p2Flush = p2.m_bySuit.at(p2FlushSuit);
            auto p2Straights = straights(p2Flush);
            RARE(p1Straights) { // p1 is straight flush
                // note: never needed to calculate number repetitions!
                RARE(p2Straights) { // both are
                    return bestStraight(p1Straights, p2Straights);
                }
                return 1;
            }
            RARE(p2Straights) { // p2 is straight flush
                // note: never needed to calculate numbers!
                return -1;
            }
            // Both are flush but none straight flush
            // xoptx How can any be full house or four of a kind?
            // xoptx What is the optimal sequence of checks?
            auto p1NumberCounts = p1.rankCounts();
            auto p2NumberCounts = p2.rankCounts();
            auto fullHouseOrFourOfAKind =
                winnerPotentialFullHouseOrFourOfAKind(
                    p1NumberCounts, p2NumberCounts
                );
            RARE(fullHouseOrFourOfAKind.decided) {
                return fullHouseOrFourOfAKind.ifDecided;
            }
            // flushes, but not straight-flush, nor full-house nor four of a kind
            return bestFlush(p1Flush, p2Flush);
        }
        // p2 is not a flush and p1 is a flush
        // xoptx this check seems premature, only if p2 is a full house is justified
        auto p1s = p1.rankCounts();
        auto p2s = p2.rankCounts();
        auto fullHouseOrFourOfAKind =
            winnerPotentialFullHouseOrFourOfAKind(p1s, p2s);
        RARE(fullHouseOrFourOfAKind.decided) {
            return fullHouseOrFourOfAKind.ifDecided;
        }
        // p2 is not a full house nor a four of a kind
        return 1;
    }
    // p1 is no flush
    auto p1s = p1.rankCounts();
    auto p2s = p2.rankCounts();
    RARE(p2Flushes) {
        auto p2FlushSuit = p2Flushes.best();
        auto p2Flush = p2.m_bySuit.at(p2FlushSuit);
        auto p2Straights = straights(p2Flush);
        RARE(p2Straights) { return -1; } // p2 is straight flush
        auto fullHouseOrFourOfAKind =
            winnerPotentialFullHouseOrFourOfAKind(p1s, p2s);
        RARE(fullHouseOrFourOfAKind.decided) {
            return fullHouseOrFourOfAKind.ifDecided;
        }
        return -1;
    }
    // No flushes
    auto p1Set = p1.rankSet();
    auto p1Straights = straights(p1Set);
    auto p2Set = p2.rankSet();
    auto p2Straights = straights(p2Set);
    RARE(p1Straights | p2Straights) {
        auto fhofoak = winnerPotentialFullHouseOrFourOfAKind(p1s, p2s);
        RARE(fhofoak.decided) { return fhofoak.ifDecided; }
        if(p1Straights) {
            if(p2Straights) {
                return positiveIndex1Better(p1Straights, p2Straights);
            }
            return 1;
        }
        return -1;
    }
    auto p1Pairs = p1s.greaterEqual<2>();
    auto p2Pairs = p2s.greaterEqual<2>();
    RARE(!p1Pairs) {
        RARE(!p2Pairs) {
            return bestFlush(p1.rankSet(), p2.rankSet());
        }
        return -1;
    }
    RARE(!p2Pairs) { return 1; }
    auto p1toaks = p1s.greaterEqual<3>();
    auto p2toaks = p2s.greaterEqual<3>();
    RARE(p1toaks) {
        RARE(p2toaks) {
            auto potentialFull =
                winnerPotentialFullHouseGivenThreeOfAKind(
                    p1s, p2s, p1toaks, p2toaks
                );
            RARE(potentialFull.decided) { return potentialFull.ifDecided; }
            // no full house or better
            auto
                p1Toak = p1toaks.best(),
                p2Toak = p2toaks.best();
            auto diff = positiveIndex1Better(p1Toak, p2Toak);
            return diff;
        }
        return 1;
    }
    // double pairs and pairs
    return 0;
}

inline int bestFullHouse(FullHouseResult p1, FullHouseResult p2) {
    return
        positiveIndex1Better(
            p1.bestPair + (p1.bestThreeOfAKind << 4),
            p2.bestPair + (p2.bestThreeOfAKind << 4)
        );
}

inline int winner(CSet community, CSet p1, CSet p2) {
    // There are three independent criteria
    // n-of-a-kind, straights and flushes
    // since flushes dominate straigts, and have in texas holdem about the
    // same probability than straights, the first inconditional check is
    // for flushes.
    // xoptx all the comparison operations should not be unary but binary
    // in the player cards and community, this would allow calculating the
    // community cards only once.
    p1 = p1 | community;
    auto p1Suits = p1.suitCounts();
    auto p1Flushes = flushes(p1Suits);
    p2 = p2 | community ;
    auto p2Suits = p2.suitCounts();
    auto p2Flushes = flushes(p2Suits);

    auto p1ranks = p1.rankCounts();
    auto p2ranks = p2.rankCounts();

    auto p1toaks = noaks<3>(p1ranks);
    auto p2toaks = noaks<3>(p2ranks);

    //auto p1str = straights(p1.rankSet());
    //auto p2str = straights(p2.rankSet());

    RARE(p1toaks) {
        RARE(p2toaks) {
            auto p1foaks = noaks<4>(p1ranks);
            auto p2foaks = noaks<4>(p2ranks);
            RARE(p1foaks) {
                static_assert(TotalHand - 3 < 5, "Four of a kind does not imply there is no straight");
                RARE(p2foaks) {
                    return bestFourOfAKind(p1ranks, p2ranks, p1foaks, p2foaks);
                }
                RARE(straightFlush(p2, p2Flushes)) { return -1; }
                return 1;
            }
            RARE(p2foaks) {
                RARE(straightFlush(p1, p2Flushes)) { return 1; }
                return -1;
            }
            // no four of a kind
            auto p1fh = isFullHouse(p1ranks);
            RARE(p1fh.isFullHouse) {
                static_assert(TotalHand < 8, "There can be full house and straight flush");
                auto p2fh = isFullHouse(p2ranks);
                RARE(p2fh.isFullHouse) { return bestFullHouse(p1fh, p2fh); }
                RARE(straightFlush(p2, p2Flushes)) { return -1; }
                return 1;
            }
            // no four of a kind, no full house either hand
            RARE(p1Flushes) {
            }
        }
    }
    return 0;
}

}
