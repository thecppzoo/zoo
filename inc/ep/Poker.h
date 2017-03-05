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
    

    constexpr core::SWAR<Size, T> counts() { return m_counts; }

    template<int N>
    constexpr Counted_impl greaterEqual() {
        return Counted_impl(doNotConvert, core::greaterEqualSWAR<N>(m_counts));
    }

    constexpr operator bool() const { return m_counts.value(); }

    constexpr int best() { return m_counts.top(); }

    constexpr Counted_impl clearAt(int index) {
        return Counted_impl(doNotConvert, m_counts.clear(index));
    }

    protected:
    core::SWAR<Size, T> m_counts;
};

using RankCounts = Counted_impl<RankSize, uint64_t>;
using SuitCounts = Counted_impl<SuitSize, uint64_t>;

struct CardSet {
    SWARRank m_ranks;

    CardSet() = default;
    constexpr CardSet(uint64_t cards): m_ranks(cards) {}

    constexpr RankCounts counts() { return RankCounts(m_ranks); }

    static unsigned ranks(uint64_t arg) {
        constexpr auto selector = ep::core::makeBitmask<4>(uint64_t(1));
        return __builtin_ia32_pext_di(arg, selector);
    }

    static unsigned ranks(RankCounts rc) {
        constexpr auto selector = ep::core::makeBitmask<4>(uint64_t(8));
        auto present = rc.greaterEqual<1>();
        return __builtin_ia32_pext_di(present.counts().value(), selector);
    }
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

}
