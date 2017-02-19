#include "ep/metaBinomial.h"
#include "ep/core/Swar.h"
#include "ep/core/metaLog.h"

#include <boost/math/special_functions/binomial.hpp>

namespace ep {

/// Number of numbers in the card set
constexpr auto NNumbers = 13;
/// Number of suits
constexpr auto NSuits = 4;

constexpr auto SuitBits = 1 << core::metaLogCeiling(NNumbers);
static_assert(16 == SuitBits, "");
constexpr auto NumberBits = 1 << core::metaLogCeiling(NSuits);
static_assert(4 == NumberBits, "");

template<int NBits> struct IntegralWide_impl;
template<> struct IntegralWide_impl<16> { using type = uint16_t; };
template<> struct IntegralWide_impl<4> { using type = uint8_t; };

template<int NBits>
using IntegralWide = typename IntegralWide_impl<NBits>::type;

using WholeSuit = IntegralWide<SuitBits>;
using AllNumbers = IntegralWide<NumberBits>;

struct MonotoneFlop {
    constexpr static auto equivalents = NSuits;
    constexpr static auto element_count = Choose<NNumbers, 3>::value;
    constexpr static auto max_repetitions = 0;
};

/// \todo There are several subcategories here:
/// The card with the non-repeated suit is highest, pairs high, middle, pairs low, lowest
struct TwoToneFlop {
    constexpr static auto equivalents = PartialPermutations<NSuits, 2>::value;
    constexpr static auto element_count =
        NNumbers * Choose<NNumbers, 2>::value;
    constexpr static auto max_repetitions = 1;
};

struct RainbowFlop {
    constexpr static auto equivalents = Choose<NSuits, 3>::value;
    constexpr static auto element_count = NNumbers*NNumbers*NNumbers;
    constexpr static auto max_repetitions = 2;
};

template<typename...> struct Count {
    constexpr static uint64_t value = 0;
};
template<typename H, typename... Tail>
struct Count<H, Tail...> {
    constexpr static uint64_t value =
        H::equivalents*H::element_count + Count<Tail...>::value;
};

static_assert(4 * Choose<13, 3>::value == Count<MonotoneFlop>::value, "");
static_assert(12*13*Choose<NNumbers, 2>::value == Count<TwoToneFlop>::value, "");
static_assert(4*13*13*13 == Count<RainbowFlop>::value, "");
static_assert(
    Choose<52, 3>::value == Count<MonotoneFlop, TwoToneFlop, RainbowFlop>::value,
    ""
);

}


#include <stdint.h>
#include <array>

constexpr std::array<uint16_t, 4> toArray(uint64_t v) {
    using us = uint16_t;
    return {
        us(v & 0xFFFF), us((v >> 16) & 0xFFFF),
        us((v >> 32) & 0xFFFF), us((v >> 48) & 0xFFFF)
    };
}

namespace ep {

union CSet {
    uint64_t whole;
    std::array<uint16_t, 4> suits;

    constexpr CSet(): whole(0) {}
    constexpr CSet(uint64_t v): whole(v) {}
    constexpr CSet(uint64_t v, void *): suits(toArray(v)) {}
};

constexpr uint64_t numberCounts(uint64_t arg) {
    return core::popcount<core::metaLogCeiling(NumberBits) - 1>(arg);
}

constexpr uint64_t flushCounts(uint64_t arg) {
    return core::popcount<core::metaLogCeiling(SuitBits) - 1>(arg);
}

struct FlushCounts {
    uint64_t whole;

    constexpr FlushCounts(uint64_t orig): whole(flushCounts(orig)) {}
    constexpr uint64_t counts() const noexcept { return whole; }
};

struct CardsMonotoneFlop: MonotoneFlop {
    WholeSuit numbers;
    AllNumbers suit;
    
    CardsMonotoneFlop(WholeSuit ws, AllNumbers su): numbers(ws), suit(su) {}
};

void classify(CSet cards) {
    FlushCounts fc(cards.whole);
    constexpr auto twoOrMoreSameSuitMask = ~core::makeBitmask<SuitBits>(3ull);
    static_assert(0x3000300030003ull == ~twoOrMoreSameSuitMask, "");
    auto twoOrMoreSameSuit = fc.whole & twoOrMoreSameSuitMask;
    //if()
}

}

int main(int argc, char** argv) {
    return 0;
}

