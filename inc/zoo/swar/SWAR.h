#pragma once

/// \file Swar.h SWAR operations

#include "zoo/meta/log.h"

#include <array>
#include <type_traits>

#ifdef _MSC_VER
#include <iso646.h>
#endif

namespace zoo { namespace swar {

template <int NBits, typename T>
struct SWAR;

template <int NumBits, typename BaseType>
struct Literals_t {};

template <int NumBits, typename BaseType>
constexpr Literals_t<NumBits, BaseType>
Literals{};


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
template<typename T>
constexpr std::make_unsigned_t<T> msbIndex(T v) noexcept {
    return meta::logFloor(v);
}

/// Index into the bits of the type T that contains the LSB.
///
/// \todo incorporate __builtin_ctzg when it is more widely available
template<typename T>
constexpr std::make_unsigned_t<T> lsbIndex(T v) noexcept {
    // This check should be SFINAE, but supporting all sorts
    // of base types is an ongoing task, we put a bare-minimum
    // temporary preventive measure with static_assert
    static_assert(sizeof(T) <= 8, "Unsupported");
    #ifdef _MSC_VER
        // ~v & (v - 1) turns on all trailing zeroes, zeroes the rest
        return meta::logFloor(1 + (~v & (v - 1)));
    #else
        return ~v ? __builtin_ctzll(v) : sizeof(T) * 8;
    #endif
}

#ifndef _MSC_VER
constexpr __uint128_t lsbIndex(__uint128_t v) noexcept {
    auto low = (v << 64) >> 64;
    if(low) { return __builtin_ctzll(low); }
    return 64 + __builtin_ctzll(v >> 64);
}
#endif

/// Core abstraction around SIMD Within A Register (SWAR).  Specifies 'lanes'
/// of NBits width against a type T, and provides an abstraction for performing
/// SIMD operations against that primitive type T treated as a SIMD register.
/// SWAR operations are usually constant time, log(lane count) cost, or O(lane count) cost.
/// Certain computational workloads can be materially sped up using SWAR techniques.
template<int NBits_, typename T = uint64_t>
struct SWAR {
    using type = std::make_unsigned_t<T>;
    constexpr static auto Literal = Literals<NBits_, T>;
    constexpr static inline type
        NBits = NBits_,
        BitWidth = sizeof(T) * 8,
        Lanes = BitWidth / NBits,
        NSlots = Lanes,
        PaddingBitsCount = BitWidth % NBits,
        SignificantBitsCount = BitWidth - PaddingBitsCount,
        AllOnes = ~type{0} >> PaddingBitsCount,
            // Also constructed in RobinHood utils: possible bug?
        LeastSignificantBit = meta::BitmaskMaker<T, type{1}, NBits>::value,
        MostSignificantBit = LeastSignificantBit << (NBits - 1),
        LeastSignificantLaneMask =
            sizeof(T) * 8 == NBits ? // needed to avoid shifting all bits
                type(~T(0)) :
                ~(type(~type(0)) << type{NBits}),
        // Use LowerBits in favor of ~MostSignificantBit to not pollute
        // "don't care" bits when non-power-of-two bit lane sizes are supported
        LowerBits = MostSignificantBit - LeastSignificantBit,
        MaxUnsignedLaneValue = LeastSignificantLaneMask;

    template <typename InputIt>
    constexpr static auto fromRange(InputIt first, InputIt last) noexcept {
        auto result = T{0};
        for (; first != last; ++first) {
            result = (result << NBits) | *first;
        }
        return result;
    }

    template <typename Range>
    constexpr static auto from(const Range &values) noexcept {
        using std::begin; using std::end;
        return SWAR{fromRange(begin(values), end(values))};
    }

    constexpr SWAR(const std::array<T, Lanes> &array) : m_v{from(array.begin(), array.end())} {}

    template<
        typename Arg,
        std::size_t N,
        // Reject via SFINAE plain arrays with non-matching number of elements
        typename = std::enable_if_t<N == Lanes>
    >
    constexpr
    SWAR(Literals_t<NBits, T>, const Arg (&values)[N]):
        m_v{from(values)}
    {}

