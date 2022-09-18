#pragma once

#include "zoo/swar/SWAR.h"

#include <array>
#include <cstddef>
#include <functional>

namespace zoo {

template<typename T>
struct GeneratorFromPointer {
    T *p_;

    constexpr T &operator*() noexcept { return *p_; }
    constexpr GeneratorFromPointer operator++() noexcept { return {++p_}; }
};

template<typename T, int MisalignmentBits>
struct MisalignedGenerator {
    T *base_;
    constexpr static auto Width = sizeof(T) * 8;

    constexpr T operator*() noexcept {
        auto firstPart = base_[0];
        auto secondPart = base_[1];
            // how to make sure the "logical" shift right is used, regardless
            // of the signedness of T?
            // I'd prefer to not use std::make_unsigned_t, since how do we
            // "make unsigned" user types?
        auto firstPartLowered = firstPart.value() >> MisalignmentBits;
        auto secondPartRaised = secondPart.value() << (Width - MisalignmentBits);
        return T{firstPartLowered | secondPartRaised};
    }

    constexpr MisalignedGenerator operator++() noexcept { return {++base_}; }
};

template<typename T>
struct MisalignedGenerator<T, 0>: GeneratorFromPointer<T> {};

// This is tightly coupled with a Metadata that happens-to have lane widths of
// 8.
template<typename T>
struct MisalignedGenerator_Dynamic {
    constexpr static auto Width = sizeof(T) * 8;
    T *base_;

    int misalignmentFirst, misalignmentSecondLessOne;

    MisalignedGenerator_Dynamic(T *base, int ma):
        base_(base),
        misalignmentFirst(ma), misalignmentSecondLessOne(Width - ma - 1)
    {}

    constexpr T operator*() noexcept {
        auto firstPart = base_[0];
        auto secondPart = base_[1];
            // how to make sure the "logical" shift right is used, regardless
            // of the signedness of T?
            // I'd prefer to not use std::make_unsigned_t, since how do we
            // "make unsigned" user types?
        auto firstPartLowered = firstPart.value() >> misalignmentFirst;
        // Avoid undefined behavior of << width of type.
        auto secondPartRaised = (secondPart.value() << misalignmentSecondLessOne) << 1;
        return T{firstPartLowered | secondPartRaised};
    }

    constexpr MisalignedGenerator_Dynamic operator++() noexcept {
        ++base_; return *this;
    }
};

namespace rh {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

namespace impl {

/// \todo decide on whether to rename this?
template<int PSL_Bits, int HashBits, typename U>
struct Metadata: swar::SWARWithSubLanes<PSL_Bits, HashBits, U> {
    using Base = swar::SWARWithSubLanes<PSL_Bits, HashBits, U>;
    using Base::Base;
    static constexpr auto MaxPSL = 1 << PSL_Bits;

