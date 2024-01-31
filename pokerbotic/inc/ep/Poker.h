#pragma once

#include "core/SWAR.h"


#include "ep/PokerTypes.h"

namespace ep {


constexpr int64_t positiveIndex1Better(uint64_t index1, uint64_t index2) {
    return index1 - index2;
}

template<int Size, typename T>
struct Counted_impl {
    using NoConversion = int Counted_impl::*;
    constexpr static NoConversion doNotConvert = nullptr;

    Counted_impl() = default;
    constexpr explicit Counted_impl(core::SWAR<Size, T> bits):
        m_counts(
            core::popcount<core::metaLogCeiling(Size - 1) - 1>(bits.value())
        )
    {}
    constexpr Counted_impl(NoConversion, core::SWAR<Size, T> bits):
        m_counts(bits) {}

    constexpr Counted_impl clearAt(int index) {
        return Counted_impl(doNotConvert, m_counts.clear(index));
    }

    constexpr core::SWAR<Size, T> counts() { return m_counts; }

    template<int N>
    constexpr core::BooleanSWAR<Size, T> greaterEqual() {
        return core::greaterEqualSWAR<N>(m_counts);
    }

    constexpr operator bool() const { return m_counts.value(); }

    constexpr int best() { return m_counts.top(); }

    protected:
    core::SWAR<Size, T> m_counts;
};

using RankCounts = Counted_impl<RankSize, uint64_t>;
using SuitCounts = Counted_impl<SuitSize, uint64_t>;
using RanksPresent = core::BooleanSWAR<RankSize, uint64_t>;

struct CardSet {
    SWARRank m_ranks;

    CardSet() = default;
    constexpr CardSet(uint64_t cards): m_ranks(cards) {}

    constexpr RankCounts counts() { return RankCounts(m_ranks); }

    static unsigned ranks(uint64_t arg);/* {
        constexpr auto selector = ep::core::makeBitmask<4>(uint64_t(1));
        return __builtin_ia32_pext_di(arg, selector);
    }*/

    static unsigned ranks(RankCounts rc);/* {
        constexpr auto selector = ep::core::makeBitmask<4>(uint64_t(8));
        auto present = rc.greaterEqual<1>();
        return __builtin_ia32_pext_di(present.value(), selector);
    }*/

    constexpr uint64_t cards() { return m_ranks.value(); }
};

inline CardSet operator|(CardSet l, CardSet r) {
    return l.m_ranks.value() | r.m_ranks.value();
}

inline uint64_t flush(uint64_t arg) {
    constexpr auto flushMask = ep::core::makeBitmask<4, uint64_t>(1);
    for(auto n = 4; n--; ) {
        auto filtered = arg & flushMask;
        auto count = __builtin_popcountll(filtered);
        if(5 <= count) { return filtered; }
        arg >>= 1;
    }
    return 0;
}

#define RARE(v) if(__builtin_expect(bool(v), false))
#define LIKELY_NOT(v) if(__builtin_expect(!bool(v), true))

inline SWARRank straights_rankRepresentation(RanksPresent present) {
    auto acep = 
        present.at(ep::abbreviations::rA) ?
            uint64_t(0xF) << ep::abbreviations::rA :
            uint64_t(0);
    auto ranks = present.value();
    auto sk = ranks << 4;
    auto rak = ranks & sk;
    auto rt = acep | (ranks << 16);
    auto rqj = rak << 8;
    auto rakqj = rak & rqj;
    return SWARRank(rakqj & rt);
}

enum BestHand {
    HIGH_CARDS = 0, PAIR, TWO_PAIRS, THREE_OF_A_KIND, STRAIGHT, FLUSH,
    FULL_HOUSE, FOUR_OF_A_KIND, STRAIGHT_FLUSH
};

union HandRank {
    int32_t code;
    struct {
        unsigned
            low: 14,
            high: 14,
            hand: 4;
    };

    constexpr HandRank(): code(0) {}

    HandRank(BestHand h, int high, int low):
        hand(h), high(high), low(low)
    {}

