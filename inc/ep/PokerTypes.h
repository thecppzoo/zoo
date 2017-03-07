#pragma once

#include "core/SWAR.h"

namespace ep {

/// Number of numbers in the card set
constexpr auto NRanks = 13;
/// Number of suits
constexpr auto NSuits = 4;

namespace canonical {

constexpr auto NRanks = 13;
constexpr auto NSuits = 4;

};

constexpr auto SuitSize = 1 << core::metaLogCeiling(NRanks);
constexpr auto RankSize = 1 << core::metaLogCeiling(NSuits);

using SWARSuit = core::SWAR<SuitSize>;
using SWARRank = core::SWAR<RankSize>;

inline SWARSuit convert(SWARRank r) {
    uint64_t rv = 0;
    auto input = r.value();
    while(input) {
        auto low = __builtin_ctzll(input);
        auto suit = (low & 0x3) << 4;
        auto rank = low >> 2;
        rv |= uint64_t(1) << (rank + suit);
        input &= (input - 1);
    }
    return SWARSuit(rv);
}

constexpr auto NCommunity = 5;
constexpr auto NPlayer = 2;
constexpr auto TotalHand = NCommunity + NPlayer;

namespace abbreviations {

enum Ranks {
    r2 = 0, r3, r4, r5, r6, r7, r8, r9, rT, rJ, rQ, rK, rA
};

inline int operator|(Ranks r1, Ranks r2) {
    return (1 << r1) | (1 << r2);
}

inline int operator|(int rv, Ranks rank) {
    return rv | (1 << rank);
}

inline int operator|=(int &rv, Ranks rank) {
    return rv |= (1 << rank);
}

enum Suits {
    sS = 0, sH, sD, sC
};

enum Cards: uint64_t {
    s2 = uint64_t(1) << 0,
    h2 = uint64_t(1) << 1,
    d2 = uint64_t(1) << 2,
    c2 = uint64_t(1) << 3,
    s3 = uint64_t(1) << 4,
    h3 = uint64_t(1) << 5,
    d3 = uint64_t(1) << 6,
    c3 = uint64_t(1) << 7,
    s4 = uint64_t(1) << 8,
    h4 = uint64_t(1) << 9,
    d4 = uint64_t(1) << 10,
    c4 = uint64_t(1) << 11,
    s5 = uint64_t(1) << 12,
    h5 = uint64_t(1) << 13,
    d5 = uint64_t(1) << 14,
    c5 = uint64_t(1) << 15,
    s6 = uint64_t(1) << 16,
    h6 = uint64_t(1) << 17,
    d6 = uint64_t(1) << 18,
    c6 = uint64_t(1) << 19,
    s7 = uint64_t(1) << 20,
    h7 = uint64_t(1) << 21,
    d7 = uint64_t(1) << 22,
    c7 = uint64_t(1) << 23,
    s8 = uint64_t(1) << 24,
    h8 = uint64_t(1) << 25,
    d8 = uint64_t(1) << 26,
    c8 = uint64_t(1) << 27,
    s9 = uint64_t(1) << 28,
    h9 = uint64_t(1) << 29,
    d9 = uint64_t(1) << 30,
    c9 = uint64_t(1) << 31,
    sT = uint64_t(1) << 32,
    hT = uint64_t(1) << 33,
    dT = uint64_t(1) << 34,
    cT = uint64_t(1) << 35,
    sJ = uint64_t(1) << 36,
    hJ = uint64_t(1) << 37,
    dJ = uint64_t(1) << 38,
    cJ = uint64_t(1) << 39,
    sQ = uint64_t(1) << 40,
    hQ = uint64_t(1) << 41,
    dQ = uint64_t(1) << 42,
    cQ = uint64_t(1) << 43,
    sK = uint64_t(1) << 44,
    hK = uint64_t(1) << 45,
    dK = uint64_t(1) << 46,
    cK = uint64_t(1) << 47,
    sA = uint64_t(1) << 48,
    hA = uint64_t(1) << 49,
    dA = uint64_t(1) << 50,
    cA = uint64_t(1) << 51,
};

}

}
