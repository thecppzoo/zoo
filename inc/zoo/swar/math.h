#pragma once
#include "SWAR.h"

template <typename IntegerType = size_t>
constexpr static
std::enable_if_t<std::is_integral_v<IntegerType>, bool>
is_power_of_two(IntegerType x) noexcept {
  return x > 0 && (x & (x - 1)) == 0;
}

template <typename IntegerType = size_t, IntegerType X>
constexpr static
std::enable_if_t<std::is_integral_v<IntegerType>, bool>
is_power_of_two() noexcept {
  return is_power_of_two(X);
}
static_assert(is_power_of_two<int, 4>());


template <size_t N, typename IntegerType = size_t>
constexpr static
std::enable_if_t<
    std::is_integral_v<IntegerType> &&
    is_power_of_two<size_t, N>(), size_t>
modulo_power_of_two(IntegerType x) noexcept {
  return x & (N - 1);
}

static_assert(modulo_power_of_two<4>(0) == 0);
static_assert(modulo_power_of_two<8>(9) == 1);
static_assert(modulo_power_of_two<4096>(4097) == 1);

// SWAR power of two

namespace zoo::swar {
template <typename S>
constexpr static auto subtract_one_unsafe(S x) noexcept {
  constexpr auto Ones = S::LeastSignificantBit;
  auto x_minus_1 = S{x.value() - Ones};
  return x_minus_1;
}

// todo subtract K unsafe using BitmaskMaker

template <typename S>
constexpr static auto is_power_of_two(S x) noexcept {
  constexpr auto NBits = S::NBits;
  using T = typename S::type;
  auto x_minus_1 = subtract_one_unsafe(x);
  return equals(S{x_minus_1.value() & x.value()}, S{0});
}

} // namespace zoo::swar

