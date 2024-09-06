#ifndef ZOO_HEADER_META_POPCOUNT_H
#define ZOO_HEADER_META_POPCOUNT_H

#include "zoo/meta/BitmaskMaker.h"
#include <cstdint>
#include <type_traits>

namespace zoo { namespace meta {

namespace detail {

template<int Level, typename T = uint64_t>
struct PopcountMask_impl {
    constexpr static inline auto value =
        BitmaskMaker<
            T,
            (1 << Level) - 1,
            (1 << (Level + 1))
        >::value;
};

template<int Bits> struct UInteger_impl;
template<> struct UInteger_impl<8> { using type = uint8_t; };
template<> struct UInteger_impl<16> { using type = uint16_t; };
template<> struct UInteger_impl<32> { using type = uint32_t; };
template<> struct UInteger_impl<64> { using type = uint64_t; };

// unfortunately this does not work since unsigned long long is not
// the same type as unsigned long, but may have the same size!
template<typename> constexpr int BitWidthLog = 0;
template<> constexpr inline int BitWidthLog<uint8_t> = 3;
template<> constexpr inline int BitWidthLog<uint16_t> = 4;
template<> constexpr inline int BitWidthLog<uint32_t> = 5;
template<> constexpr inline int BitWidthLog<uint64_t> = 6;

} // detail

template<int Bits> using UInteger = typename detail::UInteger_impl<Bits>::type;

template<int LogarithmOfGroupSize, typename T = std::uint64_t>
struct PopcountLogic {
    constexpr static int GroupSize = 1 << LogarithmOfGroupSize;
    constexpr static int HalvedGroupSize = GroupSize / 2;
    constexpr static auto CombiningMask =
        BitmaskMaker<T, T(1 << HalvedGroupSize) - 1, GroupSize>::value;
    using Recursion = PopcountLogic<LogarithmOfGroupSize - 1, T>;
    constexpr static T execute(T input);
};

template<typename T>
struct PopcountLogic<0, T> {
    constexpr static T execute(T input) { return input; }
};

template<typename T>
struct PopcountLogic<1, T> {
   constexpr static T execute(T input) {
        return input - ((input >> 1) & BitmaskMaker<T, 1, 2>::value);
        // For each pair of bits, the expression results in:
        // 11: 3 - 1: 2
        // 10: 2 - 1: 1
        // 01: 1 - 0: 1
        // 00: 0 - 0: 0
   }
};

template<int LogarithmOfGroupSize, typename T>
constexpr T PopcountLogic<LogarithmOfGroupSize, T>::execute(T input) {
    return
        Recursion::execute(input & CombiningMask) +
        Recursion::execute((input >> HalvedGroupSize) & CombiningMask);
}

#ifdef _MSC_VER
template<int LogarithmOfGroupSize, typename T = uint64_t>
using  PopcountIntrinsic = PopcountLogic<LogarithmOfGroupSize, T>;
#else
template<int LogarithmOfGroupSize, typename T = uint64_t>
struct PopcountIntrinsic {
    constexpr static T execute(T input) {
        using UI = UInteger<1 << LogarithmOfGroupSize>;
        constexpr auto times = 8*sizeof(T);
        uint64_t rv = 0;
        for(auto n = times; n; ) {
            n -= 8*sizeof(UI);
            UI tmp = input >> n;
            tmp = __builtin_popcountll(tmp);
            rv |= uint64_t(tmp) << n;
        }
        return rv;
    }
};
#endif

template<typename T>
constexpr auto NumBits() {
    return sizeof(T) * 8;
}
static_assert(NumBits<uint16_t>() == 16);
static_assert(NumBits<uint64_t>() == 64);

template <typename T = std::uint64_t>
constexpr
std::enable_if_t<std::is_unsigned_v<T> && NumBits<T>() <= 64, T>
basic_popcount(T x) {
    if constexpr (NumBits<T>() <= 32) {
        return __builtin_popcountl(x);
    } else {
        return __builtin_popcountll(x);
    }
}

static_assert(basic_popcount<uint64_t>(0b111) == 3);
static_assert(basic_popcount<uint64_t>(0xFF) == 8);
static_assert(basic_popcount<uint64_t>(0xFF'FF'FF'FF) == 32);
static_assert(basic_popcount<uint64_t>(0xFF'FF'FF'FF'FF'FF'FF'FF) == 64);
static_assert(basic_popcount<uint64_t>(0xFF'FF'FF'FF'FF'FF'FF'FF - 2 - 4 - 8) == 61);

}}

#endif
