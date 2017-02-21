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

using SWARSuit = core::SWAR<SuitBits>;
using SWARNumber = core::SWAR<NumberBits>;

template<int Size, typename T>
struct Counted {
    Counted() = default;
    constexpr Counted(core::SWAR<Size, T> bits):
        m_counts(
            core::popcount<core::metaLogCeiling(Size - 1) - 1>(bits.value())
        )
    {}

    constexpr core::SWAR<Size, T> counts() { return m_counts; }

    template<int N>
    constexpr Counted greaterEqual() {
        return core::greaterEqualSWAR<N>(m_counts);
    }
    constexpr operator bool() { return m_counts.value(); }

    protected:
    core::SWAR<Size, T> m_counts;
};

template<int Size, typename T>
constexpr Counted<Size, T>
makeCounted(core::SWAR<Size, T> bits) { return bits; }

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

struct CSet {
    SWARSuit m_bySuit;
    SWARNumber m_byNumber;

    constexpr CSet operator|(CSet o) {
        return { m_bySuit | o.m_bySuit, m_byNumber | m_byNumber };
    }

    constexpr auto suitCounts() { return makeCounted(m_bySuit); }
    constexpr auto numberCounts() { return makeCounted(m_byNumber); }
};

constexpr Counted<SuitBits, uint64_t> flushes(Counted<SuitBits, uint64_t>  ss) {
    return ss.greaterEqual<5>();
}

/// \todo optimize this
int winner(CSet community, CSet p1, CSet p2) {
    p1 = p1 | community;
    p2 = p2 | community;
    auto p1Suits = makeCounted(p1.m_bySuit);
    auto p1Flushes = flushes(p1Suits);
    if(p1Flushes) {
        auto p2Suits = p2.suitCounts();
        auto p2Flushes = flushes(p2Suits);
        if(p2Flushes) {
            // p2 loses except p2 is full house or four of a kind
            auto p2Repetitions = makeCounted(p2.m_byNumber);
            auto p2threeOfAKinds = p2Repetitions.greaterEqual<3>();
            if(!p2threeOfAKinds) { return 1; }
        }
    }
    return -1;
}

}

int main(int argc, char** argv) {
    return 0;
}

