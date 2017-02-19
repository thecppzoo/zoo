#include <boost/math/special_functions/binomial.hpp>

#include <stdint.h>
#include <type_traits>

namespace ep {

constexpr auto NNumbers = 13;
constexpr auto NSuits = 4;

template<int N, int K> struct PartialPermutations {
    constexpr static auto value = N * PartialPermutations<N - 1, K - 1>::value;
};
template<int N> struct PartialPermutations<N, 0> {
    constexpr static auto value = 1;
};

static_assert(12 == PartialPermutations<4, 2>::value, "");

template<int N, int K, bool> struct Choose_impl {
    constexpr static uint64_t value = 0;
};
template<int N, int K> struct Choose_impl<N, K, true> {
    constexpr static auto value =
        Choose_impl<N - 1, K - 1, K <= N>::value +
        Choose_impl<N - 1, K, K <= N>::value;
};
template<int N> struct Choose_impl<N, 0, true> {
    constexpr static uint64_t value = 1;
};

template<int N, int K>
using Choose =
    Choose_impl<N, K, K <= N>;

static_assert(2 == Choose<2, 1>::value, "");
static_assert(6 == Choose<4, 2>::value, "");
static_assert(286 == Choose<13, 3>::value, "");
static_assert(1326 == Choose<52, 2>::value, "");

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

template<typename T, int Progression, int Remaining> struct BitmaskMaker {
    constexpr static T repeat(T v) {
        return
            BitmaskMaker<T, Progression, Remaining - 1>::repeat(v | (v << Progression));
    }
};
template<typename T, int P> struct BitmaskMaker<T, P, 0> {
    constexpr static T repeat(T v) { return v; }
};

template<int size, typename T>
constexpr T makeBitmask(T v) {
    return BitmaskMaker<T, size, sizeof(T)*8/size>::repeat(v);
}

namespace detail {
    constexpr auto b0 = makeBitmask<2>(1ull);
    constexpr auto b1 = makeBitmask<4>(3ull);
    constexpr auto b2 = makeBitmask<8>(0xFull);
    constexpr auto b3 = makeBitmask<16>(0xFFull);
    constexpr auto multiplierNibble = makeBitmask<4>(1ull);
}

constexpr uint64_t bibitPopCounts(uint64_t v) {
    return v - ((v >> 1) & detail::b0);
}

constexpr uint64_t nibblePopCounts(uint64_t c) {   
    return ((c >> 2) & b1) + (c & detail::b1);
}

constexpr uint64_t bytePopCounts(uint64_t v) {
    return ((v >> 4) & detail::b2) + (v & detail::b2);
}

constexpr uint64_t popCounts16bit(uint64_t v) {
    return ((v >> 8) & detail::b3) + (v & detail::b3);
}

auto what = nibblePopCounts(0xF754);

uint64_t popcounts(uint64_t v) { return nibblePopCounts(v); }

int main(int argc, char** argv) {
    return 0;
}

