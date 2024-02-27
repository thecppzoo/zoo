#pragma once

/// \file Swar.h SWAR operations

#include "zoo/meta/log.h"

#include <type_traits>

#ifdef _MSC_VER
#include <iso646.h>
#endif

namespace zoo { namespace swar {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = std::uint8_t;

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
template<typename T>
constexpr std::make_unsigned_t<T> msbIndex(T v) noexcept {
    return meta::logFloor(v);
}

/// Index into the bits of the type T that contains the LSB.
template<typename T>
constexpr std::make_unsigned_t<T> lsbIndex(T v) noexcept {
    #ifdef _MSC_VER
        // ~v & (v - 1) turns on all trailing zeroes, zeroes the rest
        return meta::logFloor(1 + (~v & (v - 1)));
    #else
        return ~v ? __builtin_ctzll(v) : sizeof(T) * 8;
    #endif
}

/// Core abstraction around SIMD Within A Register (SWAR).  Specifies 'lanes'
/// of NBits width against a type T, and provides an abstraction for performing
/// SIMD operations against that primitive type T treated as a SIMD register.
/// SWAR operations are usually constant time, log(lane count) cost, or O(lane count) cost.
/// Certain computational workloads can be materially sped up using SWAR techniques.
template<int NBits_, typename T = uint64_t>
struct SWAR {
    using type = std::make_unsigned_t<T>;
    constexpr static inline type
        NBits = NBits_,
        BitWidth = sizeof(T) * 8,
        Lanes = BitWidth / NBits,
        NSlots = Lanes,
        PaddingBitsCount = BitWidth % NBits,
        SignificantBitsCount = BitWidth - PaddingBitsCount,
        AllOnes = ~std::make_unsigned_t<T>{0} >> PaddingBitsCount,
        LeastSignificantBit = meta::BitmaskMaker<T, std::make_unsigned_t<T>{1}, NBits>::value,
        MostSignificantBit = LeastSignificantBit << (NBits - 1),
        LeastSignificantLaneMask =
            ~(~T(0) << T(sizeof(T) * 8 == NBits ? 0 : NBits)),
        // Use LowerBits in favor of ~MostSignificantBit to not pollute
        // "don't care" bits when non-power-of-two bit lane sizes are supported
        LowerBits = MostSignificantBit - LeastSignificantBit;

    SWAR() = default;
    constexpr explicit SWAR(T v): m_v(v) {}
    constexpr explicit operator T() const noexcept { return m_v; }

    constexpr T value() const noexcept { return m_v; }

    #define SWAR_UNARY_OPERATORS_X_LIST \
        X(SWAR, ~)
    //constexpr SWAR operator~() const noexcept { return SWAR{~m_v}; }
    #define SWAR_BINARY_OPERATORS_X_LIST \
        X(SWAR, &) X(SWAR, ^) X(SWAR, |) X(SWAR, -) X(SWAR, +)

    #define X(rt, op) constexpr rt operator op() const noexcept { return rt(op m_v); }
    SWAR_UNARY_OPERATORS_X_LIST
    #undef X

    #define X(rt,op) constexpr rt operator op (rt o) const noexcept { return rt (m_v op o.m_v); };
    SWAR_BINARY_OPERATORS_X_LIST
    #undef X

    // Returns lane at position with other lanes cleared.
    constexpr T isolateLane(int position) const noexcept {
        return m_v & (LeastSignificantLaneMask << (NBits * position));
    }

    // Returns lane value at position, in lane 0, rest of SWAR cleared.
    constexpr T at(int position) const noexcept {
        return LeastSignificantLaneMask & (m_v >> (NBits * position));
    }

    constexpr SWAR clear(int position) const noexcept {
        constexpr auto filter = (T(1) << NBits) - 1;
        auto invertedMask = filter << (NBits * position);
        auto mask = ~invertedMask;
        return SWAR(m_v & mask);
    }

    /// The SWAR lane index that contains the MSB.  It is not the bit index of the MSB.
    /// IE: 4 bit wide 32 bit SWAR: 0x0040'0000 will return 5, not 22 (0 indexed).
    constexpr auto top() const noexcept { return msbIndex(m_v) / NBits; }
    constexpr auto lsbIndex() const noexcept { return swar::lsbIndex(m_v) / NBits; }

    constexpr SWAR setBit(int index, int bit) const noexcept {
        return SWAR(m_v | (T(1) << (index * NBits + bit)));
    }

