#pragma once

/// \file Swar.h SWAR operations

#include "zoo/swar/metaLog.h"

#include <type_traits>

#include <iostream>


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

/// Core abstraction around SIMD Within A Register (SWAR).  Specifies 'lanes'
/// of NBits width against a type T, and provides an abstraction for performing
/// SIMD operations against that primitive type T treated as a SIMD register.
/// SWAR operations are usually constant time, log(lane count) cost, or O(lane count) cost.
/// Certain computational workloads can be materially sped up using SWAR techniques.
template<int NBits, typename T = uint64_t> struct SWAR {
    static constexpr inline auto LaneCount = sizeof(T) * 8 / NBits;
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
    #undef SWAR_BINARY_OPERATORS_X_LIST

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

// SWAR is a useful abstraction for performing computations in lanes overlaid over any given integral type.
// Doing additions via SWAR techniques requires an extra bit per lane be
// available past the lane size, _or_ knowledge that both of your MSBs are set
// 0 (leaving space for the operation).  Similarly, doing multiplications via
// SWAR techniques require double bits per lane (unless you can bound your
// inputs at half lane size)
// This leads to a useful technique (which we use in the robin hood table)
// where we interleave two related small bit count integers inside of a lane of
// swar.  More generally, this is useful because it allows fast operations on
// side a of a lane if side b is blitted out, and vice versa.  In the spirit of
// separation of concerns, we provide a cut-lane-SWAR abstraction here.

template<int NBitsSideM, int NBitsSideL, typename T = uint64_t> struct SWARWithSubLanes : SWAR<NBitsSideM+NBitsSideL, T> {
  static constexpr inline auto Available = sizeof(T);
  static constexpr inline auto LaneBits = NBitsSideL+NBitsSideM;
  static constexpr inline auto NSlots = Available * 8 / LaneBits;

  SWARWithSubLanes() = default;
  constexpr explicit SWARWithSubLanes(T v) : SWAR<NBitsSideM+NBitsSideL, T>(v) {}
  constexpr explicit operator T() const noexcept { return this->m_v; }

  // U is most significant bits slice, L is least significant bits slice.
  // 0x....U2L2U1L1 or UN|LN||...||U2|L2||U1|L1
  using SL = SWARWithSubLanes<NBitsSideM, NBitsSideL, T>;
  using SD = swar::SWAR<LaneBits, T>;

  //constexpr T Ones = makeBitmask<NBits, T>(SWAR<NBits, T>{1}.value());
  static constexpr inline auto SideLOnes = makeBitmask<LaneBits, T>(SD{1}.value());
  static constexpr inline auto SideMOnes = makeBitmask<LaneBits, T>(SD{1<<NBitsSideL}.value());
  static constexpr inline auto SideLMask = makeBitmask<LaneBits, T>(SD{~(~0ull<<NBitsSideL)}.value());
  static constexpr inline auto SideMMask = ~SideLMask;

  constexpr auto sideL() const noexcept {
    return this->m_v & SideLMask;
  }
  constexpr auto sideL(u32 pos) const noexcept {
    constexpr auto filter = (T(1) << LaneBits) - 1;
    const auto keep = (filter << (LaneBits * pos)) & SideLMask;
    return this->m_v & keep;
  }
  constexpr auto sideLFlat(u32 pos) const noexcept {
    return sideL(pos) >> (LaneBits*pos);
  }
  constexpr auto sideM() const noexcept {
    return this->m_v & SideMMask;
  }
  constexpr auto sideM(u32 pos) const noexcept {
    constexpr auto filter = (T(1) << LaneBits) - 1;
    const auto keep = (filter << (LaneBits * pos)) & SideMMask;
    return this->m_v & keep;
  }
  constexpr auto sideMFlat(u32 pos) const noexcept {
    return sideM(pos) >> (LaneBits*pos)>> NBitsSideL;
  }

  // Sets the lsb sublane at |pos| with NBitsSideL of |in|
  constexpr auto setSideL(T in, u8 pos) const noexcept {
    constexpr auto filter = (T(1) << LaneBits) - 1;
    const auto keep = ~(filter << (LaneBits * pos)) | SideMMask;
    const auto rdyToInsert = this->m_v & keep;
    const auto rval = rdyToInsert | ((in & SideLMask) << (LaneBits * pos));
    //std::cerr << std::hex << "setSideL in " << in << " pos " << u32(pos) << " lmask " << SideLMask << " mmask " << SideMMask << " filter " << filter << " keep " << keep << " ins " << rdyToInsert << " rval " << rval 
// << "\n" ;
    return SL(rval);
    //return SL(SL((this->m_v & keep)).value() | ((in & SideLMask) << (LaneBits * pos)));
    //(in & SideMMask) << (LaneBits * pos)
    //this->m_v & SideMMask
    //return this->m_v.set( pos, 0); //(this->m_v.at(pos) & SideMMask) & in);
  }
    //constexpr T at(int position) const noexcept {
        //constexpr auto filter = (T(1) << LaneBits) - 1;
        //return filter & (this->m_v >> (LaneBits * position));

  constexpr auto setSideM(T in, u8 pos) const noexcept {
    constexpr auto filter = (T(1) << LaneBits) - 1;
    const auto keep = ~(filter << (LaneBits * pos)) | SideLMask;
    const auto rdyToInsert = this->m_v & keep;
    const auto insVal = (((in<<NBitsSideL) & SideMMask) << (LaneBits * pos));
    const auto rval = rdyToInsert | insVal;
    //std::cerr << std::hex << "setSideM in " << in << " pos " << u32(pos) << " lmask " << SideLMask << " mmask " << SideMMask << " filter " << filter << " keep " << keep << " ins " << rdyToInsert << " insval " << insVal << " rval " << rval 
  // << "\n" ;
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

/// Clears the lowest bit set in type T
template<typename T = uint64_t>
constexpr auto clearLSB(T v) {
  return v & (v - 1);
}

/// Leaves on the lowest bit set, or all 1s for a 0 input.
template<typename T = uint64_t>
constexpr auto isolateLSB(T v) {
  return v & ~clearLSB(v);
}

template<int NBits, typename T = uint64_t>
constexpr auto lowestNBitsMask() {
  return (T(1)<<NBits)-1;
  //return ~((~T(0))<<NBits);
}

template<int NBits, uint64_t T>
constexpr auto lowestNBitsMask() {
  // return (u64(1)<<NBits)-1;
  return ~((0ull)<<NBits);
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

/// Broadcasts the value in the 0th lane of the SWAR to the entire SWAR.
/// Precondition: 0th lane of |v| contains a value to broadcast, remainder of input SWAR zero.
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
