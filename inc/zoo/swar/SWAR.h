#pragma once

/// \file Swar.h SWAR operations

#include "zoo/swar/metaLog.h"

#include <type_traits>

namespace zoo { namespace swar {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

/// Repeats the given pattern in the whole of the argument
/// \tparam T the desired integral type
/// \tparam Progression how big the pattern is
/// \tparam Remaining how many more times to copy the pattern
template<typename T, int Progression, int Remaining> struct BitmaskMaker {
    constexpr static T repeat(T v) noexcept {
        return
            BitmaskMaker<T, Progression, Remaining - 1>::repeat(v | (v << Progression));
    }
};
template<typename T, int P> struct BitmaskMaker<T, P, 0> {
    constexpr static T repeat(T v) noexcept { return v; }
};

/// Front end to \c BitmaskMaker with the repeating count set to the whole size
template<int size, typename T>
constexpr T makeBitmask(T v) noexcept {
    return BitmaskMaker<T, size, sizeof(T)*8/size>::repeat(v);
}

/// Core implementation details
namespace detail {
    template<int level>
    constexpr auto popcountMask =
        makeBitmask<1 << (level + 1)>(
            BitmaskMaker<uint64_t, 1, (1 << level) - 1>::repeat(1)
        );

    static_assert(makeBitmask<2>(1ull) == popcountMask<0>);
}

template<int Bits> struct UInteger_impl;
template<> struct UInteger_impl<8> { using type = uint8_t; };
template<> struct UInteger_impl<16> { using type = uint16_t; };
template<> struct UInteger_impl<32> { using type = uint32_t; };
template<> struct UInteger_impl<64> { using type = uint64_t; };

template<int Bits> using UInteger = typename UInteger_impl<Bits>::type;

template<int Level>
constexpr uint64_t popcount_logic(uint64_t arg) noexcept {
    auto v = popcount_logic<Level - 1>(arg);
    constexpr auto shifter = 1 << Level;
    return
        ((v >> shifter) & detail::popcountMask<Level>) +
        (v & detail::popcountMask<Level>);
}
/// Hamming weight of each bit pair
template<>
constexpr uint64_t popcount_logic<0>(uint64_t v) noexcept {
    // 00: 00; 00
    // 01: 01; 01
    // 10: 01; 01
    // 11: 10; 10
    return v - ((v >> 1) & detail::popcountMask<0>);
}

template<int Level>
constexpr uint64_t popcount_builtin(uint64_t v) noexcept {
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

namespace detail {

    template<bool> struct Selector_impl {
        template<int Level>
        constexpr static uint64_t execute(uint64_t v) noexcept {
            return popcount_logic<Level>(v);
        }
    };
    template<> struct Selector_impl<true> {
        template<int Level>
        constexpr static uint64_t execute(uint64_t v) noexcept {
            return popcount_builtin<Level - 2>(v);
        }
    };

}

template<int Level>
constexpr uint64_t popcount(uint64_t a) noexcept {
    return detail::Selector_impl<2 < Level>::template execute<Level>(a);
}


/// Index into the bits of the type T that contains the MSB.
template<typename T> constexpr typename std::make_unsigned<T>::type msbIndex(T v) noexcept {
    return 8*sizeof(T) - 1 - __builtin_clzll(v);
}

/// Index into the bits of the type T that contains the LSB.
template<typename T> constexpr typename std::make_unsigned<T>::type lsbIndex(T v) noexcept {
    return __builtin_ctzll(v) + 1;
}

template<int NBits, typename T = uint64_t> struct SWAR {
    SWAR() = default;
    constexpr explicit SWAR(T v): m_v(v) {}
    constexpr explicit operator T() const noexcept { return m_v; }

    constexpr T value() const noexcept { return m_v; }

    constexpr SWAR operator|(SWAR o) const noexcept { return SWAR(m_v | o.m_v); }
    constexpr SWAR operator&(SWAR o) const noexcept { return SWAR(m_v & o.m_v); }
    constexpr SWAR operator^(SWAR o) const noexcept { return SWAR(m_v ^ o.m_v); }

    constexpr T at(int position) const noexcept {
        constexpr auto filter = (T(1) << NBits) - 1;
        return filter & (m_v >> (NBits * position));
    }

    constexpr SWAR clear(int position) const noexcept {
        constexpr auto filter = (T(1) << NBits) - 1;
        auto invertedMask = filter << (NBits * position);
        auto mask = ~invertedMask;
        return SWAR(m_v & mask);
    }

    /// The SWAR lane index that contains the MSB.  It is not the bit index of the MSB.
    /// IE: 8 bit wide 32 bit SWAR: 0x0040'0000 will return 5, not 43.
    constexpr int top() const noexcept { return msbIndex(m_v) / NBits; }
    constexpr int lsbIndex() const noexcept { return __builtin_ctzll(m_v) / NBits; }

    constexpr SWAR set(int index, int bit) const noexcept {
        return SWAR(m_v | (T(1) << (index * NBits + bit)));
    }

    T m_v;
};

/// Defining operator== on base SWAR types is entirely too error prone. Force a verbose invocation.
template<int NBits, typename T = uint64_t> constexpr auto horizontalEquality(SWAR<NBits, T> left, SWAR<NBits, T> right) {
  return left.m_v == right.m_v;
}

/// Isolating more than bits in type currently results in disaster.
template<int NBits, typename T = uint64_t>
constexpr auto isolate(T pattern) {
  return pattern & ((T(1)<<NBits)-1);
}

/// Clears the lowest bit set in type T
template<typename T = uint64_t>
constexpr auto clearLSB(T v) {
  return v & (v - 1);
}

// Leaves on the lowest bit set, or all 1s for a 0 input.
template<typename T = uint64_t>
constexpr auto isolateLSB(T v) {
  return v & ~clearLSB(v);
}

template<int NBits, typename T = uint64_t>
constexpr auto lowestNBitsMask() {
  return (T(1)<<NBits)-1;
}

/// Clears the block of N bits anchored at the LSB.
/// clearLSBits<3> applied to binary 00111100 is binary 00100000
template<int NBits, typename T = uint64_t>
constexpr auto clearLSBits(T v) {
  constexpr auto lowMask = lowestNBitsMask<NBits>();
  return v &(~(lowMask << metaLogFloor(isolateLSB<T>(v))));
}

/// Isolates the block of N bits anchored at the LSB.
/// isolateLSBits<2> applied to binary 00111100 is binary 00001100
template<int NBits, typename T = uint64_t>
constexpr auto isolateLSBits(T v) {
  constexpr auto lowMask = lowestNBitsMask<NBits>();
  return v &(lowMask << metaLogFloor(isolateLSB<T>(v)));
}

template<int NBits, typename T = uint64_t>
constexpr auto broadcast(SWAR<NBits, T> v) {
  constexpr T Ones = makeBitmask<NBits, T>(SWAR<NBits, T>{1}.value());
  return SWAR<NBits, T>(T(v) * Ones);
}

/// BooleanSWAR treats the MSB of each SWAR lane as the boolean associated with that lane.
template<int NBits, typename T>
struct BooleanSWAR: SWAR<NBits, T> {
    // Booleanness is stored in MSB of a given swar.
    static constexpr T maskLaneMSB = broadcast<NBits, T>(SWAR<NBits, T>(T(1) << (NBits -1))).value();
    constexpr explicit BooleanSWAR(T v): SWAR<NBits, T>(v) {}

    constexpr BooleanSWAR clear(int bit) const noexcept {
        constexpr auto Bit = T(1) << (NBits - 1);
        return this->m_v ^ (Bit << (NBits * bit)); }

    /// BooleanSWAR treats the MSB of each lane as the boolean associated with that lane.
    /// A logical NOT in this circumstance _only_ flips the MSB of each lane.  This operation is
    /// not ones or twos complement.
    constexpr BooleanSWAR operator not() const noexcept {
        return maskLaneMSB ^ *this;
    }
 private:
    constexpr BooleanSWAR(SWAR<NBits, T> initializer) noexcept: SWAR<NBits, T>(initializer) {}

    template<int NB, typename TT> friend
    constexpr BooleanSWAR<NB, TT> greaterEqual_MSB_off(SWAR<NB, TT> left, SWAR<NB, TT> right) noexcept;

};

template<int NBits, typename T>
constexpr BooleanSWAR<NBits, T> greaterEqual_MSB_off(SWAR<NBits, T> left, SWAR<NBits, T> right) noexcept {
    constexpr auto MLMSB = BooleanSWAR<NBits, T>::maskLaneMSB;
    // TODO(scottbruceheart) operator- and others on SWAR to avoid using .value here.
    return SWAR<NBits, T>{
        (((left.value() | MLMSB) - right.value()) & MLMSB)
    };
}

template<int N, int NBits, typename T>
constexpr BooleanSWAR<NBits, T> greaterEqual(SWAR<NBits, T> v) noexcept {
    static_assert(1 < NBits, "Degenerated SWAR");
    //static_assert(metaLogCeiling(N) < NBits, "N is too big for this technique");  // ctzll isn't constexpr.
    constexpr auto msbPos  = NBits - 1;
    constexpr auto msb = T(1) << msbPos;
    constexpr auto msbMask = makeBitmask<NBits, T>(msb);
    constexpr auto subtraend = makeBitmask<NBits, T>(N);
    auto adjusted = v.value() | msbMask;
    auto rv = adjusted - subtraend;
    rv &= msbMask;
    return BooleanSWAR<NBits, T>(rv);
}

}}
