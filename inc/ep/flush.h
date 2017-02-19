#pragma once

#include <stdint.h>

namespace ep {

/// Number of numbers in the card set
constexpr auto NNumbers = 13;
/// Number of suits
constexpr auto NSuits = 4;

/// K-permutations of N elements
template<int N, int K> struct PartialPermutations {
    constexpr static auto value = N * PartialPermutations<N - 1, K - 1>::value;
};
template<int N> struct PartialPermutations<N, 0> {
    constexpr static auto value = 1;
};

/// Compilation time unit test
static_assert(12 == PartialPermutations<4, 2>::value, "");

/// Implementation of binomial coefficients
template<int N, int K, bool> struct Choose_impl {
    constexpr static uint64_t value = 0;
};
/// Uses the recurrence (n choose k) = (n - 1 choose k - 1) + (n - 1 choose k)
template<int N, int K> struct Choose_impl<N, K, true> {
    constexpr static auto value =
        Choose_impl<N - 1, K - 1, K <= N>::value +
        Choose_impl<N - 1, K, K <= N>::value;
};
template<int N> struct Choose_impl<N, 0, true> {
    constexpr static uint64_t value = 1;
};

/// Binomial coefficients
template<int N, int K>
using Choose =
    Choose_impl<N, K, K <= N>;

/// \section ChooseCTUT Compile time unit tests of binomial coefficient
static_assert(2 == Choose<2, 1>::value, "");
static_assert(6 == Choose<4, 2>::value, "");
static_assert(286 == Choose<13, 3>::value, "");
static_assert(1326 == Choose<52, 2>::value, "");

}
