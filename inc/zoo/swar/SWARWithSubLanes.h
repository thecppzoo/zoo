#ifndef ZOO_SWAR_SWARWITHSUBLANES_H
#define ZOO_SWAR_SWARWITHSUBLANES_H

#include "zoo/swar/SWAR.h"

namespace zoo { namespace swar {

/// \brief Allows SWAR Lanes to be treated both as a whole or something with
/// internal structure.

/// Example: Robin Hood "Haystack" metadata composed of hoisted hash bits and
/// PSL (probe sequence lengths), that are used together or separately.
/// SWAR is a useful abstraction for performing computations in lanes overlaid
/// over any given integral type.
/// To prevent the normal integer operations in a lane to disrrupt the operation
/// in the adjoining lanes, some precautions must be maintained.  For example
/// upon an addition of lanes, we either need that the domain of our values
/// does not use the most significant bit (guaranteeing normal addition of
/// lanes won't cross to the upper lane) or that this possibility is explicitly
/// taken into account (see "full addition").  This applies to all operations,
/// including comparisons.
/// Similarly, doing multiplications via SWAR techniques require double bits per
/// lane (unless you can guarantee the values of the input lanes are half lane
/// size).
/// This leads to a useful technique (which we use in the Robin Hood table)
/// where we interleave two related small bit count integers inside of a lane of
/// swar.  More generally, this is useful because it sometimes allows fast
/// operations on side "a" of some lane if side "b" is blitted out, and vice
/// versa.  In the spirit of separation of concerns, we provide a cut-lane-SWAR
/// abstraction here.
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

}}

#endif
