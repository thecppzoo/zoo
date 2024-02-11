#pragma once

#include "ep/Poker.h"

namespace ep {

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

    constexpr SuitCounts suitCounts() { return SuitCounts(m_bySuit); }
    constexpr RankCounts rankCounts() { return RankCounts(m_byRank); }

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

constexpr core::BooleanSWAR<16, uint64_t> flushes(SuitCounts  ss) {
    return ss.greaterEqual<5>();
}

// \note Again the Egyptian multiplication algorithm,
// including Ace-as-1 7 operations instead of the 10 in the naive method
inline unsigned straights(unsigned ranks) {
    // xoptx is it better hasAce = (1 & rv) << (NRanks - 4) ?
    // xoptx what about rv |= ... ?
    constexpr auto acep = 1 << (NRanks - 1);
    auto hasAce = (acep & ranks) ? (1 << 3) : 0;
    // 0 1 2 3 4 5 6 7 8 9 0 1 2
    // =========================
    // 2 3 4 5 6 7 8 9 T J Q K A
    // -------------------------
    // - 2 3 4 5 6 7 8 9 T J Q K   sK = ranks << 1
    // -------------------------
    // 2 3 4 5 6 7 8 9 T J Q K A
    // - 2 3 4 5 6 7 8 9 T J Q K   rAK = ranks & sK
    // -------------------------
    // - - - A 2 3 4 5 6 7 8 9 T   rT = hasAce | (ranks << 4)
    // -------------------------
    // - - - 3 4 5 6 7 8 9 T J Q
    // - - - 2 3 4 5 6 7 8 9 T J   rQJ = rAK << 2
    // -------------------------
    // 2 3 4 5 6 7 8 9 T J Q K A
    // - 2 3 4 5 6 7 8 9 T J Q K
    // - - - 3 4 5 6 7 8 9 T J Q
    // - - - 2 3 4 5 6 7 8 9 T J   rAKQJ = rAK & rQJ
    // -------------------------
    auto sk = ranks << 1;
    auto rak = ranks & sk;
    auto rt = hasAce | (ranks << 4);
    auto rqj = rak << 2;
    auto rakqj = rak & rqj;
    return rakqj & rt;
}

inline HandRank handRank(CSet hand) {
    auto rankCounts = hand.rankCounts();
    auto suitCounts = hand.suitCounts();
    auto ranks = hand.rankSet();
    auto toaks = rankCounts.greaterEqual<3>();
    HandRank rv;
    RARE(toaks) {
        auto foaks = rankCounts.greaterEqual<4>();
        RARE(foaks) {
            static_assert(TotalHand < 8, "There can be four-of-a-kind and straight-flush");
            auto foak = foaks.top();
            auto without = rankCounts.clearAt(foak);
            return { FOUR_OF_A_KIND, 1 << foak, 1 << without.best() };
        }
        auto toak = toaks.top();
        auto without = rankCounts.clearAt(toak);
        auto pairs = without.greaterEqual<2>();
        RARE(pairs) {
            static_assert(TotalHand < 8, "There can be full-house and straight-flush");
            auto pair = pairs.top();
            return { FULL_HOUSE, (1 << toak), (1 << pair) };
        }
        auto high = 1 << toak;
        ranks ^= high;
        auto next = core::msb(ranks);
        auto lowTwo = 1 << next;
        ranks ^= lowTwo;
        lowTwo |= 1 << core::msb(ranks);
        rv = HandRank(THREE_OF_A_KIND, high, lowTwo);
    }
    auto flushes = suitCounts.greaterEqual<5>();
    RARE(flushes) {
        static_assert(TotalHand < 2*5, "No two flushes");
        auto suit = flushes.top();
        auto royal = hand.m_bySuit.at(suit);
        auto isIt = straights(royal);
        RARE(isIt) {
            return { STRAIGHT_FLUSH, 1 << core::msb(isIt), 0 };
        }
        for(auto count = suitCounts.counts().at(suit) - 5; count--; ) {
            royal &= (royal - 1); // turns off lsb
        }
        return { FLUSH, int(royal), 0 };
    }
    auto str = straights(ranks);
    RARE(str) { return { STRAIGHT, 1 << core::msb(str), 0 }; }
    RARE(rv.isSet()) { return rv; }
    auto pairs = rankCounts.greaterEqual<2>();
    if(pairs) {
        auto top = pairs.top();
        auto without = pairs.clear(top);
        if(without) {
            auto bottom = without.top();
            auto high = (1 << top) | (1 << bottom);
            ranks ^= high;
            return { TWO_PAIRS, high, 1 << core::msb(ranks) };
        }
        auto high = 1 << top;
        ranks ^= high;
        auto next1 = 1 << core::msb(ranks);
        ranks ^= next1;
        auto next2 = 1 << core::msb(ranks);
        ranks ^= next2;
        auto next3 = 1 << core::msb(ranks);
        return { PAIR, high, next1 | next2 | next3 };
    }
    for(auto count = TotalHand - 5; count--; ) {
        ranks &= ranks - 1;
    }
    return { HIGH_CARDS, int(ranks), 0 };
}

}
