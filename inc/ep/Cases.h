#pragma once

#include "ep/Poker.h"
#include "ep/core/deposit.h"
#include "ep/nextSubset.h"
#include "PokerTypes.h"
#include "ep/metaBinomial.h"

namespace ep {

struct SuitedPocket {
    using U = uint64_t;

    constexpr static auto equivalents = NSuits;
    constexpr static auto count = Choose<NRanks, 2>::value;
    // Exclude everything not spades
    constexpr static auto preselected = core::makeBitmask<RankSize, uint64_t>(0xE);
    // 2 and 3 of spades
    constexpr static uint64_t starting = 3;
    constexpr static auto end = uint64_t(1) << NRanks;
    constexpr static SWARSuit suits = SWARSuit(2);

    uint64_t m_current = starting;

    auto next() {
        auto rv = core::deposit(preselected, m_current);
        m_current = ep::nextSubset(m_current);
        return rv;
    }

    constexpr operator bool() const { return m_current < end; }
};

struct PocketPair {
    constexpr static auto equivalents = Choose<NSuits, 2>::value;
    constexpr static auto count = NRanks;

    constexpr static auto preselected = core::makeBitmask<RankSize, uint64_t>(0xE);
    constexpr static uint64_t starting = 1;
    constexpr static auto end = uint64_t(1) << NRanks*RankSize;
    constexpr static SWARSuit suits = SWARSuit((1 << SuitSize) | 1);

    uint64_t m_current = starting;

    auto next() {
        auto heart = m_current << 1;
        auto rv = m_current | heart;
        m_current <<= RankSize;
        return rv;
    }

    constexpr operator bool() const {
        return m_current < end;
    }
};

struct UnsuitedPocket {
    constexpr static auto equivalents = Choose<NSuits, 2>::value;
    constexpr static auto count = Choose<NRanks, 2>::value;
    constexpr static auto preselected = core::makeBitmask<RankSize, uint64_t>(0xE);
    // 2 and 3 of spades
    constexpr static uint64_t starting = 3;
    constexpr static auto end = uint64_t(1) << NRanks;
    constexpr static SWARSuit suits = SWARSuit((1 << SuitSize) | 1);

    uint64_t m_current = starting;

    auto next() {
        auto asRanks = core::deposit(preselected, m_current);
        auto makeLowRankHeart = (asRanks | (asRanks - 1)) + 1;
        m_current = nextSubset(m_current);
        return makeLowRankHeart;
    }

    constexpr operator bool() const {
        return m_current < end;
    }
};

template<typename Pocket>
struct Communities;

template<>
struct Communities<UnsuitedPocket> {
    using predecessor = UnsuitedPocket;
};

// Unsuited pocket continuations:
// 
}
