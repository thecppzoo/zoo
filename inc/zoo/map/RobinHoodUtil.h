#pragma once

#include "zoo/swar/SWAR.h"

#include <array>
#include <cstddef>

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

template<typename T>
struct MisalignedGenerator_Dynamic {
    constexpr static auto Width = sizeof(T) * 8;
    T *base_;

    int misalignmentFirst, misalignmentSecond;

    MisalignedGenerator_Dynamic(T *base, int ma):
        base_(base),
        misalignmentFirst(ma), misalignmentSecond(Width - ma)
    {}
    

    constexpr T operator*() noexcept {
        auto firstPart = base_[0];
        auto secondPart = base_[1];
            // how to make sure the "logical" shift right is used, regardless
            // of the signedness of T?
            // I'd prefer to not use std::make_unsigned_t, since how do we
            // "make unsigned" user types?
        auto firstPartLowered = firstPart.value() >> misalignmentFirst;
        auto secondPartRaised = secondPart.value() << misalignmentSecond;
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
        T(GoldenRatioReciprocals[meta::logFloor(sizeof(T)) - 3]);
    return index * MagicalConstant;
}

// Key needs to change to an integer.
template<typename Key, typename T = u64> T fibhash(Key k) noexcept {
    constexpr std::array<uint64_t, 4>
        GoldenRatioReciprocals = {
            159,
            40503,
            2654435769,
            11400714819323198485ull,
        };
    constexpr T GoldenConstant =
        T(GoldenRatioReciprocals[meta::logFloor(sizeof(T)) - 3]);
    return k * GoldenConstant;
}

template<size_t Size, typename T>
constexpr auto lemireModuloReductionAlternative(T input) noexcept {
    static_assert(sizeof(T) == sizeof(uint64_t));
    static_assert(Size < (1ull << 32));
    auto halved = uint32_t(input);
    return Size * halved >> 32;
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
