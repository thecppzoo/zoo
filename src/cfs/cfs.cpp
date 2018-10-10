#include <array>

template<typename T, std::size_t S>
T *back_inserter(std::array<T, S> &a) {
    return a.data();
}

#ifndef ZOO_CFS_CACHE_FRIENDLY_SEARCH
#define ZOO_CFS_CACHE_FRIENDLY_SEARCH

#include <algorithm>

namespace zoo {

constexpr unsigned long logPlus(unsigned long long arg) {
    return 63 - __builtin_clzll(arg + 1);
}

/// Converts sorted container to cache-friendly search container
/// of the same type.
///
/// \param input is a sorted container with <
/// A sorted array implies a binary tree, in which the root
/// is the median element of the range below it.
/// The output is the bread-first traversal of this tree
/// Assuming an initial size equal to (2^n) - 1 elements, the
/// median element is at zero-based index 2^(n - 1) - 1.
/// Assuming the root is at level 0,
/// the first element of each level l is at zero-based index 2^(n - l - 1) - 1
/// The next element in the traversal for an element at level l and
/// index i is i + 2^(n - l)
/// For the general case the size of the input S is such that
/// (2^n) - 1 < S
/// Then the algorithm will convert for the first (2^n) - 1 elements
/// and recurr for the S - (2^n) + 1 remaining
///
/// []: nothing to do
/// [0]: copy
/// [0, 1]: s = 2, n = log+(2) = 2, 2^n - 1 = 3, s < 3, thus do 1 and 1, copy
/// [0, 1, 2]: s = 3, n = log+(3) = 2, 2^n - 1 = 3, s == 3: 1, 0, 2
/// [0, 1, 2, 3]: s = 4, n = log+(4) = 3, 2^n - 1 = 7, s < 7, do 3 and 1
/// [0, 1, 2, 3, 4]: do 3 and 2
/// [0 .. 5]: do 3 and 3
/// [0 .. 6]: do 7
/// [0 .. 7]: do 7 and 1
/// Firsts: 255, 127, 63, 31, 15, 7, 3, 1
/// 2^n - 1: [2^(n - 1) - 1] 1 [2^(n - 1) - 1]  
template<typename C, typename Inserter>
auto sortedToCacheFriendlySearch(
    Inserter output, std::size_t base, const C &input
) -> void {
    auto
        inputSize = input.size(),
        remaining = inputSize - base;
    if(remaining < 3) {
        std::copy(input.begin() + base, input.end(), output);
        return;
    }
    auto
        totalLog = logPlus(remaining),
        totalPowerOf2 = 1lu << totalLog,
        nLevels = remaining < totalPowerOf2 - 1 ? totalLog - 1 : totalLog,
        straddle = 1lu << nLevels,
        size = straddle - 1,
        level = 1lu;
    for(;;) {
        auto
            nextStraddle = straddle >> 1,
            index = nextStraddle - 1;
        do {
            *output++ = input[base + index];
            index += straddle;
        } while(index < size);
        if(nextStraddle < 2) { break; }
        straddle = nextStraddle;
        ++level;
    }
    sortedToCacheFriendlySearch(output, base + size, input);
}

template<typename Container>
inline auto reserveIfAvailable(Container &c, std::size_t s) ->
decltype(c.reserve(s), (void)0) {
    c.reserve(s);
}
inline void reserveIfAvailable(...) {}

template<typename Sorted>
Sorted sortedToCacheFriendlySearch(const Sorted &s) {
    Sorted rv;
    reserveIfAvailable(rv, s.size());
    sortedToCacheFriendlySearch(back_inserter(rv), 0, s);
    return rv;
}

}

#endif

#include <ostream>

namespace std {
template<typename T, std::size_t L>
std::ostream &operator<<(std::ostream &out, const std::array<T, L> &a) {
    out << '(';
    for(auto current{cbegin(a)}, sentry{cend(a)};;) {
        out << *current++;
        if(sentry == current) { break; }
        out << ", ";
    }
    return out << ')';
}
}

#include <catch.hpp>

TEST_CASE("sorting", "[array]") {
    auto stcf = [](const auto &c) {
        return zoo::sortedToCacheFriendlySearch(c);
    };
    auto empty = std::array<int, 0>{};
    REQUIRE(stcf(empty) == empty);
    auto single = std::array<int, 1>{0};
    REQUIRE(stcf(single) == single);
    auto bi = std::array<int, 2>{0, 1};
    REQUIRE(stcf(bi) == bi);
    REQUIRE(stcf(std::array<int, 3>{0, 1, 2}) == std::array<int, 3>{1, 0, 2});
}
