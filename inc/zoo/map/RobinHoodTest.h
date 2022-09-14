#pragma once

#include "zoo/swar/SWAR.h"
#include "zoo/map/RobinHoodUtil.h"
#include "zoo/map/RobinHood_straw.h"

#include <vector>

namespace zoo {

namespace rh {

// The naive method of keeping metadata.
// TODO(sbruce) actually have correct widths of psl/hash for direct comparison
// to real metadata.
/* struct StrawMetadata {
    StrawMetadata(u32 sz) : sz_(sz), psls_(sz), hashes_(sz) {}
    u32 sz_;
    std::vector<u8> psls_;
    std::vector<u8> hashes_;
    u32 size() const { return sz_; }
    auto PSL(u32 pos) const { return psls_[pos]; }
    auto Hash(u32 pos) const { return hashes_[pos]; }
};
    std::size_t sz_;
    StrawMetadata md_;
    std::vector<Key> keys_;
*/
// template<u8 PSLBits, u8 HashBits, typename Key> struct StrawmanMap {

template<typename StrawMap, typename RealMap>
int copyMetadata(StrawMap& from, RealMap& to) {
  int count = 0;
  for (int i =0;i < from.sz_;i++) {
      auto psl = from.md_.psls_[i];
      if (psl) {
          poke(to.md_, i, psl, from.md_.hashes_[i]);
          count++; 
          to.values_[i].value().first = from.keys_[i];
      }
  }

  return count;
}

}}

