#pragma once

#include "PokerTypes.h"
#include "ep/core/metaLog.h"

namespace ep {

constexpr int64_t positiveIndex1Better(uint64_t index1, uint64_t index2) {
    return index2 - index1;
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

    constexpr int best() {
        return m_counts.top();
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

// \note Again the Egyptian multiplication algorithm,
// including Ace-as-1 7 operations instead of the 10 in the naive method
inline unsigned straights(unsigned rv) {
    // xoptx is it better hasAce = (1 & rv) << (NRanks - 4) ?
    // xoptx what about rv |= ... ?
    constexpr auto acep = 1 << (NRanks - 1);
    auto hasAce = (acep & rv) ? (1 << 3) : 0;
    // 0 1 2 3 4 5 6 7 8 9 0 1 2
    // =========================
    // 2 3 4 5 6 7 8 9 T J Q K A
    // -------------------------
    // - 2 3 4 5 6 7 8 9 T J Q K   sK = rv << 1
    // -------------------------
    // 2 3 4 5 6 7 8 9 T J Q K A
    // - 2 3 4 5 6 7 8 9 T J Q K   rAK = rv & sK
    // -------------------------
    // - - - A 2 3 4 5 6 7 8 9 T   rT = hasAce | (rv << 4)
    // -------------------------
    // - - - 3 4 5 6 7 8 9 T J Q
    // - - - 2 3 4 5 6 7 8 9 T J   rQJ = rAK << 2
    // -------------------------
    // 2 3 4 5 6 7 8 9 T J Q K A
    // - 2 3 4 5 6 7 8 9 T J Q K
    // - - - 3 4 5 6 7 8 9 T J Q
    // - - - 2 3 4 5 6 7 8 9 T J   rAKQJ = rAK & rQJ
    // -------------------------
    auto sk = rv << 1;
    auto rak = rv & sk;
    auto rt = hasAce | (rv << 4);
    auto rqj = rak << 2;
    auto rakqj = rak & rqj;
    return rakqj & rt;
}

enum BestHand {
    HIGH_CARDS = 0, PAIR, TWO_PAIRS, THREE_OF_A_KIND, STRAIGHT, FLUSH,
    FULL_HOUSE, FOUR_OF_A_KIND, STRAIGHT_FLUSH
};

union HandRank {
    uint32_t code;
    struct {
        unsigned
            hand: 4,
            high: 14,
            low: 14;
    };
};

inline uint32_t handRankCode(BestHand h, uint16_t high, uint16_t low) {
    HandRank rv;
    rv.hand = h;
    rv.high = high;
    rv.low = low;
    return rv.code;
}

inline unsigned reverseBits(unsigned input) { return input; }

uint32_t handRank(CSet hand) {
    auto rankCounts = hand.rankCounts();
    auto suitCounts = hand.suitCounts();
    auto ranks = hand.rankSet();
    auto toaks = noaks<3>(rankCounts);
    uint32_t rv = 0;
    RARE(toaks) {
        auto foaks = noaks<4>(rankCounts);
        RARE(foaks) {
            static_assert(TotalHand < 8, "There can be four-of-a-kind and straight-flush");
            auto foak = foaks.best();
            auto without = rankCounts.clearAt(foak);
            return handRankCode(FOUR_OF_A_KIND, without.best(), 0);
        }
        auto toak = toaks.best();
        auto without = rankCounts.clearAt(toak);
        auto pairs = noaks<2>(without);
        RARE(pairs) {
            static_assert(TotalHand < 8, "There can be full-house and straight-flush");
            return handRankCode(FULL_HOUSE, toak, pairs.best());
        }
        ranks ^= (1 << toak);
        auto next = core::msb(ranks);
        auto low = 1 << next;
        ranks ^= low;
        low |= 1 << core::msb(ranks);
        rv = handRankCode(THREE_OF_A_KIND, toak, low);
    }
    auto flushes = suitCounts.greaterEqual<5>();
    RARE(flushes) {
        static_assert(TotalHand < 2*5, "No two flushes");
        auto suit = flushes.best();
        auto royal = hand.m_bySuit.at(suit);
        auto isIt = straights(royal);
        RARE(isIt) {
            return handRankCode(STRAIGHT_FLUSH, core::msb(isIt), 0);
        }
        for(auto count = suitCounts.counts().at(suit) - 5; count--; ) {
            royal &= (royal - 1); // turns off lsb
        }
        return handRankCode(FLUSH, royal, 0);
    }
    auto str = straights(ranks);
    RARE(str) {
        return handRankCode(STRAIGHT, core::msb(str), 0);
    }
    RARE(rv) { return rv; }
    auto pairs = rankCounts.greaterEqual<2>();
    if(pairs) {
        auto top = pairs.best();
        auto without = pairs.clearAt(top);
        if(without) {
            auto bottom = without.best();
            auto high = (1 << top) | (1 << bottom);
            ranks ^= high;
            return handRankCode(TWO_PAIRS, high, core::msb(ranks));
        }
        auto high = 1 << top;
        ranks ^= high;
        auto next1 = 1 << core::msb(ranks);
        ranks ^= next1;
        auto next2 = 1 << core::msb(ranks);
        ranks ^= next2;
        return handRankCode(PAIR, high, next1 | next2 | core::msb(ranks));
    }
    for(auto count = TotalHand - 5; count--; ) {
        ranks &= ranks - 1;
    }
    return handRankCode(HIGH_CARDS, ranks, 0);
}

}

#if 0
A  B  C  D  E  F  G  H  I  J  K  L  M  N  r0
-----------------------------------------
B  C  D  E  F  G  H  I  J  K  L  M  N  -  r0s1 = r0 << 1 1, 0
-----------------------------------------
A  B  C  D  E  F  G  H  I  J  K  L  M  N 
B  C  D  E  F  G  H  I  J  K  L  M  N  -  r1 = r0 & r0s1 1, 1
-----------------------------------------        
C  D  E  F  G  H  I  J  K  L  M  N  -  -
D  E  F  G  H  I  J  K  L  M  N  -  -  -  r1s2 = r1 << 2 2, 1
-----------------------------------------
A  B  C  D  E  F  G  H  I  J  K  L  M  N
B  C  D  E  F  G  H  I  J  K  L  M  N  -
C  D  E  F  G  H  I  J  K  L  M  N  -  -
D  E  F  G  H  I  J  K  L  M  N  -  -  -  r2 = r1 & r1s2 2, 2
-----------------------------------------
E  F  G  H  I  J  K  L  M  N  -  -  -  -  r0s4 = r0 << 4 3, 2
-----------------------------------------
        result                            r2 & r0s4      3, 3
 
#endif
