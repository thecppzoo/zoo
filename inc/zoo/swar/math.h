#pragma once
#include "SWAR.h"
#include <cstddef>
#include <cstdint>

namespace zoo::math {

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

}

// SWAR power of two

namespace zoo::swar {
template <typename S>
constexpr static auto subtract_one_unsafe(S x) noexcept {
  constexpr auto Ones = S::LeastSignificantBit;
  auto x_minus_1 = S{x.value() - Ones};
  return x_minus_1;
}
// todo subtract K unsafe using BitmaskMaker
// todo subtract K "saturated" using BitmaskMaker

template <typename S>
constexpr static auto is_power_of_two(S x) noexcept {
  constexpr auto NBits = S::NBits;
  using T = typename S::type;
  auto greater_than_0 = greaterEqual_MSB_off(x, S{0});
  auto x_minus_1 = subtract_one_unsafe(x);
  auto zero = equals(S{x_minus_1.value() & x.value()}, S{0});
  return greater_than_0 & zero;
}

template <size_t N, typename S>
constexpr static
std::enable_if_t<zoo::math::is_power_of_two<size_t, N>(), S>
modulo_power_of_two(const S x) noexcept {
  constexpr auto NBits = S::NBits;
  using T = typename S::type;
  constexpr auto N_minus_1 = N - 1;
  constexpr auto N_in_lanes = zoo::meta::BitmaskMaker<T, N_minus_1, NBits>::value;
  T y = x.value() & N_in_lanes;
  return S{y};
}


} // namespace zoo::swar

