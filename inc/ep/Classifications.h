#pragma once

#include "ep/metaBinomial.h"

namespace ep {

struct MonotoneFlop {
    constexpr static auto equivalents = NSuits;
    constexpr static auto element_count = Choose<NRanks, 3>::value;
    constexpr static auto max_repetitions = 0;
};

/// \todo There are several subcategories here:
/// The card with the non-repeated suit is highest, pairs high, middle, pairs low, lowest
struct TwoToneFlop {
    constexpr static auto equivalents = PartialPermutations<NSuits, 2>::value;
    constexpr static auto element_count =
        NRanks * Choose<NRanks, 2>::value;
    constexpr static auto max_repetitions = 1;
};

struct RainbowFlop {
    constexpr static auto equivalents = Choose<NSuits, 3>::value;
    constexpr static auto element_count = NRanks*NRanks*NRanks;
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
static_assert(12*13*Choose<NRanks, 2>::value == Count<TwoToneFlop>::value, "");
static_assert(4*13*13*13 == Count<RainbowFlop>::value, "");
static_assert(
    Choose<52, 3>::value == Count<MonotoneFlop, TwoToneFlop, RainbowFlop>::value,
    ""
);

}