    constexpr auto blitElement(int index, T value) const noexcept {
        auto elementMask = ((T(1) << NBits) - 1) << (index * NBits);
        return SWAR((m_v & ~elementMask) | (value << (index * NBits)));
    }

    constexpr SWAR blitElement(int index, SWAR other) const noexcept {
        constexpr auto OneElementMask = SWAR(~(~T(0) << NBits));
        auto IsolationMask = OneElementMask.shiftLanesLeft(index);
        return (*this & ~IsolationMask) | (other & IsolationMask);
    }

    constexpr SWAR shiftLanesLeft(int laneCount) const noexcept {
        return SWAR(value() << (NBits * laneCount));
    }

    constexpr SWAR shiftLanesRight(int laneCount) const noexcept {
        return SWAR(value() >> (NBits * laneCount));
    }

    /// \brief as the name suggests
    /// \param protectiveMask should clear the bits that would cross the lane.
    /// The bits that will be cleared are directly related to the count of
    /// shifts, it is natural to maintain the protective mask by the caller,
    /// otherwise, the mask would have to be computed in all invocations.
    /// We are not sure the optimizer would maintain this mask somewhere, if it
    /// were to recalculate it, it would be disastrous for performance
    /// \note the \c static_cast are necessary because of narrowing conversions
    #define SHIFT_INTRALANE_OP_X_LIST X(Left, <<) X(Right, >>)
    #define X(name, op) \
        constexpr SWAR \
        shiftIntraLane##name(int bitCount, SWAR protectiveMask) const noexcept { \
            T shiftC = static_cast<T>(bitCount); \
            auto V = (*this & protectiveMask).value(); \
            auto rv = static_cast<T>(V op shiftC); \
            return SWAR{rv}; \
        }
    SHIFT_INTRALANE_OP_X_LIST
    #undef X
    #undef SHIFT_INTRALANE_OP_X_LIST

    constexpr SWAR
    multiply(T multiplier) const noexcept { return SWAR{m_v * multiplier}; }
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

template<int NBitsLeast_, int NBitsMost_, typename T = uint64_t>
struct SWARWithSubLanes: SWAR<NBitsLeast_ + NBitsMost_ , T> {
    static constexpr inline auto NBitsLeast = NBitsLeast_;
    static constexpr inline auto NBitsMost = NBitsMost_;

    using Base = SWAR<NBitsMost + NBitsLeast, T>;
    static constexpr inline auto Available = sizeof(T);
    static constexpr inline auto LaneBits = NBitsLeast + NBitsMost;

    using Base::Base;
    constexpr SWARWithSubLanes(Base b) noexcept: Base(b) {}
    constexpr SWARWithSubLanes(T most, T least) noexcept:
        Base((most << NBitsLeast) | least)
    {}

    // M is most significant bits slice, L is least significant bits slice.
    // 0x....M2L2M1L1 or MN|LN||...||M2|L2||M1|L1
    using SL = SWARWithSubLanes<NBitsLeast, NBitsMost, T>;

    static constexpr inline auto LeastOnes =
        Base(meta::BitmaskMaker<T, Base{1}.value(), LaneBits>::value);
    static constexpr inline auto MostOnes =
        Base(LeastOnes.value() << NBitsLeast);
    static constexpr inline auto LeastMask = MostOnes - LeastOnes;
    static constexpr inline auto MostMask = ~LeastMask;

    constexpr auto least() const noexcept {
        return SL{LeastMask & *this};
    }

    // Isolate the least significant bits of the lane at the specified position.
    constexpr auto least(int pos) const noexcept {
        constexpr auto Filter = SL((T(1) << NBitsLeast) - 1);
        return Filter.shiftLanesLeft(pos) & *this;
    }

    // Returns only the least significant bits at specified position, 'decoded' to their integer value.
    constexpr auto leastFlat(int pos) const noexcept {
        return least().at(pos);
    }

    constexpr auto most() const noexcept {
        return SL{MostMask & *this};
    }

    // The most significant bits of the lane at the specified position.
    constexpr auto most(int pos) const noexcept {
        constexpr auto Filter =
            SL(((T(1) << SL::NBitsMost) - 1) << SL::NBitsLeast);
        return Filter.shiftLanesLeft(pos) & *this;
    }

