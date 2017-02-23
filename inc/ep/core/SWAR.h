#pragma once

/// \file Swar.h SWAR operations

#include "metaLog.h"

namespace ep { namespace core {

/// Repeats the given pattern in the whole of the argument
/// \tparam T the desired integral type
/// \tparam Progression how big the pattern is
/// \tparam Remaining how many more times to copy the pattern
template<typename T, int Progression, int Remaining> struct BitmaskMaker {
    constexpr static T repeat(T v) {
        return
            BitmaskMaker<T, Progression, Remaining - 1>::repeat(v | (v << Progression));
    }
};
template<typename T, int P> struct BitmaskMaker<T, P, 0> {
    constexpr static T repeat(T v) { return v; }
};

/// Front end to \c BitmaskMaker with the repeating count set to the whole size
template<int size, typename T>
constexpr T makeBitmask(T v) {
    return BitmaskMaker<T, size, sizeof(T)*8/size>::repeat(v);
}

/// Core implementation details
namespace detail {
    template<int level>
    constexpr auto popcountMask =
        makeBitmask<1 << (level + 1)>(
            BitmaskMaker<uint64_t, 1, (1 << level) - 1>::repeat(1)
        );

    static_assert(makeBitmask<2>(1ull) == popcountMask<0>, "");
}

template<int Bits> struct UInteger_impl;
template<> struct UInteger_impl<8> { using type = uint8_t; };
template<> struct UInteger_impl<16> { using type = uint16_t; };
template<> struct UInteger_impl<32> { using type = uint32_t; };
template<> struct UInteger_impl<64> { using type = uint64_t; };

template<int Bits> using UInteger = typename UInteger_impl<Bits>::type;

template<int Level>
constexpr uint64_t popcount_logic(uint64_t arg) {
    auto v = popcount_logic<Level - 1>(arg);
    constexpr auto shifter = 1 << Level;
    return
        ((v >> shifter) & detail::popcountMask<Level>) +
        (v & detail::popcountMask<Level>);
}
/// Hamming weight of each bit pair
template<>
constexpr uint64_t popcount_logic<0>(uint64_t v) {
    // 00: 00; 00
    // 01: 01; 01
    // 10: 01; 01
    // 11: 10; 10
    return v - ((v >> 1) & detail::popcountMask<0>);
}

template<int Level>
constexpr uint64_t popcount_builtin(uint64_t v) {
    using UI = UInteger<1 << (Level + 3)>;
    constexpr auto times = 8*sizeof(v);
    uint64_t rv = 0;
    for(auto n = times; n; ) {
        n -= 8*sizeof(UI);
        UI tmp = v >> n;
        tmp = __builtin_popcountll(tmp);
        rv |= uint64_t(tmp) << n;
    }
    return rv;
}

template<bool> struct Selector_impl {
    template<int Level>
    constexpr static uint64_t execute(uint64_t v) {
        return popcount_logic<Level>(v);
    }
};
template<> struct Selector_impl<true> {
    template<int Level>
    constexpr static uint64_t execute(uint64_t v) {
        return popcount_builtin<Level - 2>(v);
    }
};

template<int Level>
constexpr uint64_t popcount(uint64_t a) {
    return Selector_impl<2 < Level>::template execute<Level>(a);
}
//template<>
//constexpr uint64_t popcount<3>(uint64_t a) {
    

static_assert(0x210 == popcount<0>(0x320), "");
static_assert(0x4321 == popcount<1>(0xF754), "");
static_assert(0x50004 == popcount<3>(0x3E001122), "");

template<int Size, typename T = uint64_t> struct SWAR {
    SWAR() = default;
    constexpr explicit SWAR(T v): m_v(v) {}

    constexpr T value() const noexcept { return m_v; }
    constexpr SWAR operator|(SWAR o) { return SWAR(m_v | o.m_v); }

    constexpr bool operator==(SWAR o) { return m_v == o.m_v; }

    constexpr T at(int position) {
        constexpr auto filter = (T(1) << Size) - 1;
        return filter & (m_v >> (Size * position));
    }

    constexpr void clearAt(int position) {
        constexpr auto filter = (T(1) << Size) - 1;
        auto invertedMask = filter << (Size * position);
        auto mask = ~invertedMask;
        m_v &= mask;
    }
protected:
    T m_v;
};

template<int N, int Size, typename T>
constexpr SWAR<Size, T> greaterEqualSWAR(SWAR<Size, T> v) {
    static_assert(1 < N, "N is too small");
    static_assert(metaLogCeiling(N) < Size, "N is too big for this technique");
    constexpr auto msbPos  = Size - 1;
    constexpr auto msb = T(1) << msbPos;
    constexpr auto msbMask = makeBitmask<Size, T>(msb);
    constexpr auto subtraend = makeBitmask<Size, T>(N);
    auto adjusted = v.value() | msbMask;
    auto rv = adjusted - subtraend;
    rv &= msbMask;
    return SWAR<Size, T>(rv >> msbPos);
}

static_assert(
    0x10110001 == greaterEqualSWAR<3>(SWAR<4, uint32_t>(0x32451027)).value(),
    ""
);

}}
