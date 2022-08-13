#pragma once

/// \file Swar.h SWAR operations

#include "zoo/meta/log.h"

#include <type_traits>

namespace zoo { namespace swar {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

template<int LogNBits>
constexpr uint64_t popcount(uint64_t a) noexcept {
    return
        std::conditional_t<
            (4 < LogNBits),
            meta::PopcountIntrinsic<LogNBits>,
            meta::PopcountLogic<LogNBits>
        >::execute(a);
}


/// Index into the bits of the type T that contains the MSB.
template<typename T> constexpr typename std::make_unsigned<T>::type msbIndex(T v) noexcept {
    return 8*sizeof(T) - 1 - __builtin_clzll(v);
}

/// Index into the bits of the type T that contains the LSB.
template<typename T> constexpr typename std::make_unsigned<T>::type lsbIndex(T v) noexcept {
    return __builtin_ctzll(v) + 1;
}

/// Core abstraction around SIMD Within A Register (SWAR).  Specifies 'lanes'
/// of NBits width against a type T, and provides an abstraction for performing
/// SIMD operations against that primitive type T treated as a SIMD register.
/// SWAR operations are usually constant time, log(lane count) cost, or O(lane count) cost.
/// Certain computational workloads can be materially sped up using SWAR techniques.
template<int NBits, typename T = uint64_t> struct SWAR {
    static constexpr inline auto Lanes = sizeof(T) * 8 / NBits;
    SWAR() = default;
    constexpr explicit SWAR(T v): m_v(v) {}
    constexpr explicit operator T() const noexcept { return m_v; }

    constexpr T value() const noexcept { return m_v; }

    constexpr SWAR operator~() const noexcept { return SWAR{~m_v}; }
    #define SWAR_BINARY_OPERATORS_X_LIST \
      X(SWAR, &) X(SWAR, ^) X(SWAR, |) X(SWAR, -) X(SWAR, +) X(SWAR, *)
    #define X(rt,op) constexpr rt operator op (rt o) const noexcept { return rt (m_v op o.m_v); };
    SWAR_BINARY_OPERATORS_X_LIST
    #undef X

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
    /// IE: 4 bit wide 32 bit SWAR: 0x0040'0000 will return 5, not 22 (0 indexed).
    constexpr int top() const noexcept { return msbIndex(m_v) / NBits; }
    constexpr int lsbIndex() const noexcept { return __builtin_ctzll(m_v) / NBits; }

    constexpr SWAR set(int index, int bit) const noexcept {
        return SWAR(m_v | (T(1) << (index * NBits + bit)));
    }

    T m_v;
};

// SWAR is a useful abstraction for performing computations in lanes overlaid
// over any given integral type.
// Doing additions, subtractions, and compares via SWAR techniques requires an
// extra bit per lane be available past the lane size, _or_ knowledge that both
// of your MSBs are set 0 (leaving space for the operation).  Similarly, doing
// multiplications via SWAR techniques require double bits per lane (unless you
// can bind your inputs at half lane size).
// This leads to a useful technique (which we use in the robin hood table)
// where we interleave two related small bit count integers inside of a lane of
// swar.  More generally, this is useful because it sometimes allows fast
// operations on side "a" of some lane if side "b" is blitted out, and vice
// versa.  In the spirit of separation of concerns, we provide a cut-lane-SWAR
// abstraction here.

template<int NBitsMost, int NBitsLeast, typename T = uint64_t> struct SWARWithSubLanes : SWAR<NBitsMost+NBitsLeast, T> {
    static constexpr inline auto Available = sizeof(T);
    static constexpr inline auto LaneBits = NBitsLeast+NBitsMost;
    static constexpr inline auto NSlots = Available * 8 / LaneBits;

    SWARWithSubLanes() = default;
    constexpr explicit SWARWithSubLanes(T v) : SWAR<NBitsMost+NBitsLeast, T>(v) {}
    constexpr explicit operator T() const noexcept { return this->m_v; }

    // M is most significant bits slice, L is least significant bits slice.
    // 0x....M2L2M1L1 or MN|LN||...||M2|L2||M1|L1
    using SL = SWARWithSubLanes<NBitsMost, NBitsLeast, T>;
    using Base = swar::SWAR<LaneBits, T>;

    //constexpr T Ones = meta::BitmaskMaker<NBits, SWAR<NBits, T>{1}.value(), T>::value;
    static constexpr inline auto LeastOnes = meta::BitmaskMaker<T, Base{1}.value(), LaneBits>::value;
    static constexpr inline auto MostOnes = meta::BitmaskMaker<T, Base{1<<NBitsLeast}.value(), LaneBits>::value;
    static constexpr inline auto LeastMask = meta::BitmaskMaker<T, Base{~(~0ull<<NBitsLeast)}.value(), LaneBits>::value;
    static constexpr inline auto MostMask = ~LeastMask;

    constexpr auto least() const noexcept {
        return this->m_v & LeastMask;
    }

    // Returns only the least significant bits at specified position.
    constexpr auto least(u32 pos) const noexcept {
        constexpr auto filter = (T(1) << LaneBits) - 1;
        const auto keep = (filter << (LaneBits * pos)) & LeastMask;
        return this->m_v & keep;
    }

    // Returns only the least significant bits at specified position, 'decoded' to their integer value.
    constexpr auto leastFlat(u32 pos) const noexcept {
        return least(pos) >> (LaneBits*pos);
    }

    constexpr auto most() const noexcept {
        return this->m_v & MostMask;
    }

    // Returns only the most significant bits at specified position.
    constexpr auto most(u32 pos) const noexcept {
        constexpr auto filter = (T(1) << LaneBits) - 1;
        const auto keep = (filter << (LaneBits * pos)) & MostMask;
        return this->m_v & keep;
    }

    // Returns only the most significant bits at specified position, 'decoded' to their integer value.
    constexpr auto mostFlat(u32 pos) const noexcept {
        return most(pos) >> (LaneBits*pos)>> NBitsLeast;
    }

    // Sets the lsb sublane at |pos| with least significant NBitsLeast of |in|
    constexpr auto least(T in, int pos) const noexcept {
        constexpr auto filter = (T(1) << LaneBits) - 1;
        const auto keep = ~(filter << (LaneBits * pos)) | MostMask;
        const auto rdyToInsert = this->m_v & keep;
        const auto rval = rdyToInsert | ((in & LeastMask) << (LaneBits * pos));
        return SL(rval);
    }

    // Sets the msb sublane at |pos| with least significant NBitsMost of |in|
    constexpr auto most(T in, int pos) const noexcept {
        constexpr auto filter = (T(1) << LaneBits) - 1;
        const auto keep = ~(filter << (LaneBits * pos)) | LeastMask;
        const auto rdyToInsert = this->m_v & keep;
        const auto insVal = (((in<<NBitsLeast) & MostMask) << (LaneBits * pos));
        const auto rval = rdyToInsert | insVal;
        return SL(rval);
    }
};



/// Defining operator== on base SWAR types is entirely too error prone. Force a verbose invocation.
template<int NBits, typename T = uint64_t> constexpr auto horizontalEquality(SWAR<NBits, T> left, SWAR<NBits, T> right) {
    return left.m_v == right.m_v;
}

/// Isolating >= NBits in underlying integer type currently results in disaster.
// TODO(scottbruceheart) Attempting to use binary not (~) results in negative shift warnings.
template<int NBits, typename T = uint64_t>
constexpr auto isolate(T pattern) {
    return pattern & ((T(1)<<NBits)-1);
}

/// Clears the least bit set in type T
template<typename T = uint64_t>
constexpr auto clearLSB(T v) {
    return v & (v - 1);
}

/// Leaves on the least bit set, or all 1s for a 0 input.
template<typename T = uint64_t>
constexpr auto isolateLSB(T v) {
    return v & ~clearLSB(v);
}

template<int NBits, typename T>
constexpr auto leastNBitsMask() {
    return (T(1ull)<<NBits)-1;
}

template<int NBits, uint64_t T>
constexpr auto leastNBitsMask() {
    return ~((0ull)<<NBits);
}


template<int NBits, typename T = uint64_t>
constexpr T mostNBitsMask() {
  return ~leastNBitsMask<sizeof(T)*8-NBits, T>();
}

/// Clears the block of N bits anchored at the LSB.
/// clearLSBits<3> applied to binary 00111100 is binary 00100000
template<int NBits, typename T = uint64_t>
constexpr auto clearLSBits(T v) {
    constexpr auto lowMask = leastNBitsMask<NBits, T>();
    return v &(~(lowMask << meta::logFloor(isolateLSB<T>(v))));
}

/// Isolates the block of N bits anchored at the LSB.
/// isolateLSBits<2> applied to binary 00111100 is binary 00001100
template<int NBits, typename T = uint64_t>
constexpr auto isolateLSBits(T v) {
    constexpr auto lowMask = leastNBitsMask<NBits, T>();
    return v &(lowMask << meta::logFloor(isolateLSB<T>(v)));
}

/// Broadcasts the value in the 0th lane of the SWAR to the entire SWAR.
/// Precondition: 0th lane of |v| contains a value to broadcast, remainder of input SWAR zero.
template<int NBits, typename T = uint64_t>
constexpr auto broadcast(SWAR<NBits, T> v) {
    constexpr T Ones = meta::BitmaskMaker<T, 1, NBits>::value;
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
    //static_assert(meta::logCeiling(N) < NBits, "N is too big for this technique");  // ctzll isn't constexpr.
    constexpr auto msbPosition  = NBits - 1;
    constexpr auto msb = T(1) << msbPosition;
    constexpr auto msbMask = meta::BitmaskMaker<T, msb, NBits>::value;
    constexpr auto subtraend = meta::BitmaskMaker<T, N, NBits>::value;
    auto adjusted = v.value() | msbMask;
    auto rv = adjusted - subtraend;
    rv &= msbMask;
    return BooleanSWAR<NBits, T>(rv);
}

/*
This is just a draft implementation:
1. The isolator needs pre-computing instead of adding 3 ops per iteration
2. The update of the isolator is not needed in the last iteration
3. Consider returning not the logarithm, but the biased by 1 (to support 0)
 */
template<int NBits, typename T>
constexpr SWAR<NBits, T> logarithmFloor(SWAR<NBits, T> v) noexcept {
    constexpr auto LogNBits = meta::logFloor(NBits);
    static_assert(NBits == (1 << LogNBits), "Logarithms of element width not power of two is un-implemented");
    auto whole = v.value();
    auto isolationMask = BooleanSWAR<NBits, T>::maskLaneMSB;
    for(auto groupSize = 1; groupSize < NBits; groupSize <<= 1) {
        auto shifted = whole >> groupSize;

        // When shifting down a group to double the size of a group, the upper
        // "groupSize" bits will come from the element above, mask them out
        auto isolator = ~isolationMask;
        auto withoutCrossing = shifted & isolator;
        whole |= withoutCrossing;
        isolationMask |= (isolationMask >> groupSize);
    }
    constexpr auto ones = meta::BitmaskMaker<T, 1, NBits>::value;
    auto popcounts = meta::PopcountLogic<LogNBits, T>::execute(whole);
    return SWAR<NBits, T>{popcounts - ones};
}

static_assert(
    logarithmFloor(SWAR<8>{0x8040201008040201ull}).value() ==
    0x0706050403020100ull
);
static_assert(
    logarithmFloor(SWAR<8>{0xFF7F3F1F0F070301ull}).value() ==
    0x0706050403020100ull
);

}}
