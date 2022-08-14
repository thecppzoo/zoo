#pragma once

#include "zoo/swar/SWAR.h"

#include <array>
#include <cassert>
#include <vector>

namespace zoo {

namespace rh {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

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

template<typename Key, typename T = u64> T fibhash(Key k) noexcept {
    constexpr auto FibonacciGoldenRatioReciprocal64 = 11400714819323198485ull;
    constexpr auto FibonacciGoldenRatioReciprocal32 = 2654435769u;
    constexpr auto FibonacciGoldenRatioReciprocal16 = 40503u;
    constexpr auto FibonacciGoldenRatioReciprocal8 = 159u;
    if constexpr(sizeof(Key) == 8) {
        u64 r = k * FibonacciGoldenRatioReciprocal64;
        return r; 
    } else if constexpr(sizeof(Key) == 4) {
        u32 r = k * FibonacciGoldenRatioReciprocal32; 
        return r;
    } else if constexpr(sizeof(Key) == 2) {
        u16 r = k * FibonacciGoldenRatioReciprocal16;
        return r;
    } else if constexpr(sizeof(Key) == 1) {
        u8 r = k * FibonacciGoldenRatioReciprocal8;
        return r;
    }
    assert(false);
}

/// Evenly map a large int to an int without division or modulo.
template<int SizeTable, typename T>
constexpr int mapToSlotLemireReduction(T halved) {
    return (halved * T(SizeTable)) >> (sizeof(T)/2); 
}


template<typename Key>auto reducedhashUnitary(Key k) noexcept {
    return 1;
}

template<typename Key>auto slotFromKeyUnitary(Key k) noexcept {
    return 1;
}

template<int NBitsHash, int NBitsPSL, typename T = u64> struct SlotOperations {
    using SSL = swar::SWARWithSubLanes<NBitsHash, NBitsPSL, T>;
    using SM = swar::SWAR<NBitsHash+ NBitsPSL, T>;
    using BoolSM = swar::BooleanSWAR<NBitsHash+ NBitsPSL, T>;
    static constexpr inline auto SlotOnes =
        meta::BitmaskMaker<T, SM{1}.value(), NBitsHash + NBitsPSL>::value;
    // for 64 bit size, 8 bit meta, 0x0807'0605'0403'0201ull
    static constexpr inline auto PSLProgression = SlotOnes * SlotOnes;
    static constexpr T allOnes =
        meta::BitmaskMaker<T, 1, NBitsHash+NBitsPSL>::value;

    static constexpr auto needlePSL(T currentPSL) {
        return broadcast(SM{currentPSL}) + SM{PSLProgression};
    }

    // At the position the needle psl exceeds the haystack psl, a match becomes
    // impossible. Only elements _before_ the exceeding element can match.
    // pslNeedle must be PSLProgression + startingPSLValue
    static constexpr auto deadline(SM pslHaystack, SM pslNeedle) {
        // We must ensure the MSBs of the psl blocks are off. Since we store
        // PSLs in a swar with sublanes in the least bits, this guarantee
        // holds.
        auto satisfied = greaterEqual_MSB_off(pslHaystack, pslNeedle);
        auto broken = not satisfied;
        return swar::isolateLSB(broken.value());
    }

    // Has no intrinsic binding to the metadata, just easier to write with
    // using decls.
    static constexpr auto attemptMatch(
             SM haystack, SM needleHashes, SM needlePSL) {
        const auto haystackPSL = SM{SSL::LeastMask & haystack.value()};
        const auto d = deadline(haystackPSL, needlePSL);  // breaks abstraction
        const auto needle = needleHashes | needlePSL;

        const auto matches = equals(haystack, needle);
        const auto searchEnds = d ? 1 : 0; 
        // Returned value is MSB boolean array with 'finality' bit on at
        // position 0. Breaks if PSLs are width 1 (but so does everything else)
        return SM{searchEnds | ((d - 1) & matches.value())};
    }
};

/// Slot metadata provides the affordances of 
///     'attempt to match this needle of hashes and PSLs'
/// Contains sizeof(T)/(NBitsHash,NBitsPSL) hashes and PSLs
template<int NBitsHash, int NBitsPSL, typename T = u64> struct SlotMetadata {
    using SSL = swar::SWARWithSubLanes<NBitsHash, NBitsPSL, T>;
    using SM = swar::SWAR<NBitsHash+ NBitsPSL, T>;
    using BoolSM = swar::BooleanSWAR<NBitsHash+ NBitsPSL, T>;
    // PSL are stored in least sig bits, Hashes are stored in most sig bits.
    // This allows us to do fast greaterequal checks for PSLs (with the hashes
    // blitted out) and fast equality checks for hash bits (as equality checks
    // do not need carry bits.
    SSL data_;

    constexpr auto PSLs() const noexcept{
        return data_.least();
    }

    constexpr auto Hashes() const noexcept {
        return data_.most();
    }

    constexpr auto attemptMatch(SM needleHashes, SM needlePSL) {
        return SlotOperations<NBitsHash, NBitsPSL, T>::attemptMatch(
              data_, needleHashes, needlePSL);
    }
};

}}  // namespace zoo, namespace rh
