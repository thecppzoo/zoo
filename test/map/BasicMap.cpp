#include "zoo/map/robinhood.h"
#include "zoo/swar/SWAR.h"
#include "catch2/catch.hpp"

#include <iostream>
#include <type_traits>

using namespace zoo;
using namespace zoo::swar;
using namespace zoo::rh;

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

bool keyCheck(u64 lookup, u64 target) {
  return lookup == target;
}

template<u8 PSLBits, u8 HashBits, typename Key> 
void dump(const StrawmanMap<PSLBits, HashBits, Key>& t) {
  std::cerr << "sz k " << t.keys_.size() << " p " << t.md_.psls_.size() << " h " << t.md_.hashes_.size() << "\n";
  std::cerr << "keys ";
  for (auto e : t.keys_) std::cerr << e << " ";
  std::cerr << "\n psl ";
  for (u16 e : t.md_.psls_) std::cerr << e << " ";
  std::cerr << "\nhash ";
  for (u16 e : t.md_.hashes_) std::cerr << e << " ";
  std::cerr << "\n";
}

template<u8 PSLBits, u8 HashBits>
void dumpMeta(const std::string& label, const StrawMetadata<PSLBits, HashBits>& md) {
  std::cerr << "label " << label << " sz p " << md.psls_.size() << " h " << md.hashes_.size() << "\n";
  std::cerr << "\n psl ";
  for (u16 e : md.psls_) std::cerr << e << " ";
  std::cerr << "\nhash ";
  for (u16 e : md.hashes_) std::cerr << e << " ";
  std::cerr << "\n";
}

TEST_CASE(
    "CheapOkHash",
    "[robinhood][hash]"
) {

  u64 a = 0xf0f0'f0f0'f0f0'f0f0;
  CHECK(cheapOkHash<8>(a) == 134);  // Not actually checked to be correct.
  u64 b = 0xffff'ffff'ffff'ffff;
  CHECK(cheapOkHash<8>(b) == 0xfe);  // Not actually checked to be correct.
  u64 c = 0x0000'0000'0000'0000;
  CHECK(cheapOkHash<8>(c) == 0x00);  // Not actually checked to be correct.
}

TEST_CASE(
    "StrawhoodBasic",
    "[robinhood]"
) {
 u32 sz = 9;
 StrawmanMap<7, 9, u64> rhmap(sz);

 CHECK(rhmap.keys_.size() == sz);
 CHECK(rhmap.md_.psls_.size() == sz);
 CHECK(rhmap.md_.hashes_.size() == sz);
 CHECK(rhmap.insert(5, &keyCheck).first == 1);
 CHECK(rhmap.exists(5, &keyCheck) == true);
 CHECK(rhmap.insert(5, &keyCheck).first == 1);
 CHECK(rhmap.exists(5, &keyCheck) == true);
 CHECK(rhmap.insert(7, &keyCheck).first == 2);
 CHECK(rhmap.exists(7, &keyCheck) == true);
 CHECK(rhmap.insert(9, &keyCheck).first == 3);
 CHECK(rhmap.exists(9, &keyCheck) == true);
 CHECK(rhmap.insert(11, &keyCheck).first == 4);
 CHECK(rhmap.exists(5, &keyCheck) == true);
 CHECK(rhmap.exists(7, &keyCheck) == true);
 CHECK(rhmap.exists(9, &keyCheck) == true);
 CHECK(rhmap.exists(11, &keyCheck) == true);
 CHECK(rhmap.insert(11, &keyCheck).first == 4);
 CHECK(rhmap.exists(11, &keyCheck) == true);
}

TEST_CASE(
    "TranslateWireRepr",
    "[robinhood]"
) {
  u32 sz = 9;
  StrawmanMap<7, 9, u64> rhmap(sz);
  
  CHECK(rhmap.insert(5, &keyCheck).first == 1);
  CHECK(rhmap.insert(5, &keyCheck).first == 1);
  CHECK(rhmap.insert(7, &keyCheck).first == 2);
  CHECK(rhmap.insert(9, &keyCheck).first == 3);
  CHECK(rhmap.insert(11, &keyCheck).first == 4);
  CHECK(rhmap.insert(11, &keyCheck).first == 4);

//template<u8 PSLBits, u8 HashBits>
//RobinHoodMetadata<PSLBits, HashBits> strawToReal(const StrawMetadata<PSLBits, HashBits>& straw) {
//template<u8 PSLBits, u8 HashBits>
//StrawMetadata<PSLBits, HashBits> realToStraw(const RobinHoodMetadata<PSLBits, HashBits> & actual) {
  const auto real = strawToReal<7,9,u64>(rhmap.md_);
  const auto strawToo = realToStraw<7,9,u64>(real);
  dumpMeta("before", rhmap.md_);
  dumpRealMeta(real);
  dumpMeta("after", strawToo);
  CHECK(rhmap.md_.size() == strawToo.size()+5);
}
