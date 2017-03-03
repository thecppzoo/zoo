#pragma once

namespace ep {

// 1100
// 1010
// 0110
// 1001
// 0101
// 0011
constexpr uint64_t nextSubset(uint64_t v) {
    constexpr auto allSet = ~uint64_t(0);
    constexpr auto one = uint64_t(1);

    // find first available element to include
    auto trailingZero = __builtin_ctzll(v);
    auto withoutTrailingZero = (v >> trailingZero);
    auto inverted = ~withoutTrailingZero;
    auto onesCount = __builtin_ctzll(inverted);
    auto firstAvailable = trailingZero + onesCount;
    // leave onesCount - 1 in the least significant parts of the result
    // and one bit set at firstAvailable, the rest is the same
    auto leastOnes = ~(allSet << (onesCount - 1));
    leastOnes |= (one << firstAvailable);
    auto mask = allSet << firstAvailable;
    return (v & mask) | leastOnes;
}

static_assert(2 == nextSubset(1), "");
static_assert(8 == nextSubset(4), "");
static_assert(9 == nextSubset(6), "");
// 0001. 1010 == 0x58
// 1000. 0110 == 0x61
static_assert(0x61 == nextSubset(0x58), "");

}