    constexpr std::array<T, Lanes> to_array() const noexcept {
        std::array<T, Lanes> result = {};
        for (int i = 0; i < Lanes; ++i) {
            auto otherEnd = Lanes - i - 1;
            result[otherEnd] = at(i);
        }
        return result;
    }

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
        return m_v & (LeastSignificantLaneMask << T(NBits * position));
    }

    // Returns lane value at position, in lane 0, rest of SWAR cleared.
    constexpr T at(int position) const noexcept {
        return LeastSignificantLaneMask & (m_v >> T(NBits * position));
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

template <int NBits, typename T, typename Arg>
SWAR(
    Literals_t<NBits, T>,
    const Arg (&values)[SWAR<NBits, T>::Lanes]
) -> SWAR<NBits, T>;

template <int NBits, typename T>
SWAR(
    Literals_t<NBits, T>,
    const std::array<T, SWAR<NBits, T>::Lanes>&
) -> SWAR<NBits, T>;

/// Defining operator== on base SWAR types is entirely too error prone. Force a verbose invocation.
template<int NBits, typename T = uint64_t>
constexpr auto horizontalEquality(SWAR<NBits, T> left, SWAR<NBits, T> right) {
    return left.m_v == right.m_v;
}



#if ZOO_USE_LEASTNBITSMASK
template<int NBits, typename T = uint64_t>
constexpr auto isolate(T pattern) {
    return pattern & leastNBitsMask<NBits, T>();
}
#endif

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

#if ZOO_USE_LEASTNBITSMASK
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
#endif

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

    template<std::size_t N, typename = std::enable_if_t<Base::Lanes == N>>
    constexpr BooleanSWAR(Literals_t<NBits, T>, const bool (&values)[N]):
        Base(Literals<NBits, T>, values)
    { this->m_v <<= (NBits - 1); }

    // Booleanness is stored in the MSBs
    static constexpr auto MaskMSB =
        broadcast<NBits, T>(Base(T(1) << (NBits -1)));
    static constexpr auto AllTrue = MaskMSB;
    static constexpr auto MaskLSB =
         broadcast<NBits, T>(Base(T(1)));
    // Turns off LSB of each lane
    static constexpr auto MaskNonLSB = ~MaskLSB;
    static constexpr auto MaskNonMSB = ~MaskMSB;
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

    constexpr auto operator not() const noexcept {
        return BooleanSWAR(MaskMSB ^ *this);
    }

    #define BOOLEANSWAR_BINARY_LOGIC_OPERATOR_X_LIST  X(^) X(&) X(|)
    #define X(op) \
        constexpr BooleanSWAR operator op(BooleanSWAR other) const noexcept { return this->Base::operator op(other); }
    BOOLEANSWAR_BINARY_LOGIC_OPERATOR_X_LIST
    #undef X

    // BooleanSWAR as a mask: BooleanSWAR<4, u16>(0x0800).MSBtoLaneMask() => SWAR<4,u16>(0x0F00)
    constexpr auto MSBtoLaneMask() const noexcept {
        const auto MSBMinusOne = this->m_v - (this->m_v >> (NBits-1)); // Convert pattern 10* to 01*
        return SWAR<NBits,T>(MSBMinusOne | this->m_v); // Blit 01* and 10* together for 1* when MSB was on.
    }

    explicit
    constexpr operator bool() const noexcept { return this->m_v; }
 private:
    constexpr BooleanSWAR(Base initializer) noexcept:
        SWAR<NBits, T>(initializer)
    {}

    template<int NB, typename TT>
    friend constexpr BooleanSWAR<NB, TT>
    equals(SWAR<NB, TT>, SWAR<NB, TT>) noexcept;

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
    constexpr T
    indexOfMostSignficantLaneSet(SWAR<NBits, T> test) noexcept;

    template<int NB, typename TT>
    friend constexpr BooleanSWAR<NB, TT>
    convertToBooleanSWAR(SWAR<NB, TT> arg) noexcept;
};

template <int NBits, typename T>
BooleanSWAR(
    Literals_t<NBits, T>,
    const bool (&values)[BooleanSWAR<NBits, T>::Lanes]
) -> BooleanSWAR<NBits, T>;

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

template<typename T, typename U, typename V>
constexpr T median(T x, U y, V z) {
    return (x | y) & (y | z) & (x | z);
}

template<int NBits, typename T>
constexpr BooleanSWAR<NBits, T>
greaterEqual(SWAR<NBits, T> left, SWAR<NBits, T> right) noexcept {
    // Adapted from TAOCP V4 P152
    // h is msbselector, x is right, l is lower/left.  Sets MSB to 1 in lanes
    // in test variable t for when xi < yi for lane i . Invert for greaterEqual.
    // t = h & ~<x~yz>
    // z = (x|h) - (y&~h)
    using S = swar::SWAR<NBits, T>;
    const auto h = S::MostSignificantBit, x = left.value(), y = right.value();  // x=left, y= right is x < y
    const auto z = (x|h) - (y&~h);
    // bitwise ternary median!
    const auto t = h & ~median(x, ~y, z);
    return ~BooleanSWAR<NBits, T>{static_cast<T>(t)};  // ~(x<y) === x >= y
}

// In the condition where only MSBs will be on, we can fast lookup with 1 multiply the index of the most signficant byte that is on.
// This appears to be a mapping from the (say) 256 unique values of a 64 bit int where only MSBs of each 8 bits can be on, but I don't fully understand it.
// Adapted from TAOCP Vol 4A Page 153 Eq 94.
template<int NBits, typename T>
constexpr T
indexOfMostSignficantLaneSet(SWAR<NBits, T> test) noexcept {
    const auto TypeWidth = sizeof(T) * 8;
    const auto TopVal = (T{1}<<(TypeWidth-NBits))-1, BottomVal = (T{1}<<(NBits-1))-1;
    const T MappingConstant = TopVal / BottomVal;
    return (test.value() * MappingConstant) >> (TypeWidth - NBits);
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
    return ~equals(a1, a2);
}

/**
 * @return BooleanSWAR that contains a true value in each lane where the two
 * input SWARs match.
 */
template<int NBits, typename T>
constexpr BooleanSWAR<NBits, T>
equals(SWAR<NBits, T> a1, SWAR<NBits, T> a2) noexcept {
    // Knuth, TAOCP 4A pg 152
    using S = swar::SWAR<NBits, T>;
    constexpr auto TAOCP_H{S{S::MostSignificantBit}}, TAOCP_L{S{S::LeastSignificantBit}};
    auto
        nullLaneIfEqual = a1 ^ a2,
        allowSubtractionByOneTurningOnMSB = nullLaneIfEqual | TAOCP_H,
        nullDetectorViaFlippingMSB_low = allowSubtractionByOneTurningOnMSB - TAOCP_L,
        equalLanesHaveMSB_off = nullLaneIfEqual | nullDetectorViaFlippingMSB_low,
        flipMSB = ~equalLanesHaveMSB_off,
        rv = TAOCP_H & flipMSB;
    return BooleanSWAR<NBits, T>{rv};
}

/**
 * \brief finds the first null (least significant lane zero) using less compute than finding the first in greaterEqual(S{0}, x) or equals(x, S{0})
 * @return psuedoBooleanSWAR. If there are no null lanes present, will be all
 * zeros.  If there is a null lane present, that lane will be all 1s, and any
 * less significant lane will be all zeros. Anything more significant than the
 * least significant null lane will be unspecified.
 */
template<int NBits, typename T>
constexpr auto
firstZeroLane(SWAR<NBits, T> x) {
    // Knuth, TAOCP 4A pg 152
    using S = swar::SWAR<NBits, T>;
    const auto h = S{S::MostSignificantBit}, l = S{S::LeastSignificantBit};
    return h & ( x - l) & ~x;
}

/**
 * \brief finds the first matching lane (least significant) using less compute than calling equals(a1, a2)
 * As firstZeroLane, except we test against a given swar, lanewise.
 * Less operations than equals(), but costs 1 more xor than firstZeroLane.
 * @return psuedoBooleanSWAR. No matches: all zero. Matching lane: all 1s, and
 * any less significant lane will be all zeros. Anything more significant will
 * be unspecified.
 */
template<int NBits, typename T>
constexpr auto
firstMatchingLane(SWAR<NBits, T> a1, SWAR<NBits, T> a2) {
    using S = swar::SWAR<NBits, T>;
    const auto h = S{S::MostSignificantBit}, l = S{S::LeastSignificantBit};
    return h & ( (a1^a2) - l) & ~(a1^a2);
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
