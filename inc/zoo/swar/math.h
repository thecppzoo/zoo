#pragma once
#include "SWAR.h"

namespace zoo::math {

template <typename IntegerType = size_t>
constexpr static
std::enable_if_t<std::is_integral_v<IntegerType>, bool>
isPowerOfTwo(IntegerType x) noexcept {
    return x && (x & (x - 1)) == 0;
}

template <typename IntegerType = size_t, IntegerType X>
constexpr static
std::enable_if_t<std::is_integral_v<IntegerType>, bool>
isPowerOfTwo() noexcept {
    return isPowerOfTwo(X);
}
static_assert(isPowerOfTwo<int, 4>());


template <size_t N, typename IntegerType = size_t>
constexpr static
std::enable_if_t<
    std::is_integral_v<IntegerType> &&
    isPowerOfTwo<size_t, N>(), size_t>
moduloPowerOfTwo(IntegerType x) noexcept {
    return x & (N - 1);
}

static_assert(moduloPowerOfTwo<4>(0) == 0);
static_assert(moduloPowerOfTwo<8>(9) == 1);
static_assert(moduloPowerOfTwo<4096>(4097) == 1);

}

namespace zoo::swar {
template <typename S>
constexpr static auto subtractOneUnsafe(S x) noexcept {
    constexpr auto Ones = S::LeastSignificantBit;
    auto x_minus_1 = S{x.value() - Ones};
    return x_minus_1;
}
// todo subtract K unsafe using BitmaskMaker
// todo subtract K "saturated" using BitmaskMaker

template <typename S>
constexpr static auto isPowerOfTwo(S x) noexcept {
    constexpr auto NBits = S::NBits;
    using T = typename S::type;
    auto greater_than_0 = greaterEqual_MSB_off(x, S{0});
    auto x_minus_1 = subtractOneUnsafe(x);
    auto zero = equals(S{x_minus_1.value() & x.value()}, S{0});
    return greater_than_0 & zero;
}

template <size_t N, typename S>
constexpr static
std::enable_if_t<zoo::math::isPowerOfTwo<size_t, N>(), S>
moduloPowerOfTwo(const S x) noexcept {
    constexpr auto N_minus_1 = N - 1;
    constexpr auto N_in_lanes = zoo::meta::BitmaskMaker<typename S::type, N_minus_1, S::NBits>::value;
    auto y = x.value() & N_in_lanes;
    return S{y};
}


} // namespace zoo::swar

