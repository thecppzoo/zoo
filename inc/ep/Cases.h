#pragma once

#include "ep/Poker.h"
#include "ep/core/deposit.h"
#include "ep/nextSubset.h"
#include "PokerTypes.h"

namespace ep {

struct ColoredPocket {
    using U = uint64_t;

    constexpr static auto equivalents = NSuits;
    // Exclude everything not spades
    constexpr static auto preselected = core::makeBitmask<RankSize, uint64_t>(0xE);
    // 2 and 3 of spades
    constexpr static uint64_t starting = 0x11;
    constexpr static auto end = uint64_t(1) << (NRanks + 1)*4;
    constexpr static uint64_t suits = 2;

    uint64_t m_current = starting;

    auto next() {
        m_current = ep::nextSubset(m_current);
        return core::deposit(preselected, m_current);
    }

    constexpr operator bool() const { return m_current < end; }
};

}