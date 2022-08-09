#pragma once

#include "zoo/swar/SWAR.h"

#include <cassert>
#include <iomanip>
#include <ios>
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
  StrawMetadata(u32 sz) : sz_(sz), psls_(sz), hashes_(sz) {}
  u32 sz_;
  std::vector<u8> psls_;
  std::vector<u8> hashes_;
  u32 size() const { return sz_; }
};

// Structure keeping PSLs in the low bits and Hash in the high bits (as we want
// to cmp psl bits for cheap with known blitted out hash bits so their MSBs are
// off.
template<u8 PSLBits, u8 HashBits, typename T> struct RobinHoodMetadataBlock {
  using SW = swar::SWARWithSubLanes<PSLBits, HashBits, T>; 
  static constexpr auto NSlots = SW::NSlots;
  SW m_; 
  constexpr auto PSLs() { return m_.sideL(); }
  constexpr auto Hashes() { return m_.sideM(); }
  constexpr auto PSL(u32 pos) { return m_.sideLFlat(pos); }
  constexpr auto Hash(u32 pos) { return m_.sideMFlat(pos); }
};

template<u8 PSLBits, u8 HashBits, typename T> struct RobinHoodMetadata {
  using Block = RobinHoodMetadataBlock<PSLBits, HashBits, T>;
  using SW = swar::SWARWithSubLanes<PSLBits, HashBits, T>; 
  std::vector<Block> blocks_;
  u32 sz_ = 0;
 
  void setValue(u32 psl, u32 hash, u32 idx) {
    const auto vecpos = idx / Block::NSlots;
    const auto lane = idx % Block::NSlots;
    std::cerr << "setValue idx " << idx << " psl " << psl << " hash " << hash << "\n";
    auto preblock = blocks_[vecpos].m_.value();
    auto setPSL = blocks_[vecpos].m_.setSideL(psl, lane).value();
    auto setHash = blocks_[vecpos].m_.setSideM(hash, lane).value();
    std::cerr << std::hex << std::setw(16) << std::setfill('0')<< "setValue pre " << preblock << " setPSL " << setPSL << " setHash " << setHash << " " << (setHash | setPSL) << "\n";
    blocks_[vecpos].m_ = SW{setHash | setPSL};
  }
  constexpr auto PSL(u32 idx) { 
    const auto vecpos = idx / Block::NSlots;
    const auto lane = idx % Block::NSlots;
    return blocks_[vecpos].sideL(lane);
  }
  constexpr auto Hash(u32 idx) { 
    const auto vecpos = idx / Block::NSlots;
    const auto lane = idx % Block::NSlots;
    return blocks_[vecpos].sideM(lane); 
  }
};

template<u8 PSLBits, u8 HashBits, typename T>
void dumpRealMeta(const RobinHoodMetadata<PSLBits, HashBits, T> & actual) {
  std::cerr << "rh meta sz " << actual.sz_ << " blocks: ";
  for (auto block : actual.blocks_) {
    std::cerr << std::hex << std::setfill('0') << std::setw(16) << block.m_.value() << " - ";
  }
  std::cerr << "\n";
}

// These two functions are for testing: If they are correct, then any table
// under test can be converted to a completely independent implementation to
// verify any operation.

template<u8 PSLBits, u8 HashBits, typename T>
RobinHoodMetadata<PSLBits, HashBits, T> strawToReal(const StrawMetadata<PSLBits, HashBits>& straw) {
  using RHM = RobinHoodMetadata<PSLBits, HashBits, T>; 

  RobinHoodMetadata<PSLBits, HashBits, T> actual;
  auto sz = straw.psls_.size();
  actual.sz_=straw.sz_;
  assert(sz == straw.sz_);
  u32 actualBlocks = sz/RobinHoodMetadata<PSLBits, HashBits, T>::Block::NSlots + 1;
  actual.blocks_.resize(actualBlocks);
  u32 count = 0;
  std::cerr << "strawToReal actualBlocks " << actualBlocks << " strawsize " << sz << "\n";
  for (auto idx = 0 ; idx < sz; ++idx) {
    u32 psl = straw.psls_[idx]; 
    u32 h = straw.hashes_[idx];
    actual.setValue(psl, h, idx);
    std::cerr << "strawToReal count " << count << " idx " << idx << " psl " << psl << " hash " << h << "\n";
    count ++;
  }
  for (auto block : actual.blocks_) {
    std::cerr <<std::hex<< actual.blocks_[0].m_.value() << "\n";
  }
  dumpRealMeta(actual);
  return actual;
}


template<u8 PSLBits, u8 HashBits, typename T>
StrawMetadata<PSLBits, HashBits> realToStraw(const RobinHoodMetadata<PSLBits, HashBits, T> & actual) {
  constexpr auto lanecount = RobinHoodMetadata<PSLBits, HashBits, T>::Block::NSlots;
  StrawMetadata<PSLBits, HashBits> straw(actual.sz_);
  straw.sz_ = actual.sz_;
  u32 count = 0;
  for (auto block : actual.blocks_) {
    std::cerr << "Block \n";
    for (auto inner = 0; inner < lanecount ;inner++) {
      const u32 psl  =block.PSL(inner);
      const u32 hash =block.Hash(inner);
      std::cerr << "realToStraw count " << count << " psl " << psl << " hash " << hash << "\n";
      straw.psls_[count] = psl;
      straw.hashes_[count] = hash;
      if (count == actual.sz_) break;
      count ++;
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