    constexpr bool isSet() { return 0 != code; }

    constexpr bool operator==(HandRank hr) {
        return code == hr.code; }
};

inline unsigned toRanks(uint64_t ranks) {
    constexpr auto selector = ep::core::makeBitmask<4>(uint64_t(1));
    return __builtin_ia32_pext_di(ranks, selector);
}

inline unsigned toRanks(RanksPresent rp) {
    constexpr auto selector = ep::core::makeBitmask<4>(uint64_t(8));
    return __builtin_ia32_pext_di(rp.value(), selector);
}

inline uint64_t flush(void *, uint64_t cards) { return flush(cards); }

namespace {
    struct Colored { int shift; };
    struct ColorBlind {};
};

inline uint64_t flush(Colored *c, uint64_t cards);
inline uint64_t flush(ColorBlind *c, uint64_t cards);

/// \note benchmarks seem to indicate this is a 0.5% time pessimization!
template<typename T>
inline bool mayStraight(T *p, CardSet hand) {
    static_assert(13 == NRanks, "");
    constexpr auto fivesOrTens = uint64_t(0xF0000F000);
    return hand.cards() & fivesOrTens;
}

template<typename T>
inline HandRank handRank(T *p, CardSet hand) {
    auto mayS = mayStraight(p, hand);
    RankCounts rankCounts{SWARRank(hand.cards())};
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
        auto kicker1 = without.best();
        auto without2 = without.clearAt(kicker1);
        auto kicker2 = without2.best();
        auto low = (1 << kicker1) | (1 << kicker2);
        rv = HandRank(THREE_OF_A_KIND, high, low);
    }
    static_assert(TotalHand < 2*5, "No two flushes");
    auto flushCards = flush(p, hand.cards());
    RARE(flushCards) {
        auto tmp = RanksPresent(flushCards);
        auto straightFlush =
            mayS ?
                straights_rankRepresentation(tmp) :
                SWARRank(0);
        RARE(straightFlush) {
            return { STRAIGHT_FLUSH, 0, 1 << straightFlush.top() };
        }
        auto cards = flushCards;
        for(auto count = __builtin_popcountll(cards); 5 < count--; ) {
            cards &= (cards - 1); // turns off lsb
        }
        auto ranks = toRanks(cards);
        return { FLUSH, 0, int(ranks) };
    }
    auto ranks = rankCounts.greaterEqual<1>();
    auto str =
        mayS ?
            straights_rankRepresentation(ranks) :
            SWARRank(0);
    RARE(str) { return { STRAIGHT, 0, 1 << str.top() }; }
    RARE(rv.isSet()) { return rv; }
    auto pairs = rankCounts.greaterEqual<2>();
    if(pairs) {
        auto top = pairs.top();
        auto high = (1 << top);
        auto bottomPairs = pairs.clear(top);
        auto without = ranks.clear(top);
        if(bottomPairs) {
            auto bottom = bottomPairs.top();
            auto without2 = without.clear(bottom);
            return { TWO_PAIRS, high | (1 << bottom), 1 << without2.top() };
        }
        auto valueRanks = toRanks(without);
        // Objective: leave in valueRanks three kickers
        // Given: TotalHand - 2 ranks
        // TotalHand - 1 - 1 ranks - count = 3 <=> count = TotalHand - 5;
        for(auto count = TotalHand - 5; count--; ) {
            valueRanks &= valueRanks - 1;
        }
        return { PAIR, high, int(valueRanks) };
    }
    auto valueRanks = toRanks(ranks);
    for(auto count = TotalHand - 5; count--; ) {
        valueRanks &= valueRanks - 1;
    }
    return { HIGH_CARDS, 0, int(valueRanks) };
}

inline HandRank handRank(CardSet hand) { return handRank<void>(nullptr, hand); }

inline int compare(CardSet community, CardSet p1, CardSet p2) {
    auto rank1 = handRank(community | p1);
    auto rank2 = handRank(community | p2);
    return rank1.code - rank2.code;
}

}