    constexpr auto PSLs() const noexcept { return this->least(); }
    constexpr auto hashes() const noexcept { return this->most(); }
};

template<int PSL_Bits, int HashBits, typename U>
struct MatchResult {
    U deadline;
    Metadata<PSL_Bits, HashBits, U> potentialMatches;
};

} // impl

template<int NBits, typename U>
constexpr auto hashReducer(U n) noexcept {
    constexpr auto Shift = (NBits * ((sizeof(U)*8 / NBits) - 1));
    constexpr auto AllOnes = meta::BitmaskMaker<U, 1, NBits>::value;
    auto temporary = AllOnes * n;
    auto higestNBits = temporary >> Shift;
    return
        (0 == (64 % NBits)) ?
            higestNBits :
            swar::isolate<NBits>(higestNBits);
}


template<typename T>
constexpr auto fibonacciIndexModulo(T index) {
    constexpr std::array<uint64_t, 4>
        GoldenRatioReciprocals = {
            159,
            40503,
            2654435769,
            11400714819323198485ull,
        };
    constexpr T MagicalConstant =
        T(GoldenRatioReciprocals[meta::logFloor(sizeof(T))]);
    return index * MagicalConstant;
}

template<size_t Size, typename T>
constexpr auto lemireModuloReductionAlternative(T input) noexcept {
    static_assert(sizeof(T) == sizeof(uint64_t));
    constexpr T MiddleBit = 1ull << 32;
    static_assert(Size < MiddleBit);
    auto lowerHalf = input & (MiddleBit - 1);
    return Size * lowerHalf >> 32;
}

// Scatters a range onto itself
template<typename T>
struct FibonacciScatter {
    constexpr auto operator()(T index) noexcept {
      return fibonacciIndexModulo(index);
    }
};

// Reduces an int onto a range via Lemire reduction.
template<size_t Size, typename T>
struct LemireReduce {
    constexpr auto operator()(T input) noexcept {
      return lemireModuloReductionAlternative<Size, T>(input);
    }
};

// Reduces an input value of U to NBits width, via ones multiply and top bits.
template<int NBits, typename U>
struct TopHashReducer {
    constexpr auto operator()(U n) noexcept {
        return hashReducer<NBits>(n);
    }
};

// Often used as:
// UnitaryHash<u64>, UnitaryScatter<u64>, UnitaryReduce<u64>, UnitaryReducer<u64>
// Hash that is always 1.
template<typename T>
struct UnitaryHash {
    constexpr auto operator()(const T& v) noexcept { return 1; }
};

// Scatters a range to always be 1
template<typename T>
struct UnitaryScatter {
    constexpr auto operator()(T index) noexcept { return 1; }
};

// Reduces an int onto a range but that range is always 1.
template<typename T>
struct UnitaryReduce {
    constexpr auto operator()(T input) noexcept { return 1; }
};

// Reduces an input value of U to NBits width, but that value is always 1.
template<typename U>
struct UnitaryReducer {
    constexpr auto operator()(U n) noexcept { return 1; }
};

/// Given a key and sufficient templates specifying its transformation process,
/// return hoisted hash bits and home index in table.
template<
    typename K,
    size_t RequestedSize,
    int HashBits,
    typename U = std::uint64_t,
    typename Hash = std::hash<K>,
    typename Scatter = FibonacciScatter<U>,
    typename RangeReduce = LemireReduce<RequestedSize, U>,
    typename HashReduce = TopHashReducer<HashBits, U> >
static constexpr auto findBasicParameters(const K&k) noexcept {
    auto hashCode = Hash{}(k);
    auto scattered = Scatter{}(hashCode);
    auto homeIndex = RangeReduce{}(scattered);
    auto hoisted = HashReduce{}(hashCode);
    return std::tuple{hoisted, homeIndex};
}

template<int NBits>
constexpr auto cheapOkHash(u64 n) noexcept {
    constexpr auto shift = (NBits * ((64 / NBits)-1));
    constexpr u64 allOnes = meta::BitmaskMaker<u64, 1, NBits>::value;
    auto temporary = allOnes * n;
    auto higestNBits = temporary >> shift;
    return (0 == (64 % NBits)) ?
        higestNBits : swar::isolate<NBits, u64>(higestNBits);
}

/// Does some multiplies with a lot of hash bits to mix bits, returns only a few
/// of them.
template<int NBits> auto badMixer(u64 h) noexcept {
    constexpr u64 allOnes = ~0ull;
    constexpr u64 mostSigNBits = swar::mostNBitsMask<NBits, u64>();
    auto tmp = h * allOnes;

    auto mostSigBits = tmp & mostSigNBits;
    return mostSigBits >> (64 - NBits);
}

/// Evenly map a large int to an int without division or modulo.
template<int SizeTable, typename T>
constexpr int mapToSlotLemireReduction(T halved) {
    // TODO: E and S think that the upper bits of are higher quality entropy,
    // explore at some point.
    return (halved * T(SizeTable)) >> (sizeof(T)/2);
}

namespace impl {

template<typename MetadataCollection>
auto peek(const MetadataCollection &collection, size_t index) {
    using MD =
        std::remove_const_t<std::remove_reference_t<decltype(collection[0])>>;
    auto swarIndex = index / MD::NSlots;
    auto intraIndex = index % MD::NSlots;
    auto swar = collection[swarIndex];
    return
        std::tuple{
            swar.leastFlat(intraIndex),
            swar.mostFlat(intraIndex)
        };
}

template<typename MetadataCollection >
void poke(MetadataCollection &collection, size_t index, u64 psl, u64 hash) {
    using MD = std::remove_reference_t<decltype(collection[0])>;
    auto swarIndex = index / MD::NSlots;
    auto intraIndex = index % MD::NSlots;
    auto swar = collection[swarIndex];
    auto newPSL = swar.least(psl, intraIndex);
    auto replacement = newPSL.most(hash, intraIndex);
    collection[swarIndex] = replacement;
}

} // impl
} // namespace rh
} // namespace zoo