    // The most significant bits of the lane at the specified position,
    // 'decoded' to their integer value.
    constexpr auto mostFlat(int pos) const noexcept {
        return most().at(pos) >> SL::NBitsLeast;
    }

    // Blits most sig bits into least significant bits. Experimental.
    constexpr auto flattenMostToLeast(int pos) const noexcept {
        return SL(this->m_v >> NBitsLeast) & LeastMask;
    }

    // Blits least sig bits into most significant bits. Experimental.
    constexpr auto promoteLeastToMost(int pos) const noexcept {
        return SL(this->m_v << NBitsMost) & MostMask;
    }

    // Sets the lsb sublane at |pos| with least significant NBitsLeast of |in|
    constexpr auto least(T in, int pos) const noexcept {
        constexpr auto filter = (T(1) << LaneBits) - 1;
        const auto keep = ~(filter << (LaneBits * pos)) | MostMask.value();
        const auto rdyToInsert = this->m_v & keep;
        const auto rval = rdyToInsert | ((in & LeastMask.value()) << (LaneBits * pos));
        return SL(rval);
    }

    // Sets the msb sublane at |pos| with least significant NBitsMost of |in|
    constexpr auto most(T in, int pos) const noexcept {
        constexpr auto filter = (T(1) << LaneBits) - 1;
        const auto keep = ~(filter << (LaneBits * pos)) | LeastMask.value();
        const auto rdyToInsert = this->m_v & keep;
        const auto insVal = (((in<<NBitsLeast) & MostMask.value()) << (LaneBits * pos));
        const auto rval = rdyToInsert | insVal;
        return SL(rval);
    }
};


/// Defining operator== on base SWAR types is entirely too error prone. Force a verbose invocation.
template<int NBits, typename T = uint64_t>
constexpr auto horizontalEquality(SWAR<NBits, T> left, SWAR<NBits, T> right) {
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
    using Base = SWAR<NBits, T>;

    constexpr explicit BooleanSWAR(T v): Base(v) {}

    constexpr BooleanSWAR clear(int bit) const noexcept {
        constexpr auto Bit = T(1) << (NBits - 1);
        return this->m_v ^ (Bit << (NBits * bit)); }

    constexpr BooleanSWAR clearLSB() const noexcept {
        return BooleanSWAR(swar::clearLSB(this->value()));
    }

    /// BooleanSWAR treats the MSB of each lane as the boolean associated with that lane.
    /// A logical NOT in this circumstance _only_ flips the MSB of each lane.  This operation is
    /// not ones or twos complement.
    constexpr auto operator ~() const noexcept {
        return BooleanSWAR(Base{Base::MostSignificantBit} ^ *this);
    }

    #define BOOLEANSWAR_BINARY_LOGIC_OPERATOR X(^) X(&) X(|)
    #define X(op) \
        constexpr BooleanSWAR operator op(BooleanSWAR other) const noexcept { return this->Base::operator op(other); }
    BOOLEANSWAR_BINARY_LOGIC_OPERATOR
    #undef X

    explicit
    constexpr operator bool() const noexcept { return this->m_v; }
 private:
    constexpr BooleanSWAR(Base initializer) noexcept:
        SWAR<NBits, T>(initializer)
    {}

    template<int N, int NB, typename TT>
    friend constexpr BooleanSWAR<NB, TT>
    constantIsGreaterEqual(SWAR<NB, TT>) noexcept;

    template<int N, int NB, typename TT>
    friend constexpr BooleanSWAR<NB, TT>
    constantIsGreaterEqual_MSB_off(SWAR<NB, TT>) noexcept;

    template<int NB, typename TT>
    friend constexpr BooleanSWAR<NB, TT>
    greaterEqual(SWAR<NB, TT>, SWAR<NB, TT>) noexcept;

    template<int NB, typename TT>
    friend constexpr BooleanSWAR<NB, TT>
    greaterEqual_MSB_off(SWAR<NB, TT>, SWAR<NB, TT>) noexcept;

    template<int NB, typename TT>
    friend constexpr BooleanSWAR<NB, TT>
    convertToBooleanSWAR(SWAR<NB, TT> arg) noexcept;
};

template<int NBits, typename T>
constexpr BooleanSWAR<NBits, T>
convertToBooleanSWAR(SWAR<NBits, T> arg) noexcept {
    return SWAR<NBits, T>{SWAR<NBits, T>::MostSignificantBit} & arg;
}

template<int N, int NBits, typename T>
constexpr BooleanSWAR<NBits, T>
constantIsGreaterEqual(SWAR<NBits, T> subtrahend) noexcept {
    static_assert(1 < NBits, "Degenerated SWAR");
    constexpr auto MSB_Position  = NBits - 1;
    constexpr auto MSB = T(1) << MSB_Position;
    constexpr auto MSB_Mask =
        SWAR<NBits, T>{meta::BitmaskMaker<T, MSB, NBits>::value};
    constexpr auto Minuend =
        SWAR<NBits, T>{meta::BitmaskMaker<T, N, NBits>::value};
    constexpr auto N_MSB = MSB & N;

    auto subtrahendWithMSB_on = MSB_Mask & subtrahend;
    auto subtrahendWithMSB_off = ~subtrahendWithMSB_on;
    auto subtrahendMSBs_turnedOff = subtrahend ^ subtrahendWithMSB_on;
    if constexpr(N_MSB) {
        auto leastSignificantComparison = Minuend - subtrahendMSBs_turnedOff;
        auto merged =
            subtrahendMSBs_turnedOff | // the minuend MSBs are on
            leastSignificantComparison;
        return MSB_Mask & merged;
    } else {
        auto minuendWithMSBs_turnedOn = Minuend | MSB_Mask;
        auto leastSignificantComparison =
            minuendWithMSBs_turnedOn - subtrahendMSBs_turnedOff;
        auto merged =
            subtrahendWithMSB_off & // the minuend MSBs are off
            leastSignificantComparison;
        return MSB_Mask & merged;
    }
}

template<int N, int NBits, typename T>
constexpr BooleanSWAR<NBits, T>
constantIsGreaterEqual_MSB_off(SWAR<NBits, T> subtrahend) noexcept {
    static_assert(1 < NBits, "Degenerated SWAR");
    constexpr auto MSB_Position  = NBits - 1;
    constexpr auto MSB = T(1) << MSB_Position;
    constexpr auto MSB_Mask = meta::BitmaskMaker<T, MSB, NBits>::value;
    constexpr auto Minuend = meta::BitmaskMaker<T, N, NBits>::value;
    constexpr auto N_MSB = MSB & Minuend;

    auto subtrahendWithMSB_on = subtrahend;
    auto subtrahendWithMSB_off = ~subtrahendWithMSB_on;
    auto subtrahendMSBs_turnedOff = subtrahend ^ subtrahendWithMSB_on;
    if constexpr(N_MSB) { return MSB_Mask; }
    else {
        auto minuendWithMSBs_turnedOn = Minuend | MSB_Mask;
        auto leastSignificantComparison =
            minuendWithMSBs_turnedOn - subtrahendMSBs_turnedOff;
        return MSB_Mask & leastSignificantComparison;
    }
}

template<int NBits, typename T>
constexpr BooleanSWAR<NBits, T>
greaterEqual_MSB_off(SWAR<NBits, T> left, SWAR<NBits, T> right) noexcept {
    constexpr auto MLMSB = SWAR<NBits, T>{SWAR<NBits, T>::MostSignificantBit};
    auto minuend = MLMSB | left;
    return MLMSB & (minuend - right);
}

template<int NB, typename T>
constexpr auto
booleans(SWAR<NB, T> arg) noexcept {
    return ~constantIsGreaterEqual<0>(arg);
}

template<int NBits, typename T>
constexpr auto
differents(SWAR<NBits, T> a1, SWAR<NBits, T> a2) {
    return booleans(a1 ^ a2);
}

template<int NBits, typename T>
constexpr auto
equals(SWAR<NBits, T> a1, SWAR<NBits, T> a2) {
    return ~differents(a1, a2);
}

/*
This is just a draft implementation:
b1. The isolator needs pre-computing instead of adding 3 ops per iteration
2. The update of the isolator is not needed in the last iteration
3. Consider returning not the logarithm, but the biased by 1 (to support 0)
 */
template<int NBits, typename T>
constexpr SWAR<NBits, T> logarithmFloor(SWAR<NBits, T> v) noexcept {
    constexpr auto LogNBits = meta::logFloor(NBits);
    static_assert(NBits == (1 << LogNBits), "Logarithms of element width not power of two is un-implemented");
    auto whole = v.value();
    auto isolationMask = SWAR<NBits, T>::MostSignificantBit;
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
