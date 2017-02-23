#pragma once

#include "PokerTypes.h"
#include "ep/core/metaLog.h"

namespace ep {

constexpr int positiveIndex1Better(int index1, int index2) {
    return index2 - index1;
}

int bestBit(uint64_t val) {
    return __builtin_ctzll(val);
}

template<int Size, typename T>
struct Counted_impl {
    Counted_impl() = default;
    constexpr Counted_impl(core::SWAR<Size, T> bits):
        m_counts(
            core::popcount<core::metaLogCeiling(Size - 1) - 1>(bits.value())
        )
    {}

    constexpr core::SWAR<Size, T> counts() { return m_counts; }

    template<int N>
    constexpr Counted_impl greaterEqual() {
        return core::greaterEqualSWAR<N>(m_counts);
    }
    constexpr operator bool() { return m_counts.value(); }

    // \note Best is smaller number
    constexpr int bestIndex() {
        return bestBit(m_counts.value()) / Size;
    }

    constexpr Counted_impl clearAt(int index) { return m_counts.clear(index); }

    protected:
    core::SWAR<Size, T> m_counts;
};

using RankCounts = Counted_impl<RankSize, uint64_t>;
using SuitCounts = Counted_impl<SuitSize, uint64_t>;

struct CSet {
    SWARSuit m_bySuit;
    SWARRank m_byRank;

    constexpr CSet operator|(CSet o) {
        return { m_bySuit | o.m_bySuit, m_byRank | m_byRank };
    }

    constexpr CSet operator&(CSet o) {
        return { m_bySuit & o.m_bySuit, m_byRank & m_byRank };
    }

    constexpr CSet operator^(CSet o) {
        return { m_bySuit ^ o.m_bySuit, m_byRank ^ m_byRank };
    }

    constexpr SuitCounts suitCounts() { return {m_bySuit}; }
    constexpr RankCounts rankCounts() { return m_byRank; }

    constexpr static unsigned rankSet(uint64_t orig) {
        auto rv = orig;
        for(auto ndx = NSuits; --ndx;) {
            orig = orig >> SuitSize;
            rv |= orig;
        }
        constexpr auto isolateMask = (uint64_t(1) << NRanks) - 1;
        return rv & isolateMask;
    }

    constexpr unsigned rankSet() { return rankSet(m_bySuit.value()); }

    constexpr CSet include(int rank, int suit) {
        return { m_bySuit.set(suit, rank), m_byRank.set(rank, suit) };
    }
};

constexpr SuitCounts flushes(SuitCounts  ss) {
    return ss.greaterEqual<5>();
}

template<int N>
constexpr RankCounts noaks(RankCounts rs) {
    return rs.greaterEqual<N>();
}

#define RARE(v) if(__builtin_expect(bool(v), false))
#define LIKELY_NOT(v) if(__builtin_expect(!bool(v), true))

inline unsigned straights(unsigned rv) {
    // xoptx is it better hasAce = (1 & rv) << (NRanks - 4) ?
    // xoptx what about rv |= ... ?
    auto hasAce = (1 & rv) ? (1 << (NRanks - 4)) : 0;
    // 2 1 0 9 8 7 6 5 4 3 2 1 0 bit index
    // =========================
    // 2 3 4 5 6 7 8 9 T J Q K A
    // - 2 3 4 5 6 7 8 9 T J Q K &
    // - - 2 3 4 6 5 6 7 8 9 T J &
    // - - - 2 3 4 5 6 7 8 9 T J &
    // - - - A - - - - - - - - - |
    // - - - A 2 3 4 5 6 7 8 9 T &
    
    
    rv &= (rv >> 1);  // two
    rv &= (rv >> 1); // three in sequence
    rv &= (rv >> 1); // four in sequence
    rv |= hasAce;
    rv &= (rv >> 1);
    return rv;
}

struct ComparisonResult {
    int ifDecided;
    bool decided;
};

inline int bestKicker(
    RankCounts p1s, RankCounts p2s
) {
    return positiveIndex1Better(p1s.bestIndex(), p2s.bestIndex());
}

inline int bestFourOfAKind(
    RankCounts p1s, RankCounts p2s,
    RankCounts p1foaks, RankCounts p2foaks
) {
    auto p1BestNdx = p1foaks.bestIndex();
    auto p2BestNdx = p2foaks.bestIndex();
    auto diff = positiveIndex1Better(p1BestNdx, p2BestNdx);
    if(diff) { return diff; }
    return bestKicker(p1s.clearAt(p1BestNdx), p2s.clearAt(p2BestNdx));
}

inline int bestFlush(unsigned p1, unsigned p2) {
    for(auto count = 5; count--; ) {
        auto
            bestP1 = bestBit(p1),
            bestP2 = bestBit(p2);
        auto diff = positiveIndex1Better(bestP1, bestP2);
        if(diff) { return diff; }
    }
    return 0;
}

inline int bestStraight(unsigned s1, unsigned s2) {
    return positiveIndex1Better(bestBit(s1), bestBit(s2));
}

struct FullHouseResult {
    bool isFullHouse;
    int bestThreeOfAKind, bestPair;
};

inline FullHouseResult isFullHouse(RankCounts counts) {
    auto toaks = counts.greaterEqual<3>();
    RARE(toaks) {
        auto bestTOAK = toaks.bestIndex();
        auto withoutBestTOAK = counts.clearAt(bestTOAK);
        auto fullPairs = withoutBestTOAK.greaterEqual<2>();
        RARE(fullPairs) { return { true, bestTOAK, fullPairs.bestIndex() }; }
    }
    return { false, 0, 0 };
}

template<int N>
inline int bestKickers(
    RankCounts p1s, RankCounts p2s
) {
    for(auto counter = N;;) {
        auto p1Best = p1s.bestIndex();
        auto rv = positiveIndex1Better(p1Best, p2s.bestIndex());
        if(rv) { return rv; }
        if(!--counter) { break; }
        // xoptx
        p1s = p1s.clearAt(p1Best);
        p2s = p2s.clearAt(p1Best);
    }
    return 0;
}

inline unsigned straightFlush(CSet cards, SuitCounts ss) {
    static_assert(TotalHand < 2*5, "Assumes only one flush");
    auto ndx = ss.bestIndex();
    return straights(cards.m_bySuit.at(ndx));
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

    auto p1str = straights(p1.rankSet());
    auto p2str = straights(p2.rankSet());

    RARE(p1toaks) {
        RARE(p2toaks) {
            auto p1foaks = noaks<4>(p1ranks);
            auto p2foaks = noaks<4>(p2ranks);
            RARE(p1foaks) {
                static_assert(TotalHand - 3 < 5, "Four of a kind does not imply there is no straight");
                RARE(p2foaks) {
                    return bestFourOfAKind(p1ranks, p2ranks, p1foaks, p2foaks);
                }
                LIKELY_NOT(p2Flushes) {
                    return 1;
                }
                static_assert(TotalHand < 2*5, "There can be two flushes");
                auto royal = straights(p2.m_bySuit.at(p2Flushes.bestIndex()));
                RARE(royal) {
                    return -1;
                }
                return 1;
            }
            RARE(p2foaks) {}
        }
    }
}

}
