#pragma once

#include "zoo/swar/SWAR.h"
#include "zoo/map/RobinHoodUtil.h"

#include <vector>

namespace zoo {

namespace rh {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

// The naive method of keeping metadata.
// TODO(sbruce) actually have correct widths of psl/hash for direct comparison
// to real metadata.
struct StrawMetadata {
    StrawMetadata(u32 sz) : sz_(sz), psls_(sz), hashes_(sz) {}
    u32 sz_;
    std::vector<u8> psls_;
    std::vector<u8> hashes_;
    u32 size() const { return sz_; }
    auto PSL(u32 pos) const { return psls_[pos]; }
    auto Hash(u32 pos) const { return hashes_[pos]; }
};

template<u8 PSLBits, u8 HashBits, typename Key> struct StrawmanMap {
  template <typename KeyCheck>
  bool exists(Key k, KeyCheck kc) {
    return findSlot(k, kc).second;
  }

  // Slot that key exists in or should exist in, true means found, false not
  // found.
  template <typename KeyCheck>
  std::pair<int, bool> findSlot(Key k, KeyCheck kc) {
      u32 hashbits = reducedhashUnitary(k);
      u32 slot = slotFromKeyUnitary<u32>(k);
      auto psl = 0;
      while (true) {
        ++psl;  // bias psl by one, convenient to do it here
        auto offset = slot+psl-1;
        u32 tablePSL = md_.psls_[offset];
        if (tablePSL > psl) { // Definitely not, continue.
          continue;
        }
        if (tablePSL < psl) { // definite miss.
          return {offset, false};
        }
        if (tablePSL == psl) { // maybe
          if (md_.hashes_[offset] == hashbits) {
            if(kc(k, keys_[offset])) {
              return {offset, true};
            }
          }
        }
      }
    }

    template <typename KeyCheck> 
    std::pair<int, bool> insert(Key k, KeyCheck kc) {
        u32 hashbits = reducedhashUnitary(k);
        u32 slot = slotFromKeyUnitary<u32>(k);
        auto slotFind = findSlot(k, kc);
        if (slotFind.second) {
            return {slotFind.first, false};  // already in
        }

        // Evict loop.
        int offset = slotFind.first;
        while(true) {
            if (md_.psls_[offset] == 0) {
                // easy insert
                md_.psls_[offset] = offset-slot+1;
                md_.hashes_[offset] = hashbits;
                keys_[offset] = k;
                // incorrect returns last key inserted.
                return {offset, true};
            }
            // Evicting insert.
            // tmpvar meta/key
            // insert this key
            // cycle insert next key
            auto tmppsl = md_.psls_[offset];
            auto tmphash = md_.hashes_[offset];
            auto tmpkey = keys_[offset];
            md_.psls_[offset] = offset-slot;
            md_.hashes_[offset] = hashbits;
            keys_[offset] = k;
            slot = offset-tmppsl;
            hashbits = tmphash;
            k = tmpkey;
            offset+=1;
        }
    }

    StrawmanMap(std::size_t sz) : sz_(sz), md_(sz), keys_(sz) {}

    std::size_t sz_;
    StrawMetadata md_;
    std::vector<Key> keys_;
};

}}

