#pragma once

#include "zoo/swar/SWAR.h"

#include <iostream>
#include <vector>

namespace zoo {

namespace rh {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

template<int NBits>
constexpr auto cheapOkHash(u64 n) {
  constexpr auto shift = (NBits * ((64 / NBits)-1));
  constexpr u64 allOnes = swar::makeBitmask<NBits, u64>(swar::SWAR<NBits, u64>{1}.value());
  auto temporary = allOnes * n;
  auto higestNBits = temporary >> shift;
  return (0 == (64 % NBits)) ? higestNBits : swar::isolate<NBits, u64>(higestNBits);
}

// The naive method of keeping metadata.
template<u8 PSLBits, u8 HashBits> struct StrawMetadata {
  StrawMetadata(u32 sz) : psls_(sz), hashes_(sz) {}
  std::vector<u8> psls_;
  std::vector<u8> hashes_;

};

// Structure keeping PSLs in the low bits and Hash in the high bits (as we want
// to cmp psl bits for cheap with known blitted out hash bits so their MSBs are
// off.
template<u8 PSLBits, u8 HashBits> struct RobinHoodMetadataBlock {
  using SW = swar::SWARWithSubLanes<PSLBits, HashBits>; 
  SW m_; 
  constexpr auto PSLs() { return m_.sideL(); }
  constexpr auto Hashes() { return m_.sideM(); }
};

template<u8 PSLBits, u8 HashBits> struct RobinHoodMetadata {
  using Block = RobinHoodMetadataBlock<PSLBits, HashBits>;
  std::vector<Block> blocks_;
};

// These two functions are for testing: If they are correct, then any table
// under test can be converted to a completely independent implementation to
// verify any operation.

template<u8 PSLBits, u8 HashBits>
RobinHoodMetadata<PSLBits, HashBits> strawToReal(const StrawMetadata<PSLBits, HashBits>& straw) {
  RobinHoodMetadata<PSLBits, HashBits> actual;
  
  auto sz = straw.psls_.size();
  actual.blocks_.resize(sz/RobinHoodMetadata<PSLBits, HashBits>::LaneCount);
  for (auto idx = 0 ; idx < sz; ++idx) {
    auto lane = idx / RobinHoodMetadata<PSLBits, HashBits>::LaneCount;
    auto psl =straw.psls_[idx]; 
    auto h = straw.hashes_[idx];
    actual.blocks_[idx/RobinHoodMetadata<PSLBits, HashBits>::LaneCount].setSideL(psl, lane);
    actual.blocks_[idx/RobinHoodMetadata<PSLBits, HashBits>::LaneCount].setSideM(h, lane);
  }

  return actual;
}

template<u8 PSLBits, u8 HashBits>
StrawMetadata<PSLBits, HashBits> realToStraw(const RobinHoodMetadata<PSLBits, HashBits> & actual) {
  StrawMetadata<PSLBits, HashBits> straw;
  for (auto block : actual.blocks_) {
    for (auto inner = 0 ; inner < RobinHoodMetadata<PSLBits, HashBits>::LaneCount;inner++) {
    straw.psls_.push_back(block.PSLs[inner]);
    straw.hashes.push_back(block.Hashes[inner]);
    }
  }
  return straw;
}


template<typename Key>auto fibhash(Key k) noexcept {
  return 1;
}

template<typename Key>auto badmix(Key k) noexcept {
  return 1;
}

template<typename Key>auto reducedhash(Key k) noexcept {
  return badmix(k);
}

template<typename Key>auto slotFromKey(Key k) noexcept {
  return 1;
}

template<u8 PSLBits, u8 HashBits, typename Key> struct StrawmanMap {
  
  template <typename KeyCheck>
  bool exists(Key k, KeyCheck kc) {
    return findSlot(k, kc).second;
  }

  // Slot that key exists in or should exist in, true means found, false not
  // found.
  template <typename KeyCheck>
  std::pair<int, bool> findSlot(Key k, KeyCheck kc) {
    u32 hashbits = reducedhash(k);
    u32 slot = slotFromKey<u32>(k);
    auto psl = 0;
    while (true) {
      ++psl;  // bias psl by one, convenient to do it here
      auto offset = slot+psl-1;
      u32 tablePSL = md_.psls_[offset];
      std::cerr << "findSlot psl " << psl << " offset " << offset << " tpsl " << tablePSL << " hbits " << hashbits << "\n";
      if (tablePSL > psl) { // Definitely not, continue.
            std::cerr << "findSlot continue \n";
        continue;
      }
      if (tablePSL < psl) { // definite miss.
            std::cerr << "findSlot definite miss offset " << offset << " table psl " << tablePSL << " psl " << psl << " \n";
        return std::make_pair(offset, false);
      }
      if (tablePSL == psl) { // maybe
            std::cerr << "findSlot psl match \n";
        if (md_.hashes_[offset] == hashbits) {
            std::cerr << "findSlot hash match \n";
          if(kc(k, keys_[offset])) {
            std::cerr << "findSlot keycheck match key " << k << " psl " << psl << " offset " << offset << " tpsl " << tablePSL << " hbits " << hashbits << "\n";
            return std::make_pair(offset, true);
          }
        }
      }
    }
  }

  template <typename KeyCheck>
   std::pair<int, bool> insert(Key k, KeyCheck kc) {
    u32 hashbits = reducedhash(k);
    u32 slot = slotFromKey<u32>(k);
    auto slotFind = findSlot(k, kc);
std::cerr << "insert slot find " << slotFind.first << " " << slotFind.second << "\n";
    if (slotFind.second) {
std::cerr << "key " << k << " already in table at " << slotFind.first << "\n";
 return std::make_pair(slotFind.first, false);  // already in
}

    // Evict loop.
    int offset = slotFind.first;
    while(true) {
      if (md_.psls_[offset] == 0) {
        // easy insert
        md_.psls_[offset] = offset-slot+1;
        md_.hashes_[offset] = hashbits;
        keys_[offset] = k;
std::cerr << "key " << k << " inserted at " << offset << " psl "  << u32(md_.psls_[offset]) << " hash "  << u32(md_.hashes_[offset]) << " key "  << keys_[offset]  << "\n";
        // incorrect returns last key inserted.
        return std::make_pair(offset, true);
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
  StrawMetadata<PSLBits, HashBits> md_;
  std::vector<Key> keys_;
};

}}

