
#pragma once

#include "zoo/swar/SWAR.h"

#include <array>
#include <cstddef>

namespace zoo {

namespace rh {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

template<int NBitsHash, int NBitsPSL, typename T = u64> struct SlotOperations {
    using SSL = swar::SWARWithSubLanes<NBitsHash, NBitsPSL, T>;
    using SM = swar::SWAR<NBitsHash + NBitsPSL, T>;
    using BoolSM = swar::BooleanSWAR<NBitsHash + NBitsPSL, T>;
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
        auto broken = ~satisfied;
        return swar::isolateLSB(broken.value());
    }

    // Has no intrinsic binding to the metadata, just easier to write with
    // using decls.
    static constexpr auto attemptMatch(
             SM haystack, SM needleHashes, SM needlePSL) {
        const auto haystackPSL = SM{SSL::LeastMask.value() & haystack.value()};
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
    using SM = swar::SWAR<NBitsHash + NBitsPSL, T>;
    using BoolSM = swar::BooleanSWAR<NBitsHash + NBitsPSL, T>;
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

/// BlockProvider provides affordances of
///     'give me the slot metadata that contains position p'
///     'set slot metadata at index that contains position p'
///     'set key at position p'
///     'check this concrete key against position p'
template<int NBitsHash, int NBitsPSL, int Size, typename T = u64, typename Key = u64> struct SlotSplitKeys {
    using SSL = swar::SWARWithSubLanes<NBitsHash, NBitsPSL, T>;
    using SM = swar::SWAR<NBitsHash+ NBitsPSL, T>;
    using BoolSM = swar::BooleanSWAR<NBitsHash+ NBitsPSL, T>;

    std::array<Key,Size > keys_;
    std::array<T, Size/SM::Lanes> metadata_;
    
    Key keyAt(int pos) const {
      return keys_[pos];
    }
    void setKey(int pos, Key k) {
      keys_[pos] = k;
    }
    constexpr int posToSlot(int pos) const { return pos/SM::Lanes; }
    void setSlotAt(int idx, T v) {
       metadata_[idx] = v;
    }

    // Track to avoid second divide?
    SSL slotAt(int pos) const { return metadata_[posToSlot(pos)]; }
    bool keyCheck(int major, int minor, Key k) const { return k == keyAt(major * SM::Lanes + minor); }
    bool keyCheck(int pos, Key k) const { return k == keyAt(pos); }
};

/// RobinHood tables provide affordances of
///     'lookup this key'
///     'insert this key'
///     'delete this key'
/// Split locale of key value when both are present.
/// Position is major + minor

template<typename Key> 
struct Hasher {
    Key operator()(Key k) { return k; }
};

template<int NBitsHash, int NBitsPSL, int Size, typename Meta, typename T = u64, typename Key = u64, typename Hash = Hasher<Key>> 
struct RH {
    using SSL = swar::SWARWithSubLanes<NBitsHash, NBitsPSL, T>;
    using SM = swar::SWAR<NBitsHash+ NBitsPSL, T>;
    using BoolSM = swar::BooleanSWAR<NBitsHash+ NBitsPSL, T>;
    Meta m_;
    Hash h_;

    bool exists(Key k) {
      return findSlot(k).second;
    }

    struct MajorMinorInserted {
        int major;
        int minor;
        bool inserted;
    };

    /// pos, hash
    std::pair<u32, T> twoNumbers(Key k) {
        auto hash = h_(k);
        const u32 pos = mapToSlotLemireReduction<Size, T>(fibonacciIndexModulo(hash));
        const T thinhash = badMixer<NBitsHash> (hash);
        return {pos, thinhash};
    }

    std::pair<u32, T> twoNumbersBad(Key k) {
        return {1, 1};
    }

    /// Find slot can mean 'psl too short/no slot', 'found and in slot', 'not found but (richer|empty) slot'
    /// Currently bug: 'psl too short/no slot' not handled correctly.
    /// Returns major/minor to attempt to avoid divisions.
    template <typename KeyCheck> 
    MajorMinorInserted findSlot(Key k, KeyCheck kc) {
      const auto twoNum = twoNumbersBad(k);
      const auto pos = twoNum.first;
      const auto hash = twoNum.second;
      const auto major = m_.posToSlot(pos);
      const auto haystack = m_.slotAt(pos);
      // PSL is off by one territory
      constexpr auto exactlyOne = SM{1};
      auto minor = 0;
      while(true) {
          const auto matchResult = SlotOperations<NBitsHash, NBitsPSL, T>::matchAttempt(haystack, hash, 0);
          auto finished = exactlyOne & matchResult;
          auto matches = exactlyOne & ~matchResult;
          while (matches) {
              auto minor = matches.lsbIndex();  // Lane offset
              if (m_.keyCheck(major + minor, k)) { 
                  return {major, minor, true};  // 'found and in slot'
              }
              matches = SM{matches.clearLSB()};
          }
          // minor points at slot that the key currently fits in.
          if (finished) {
              return {major, minor, false};  // 'not found, richer or empty slot'
          }
      }
    }
};


} // namespace rh
} // namespace zoo
