#include "ep/metaBinomial.h"
#include "ep/core/Swar.h"

#include <boost/math/special_functions/binomial.hpp>

namespace ep {

/// Number of numbers in the card set
constexpr auto NNumbers = 13;
/// Number of suits
constexpr auto NSuits = 4;

struct MonotoneFlop {
    constexpr static auto equivalents = NSuits;
    constexpr static auto element_count = Choose<NNumbers, 3>::value;
};

/// \todo There are several subcategories here:
/// The card with the non-repeated suit is highest, pairs high, middle, pairs low, lowest
struct TwoToneFlop {
    constexpr static auto equivalents = PartialPermutations<NSuits, 2>::value;
    constexpr static auto element_count =
        NNumbers * Choose<NNumbers, 2>::value;
};

struct RainbowFlop {
    constexpr static auto equivalents = Choose<NSuits, 3>::value;
    constexpr static auto element_count = NNumbers*NNumbers*NNumbers;
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

}

int main(int argc, char** argv) {
    return 0;
}

