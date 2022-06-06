#include "zoo/swar/metaLog.h"
#include "zoo/swar/SWAR.h"

#include "catch2/catch.hpp"

#include <type_traits>

using namespace zoo;
using namespace zoo::swar;

#define HE(nbits, t, v0, v1) static_assert(horizontalEquality<nbits, t>(SWAR<nbits, t>(v0), SWAR<nbits, t>(makeBitmask<nbits, t>(v1))));
HE(8, u64, 0x0808'0808'0808'0808, 0x8);
HE(4, u64, 0x1111'1111'1111'1111, 0x1);
HE(3, u64, 0xFFFF'FFFF'FFFF'FFFF, 0x7);
HE(8, u32, 0x0808'0808, 0x8);
HE(8, u16, 0x0808, 0x8);
HE(3, u8, 0xFF, 0x7);
HE(2, u8, 0xAA, 0x2);
#undef HE

TEST_CASE(
    "Isolate",
    "[swar]"
) {
  for (auto i = 0; i < 63; ++i) { 
    CHECK(i == isolate<8>(i));
    CHECK(i == isolate<8>(0xFF00+i));
    CHECK(i == isolate<8>(0xFFFF00+i));
  }
  for (auto i = 0; i < 31; ++i) { 
    CHECK(i == isolate<7>(i));
    CHECK(i == isolate<7>(0xFF00+i));
    CHECK(i == isolate<7>(0xFFFF00+i));
  }
  for (auto i = 0; i < 31; ++i) { 
    CHECK(i == isolate<11>(i));
    CHECK(i == isolate<11>(0xF800+i));
    CHECK(i == isolate<11>(0xFFF800+i));
  }
}

static_assert(1 == popcount<4>(0x100ull));
static_assert(1 == popcount<4>(0x010ull));
static_assert(1 == popcount<4>(0x001ull));
static_assert(4 == popcount<4>(0xF00ull));
static_assert(8 == popcount<4>(0xFF0ull));
static_assert(9 == popcount<4>(0xEEEull));
static_assert(0x210 == popcount<0>(0x320));
static_assert(0x4321 == popcount<1>(0xF754));
static_assert(0x50004 == popcount<3>(0x3E001122));

static_assert(1 == msbIndex<u64>(1ull<<1));
static_assert(3 == msbIndex<u64>(1ull<<3));
static_assert(5 == msbIndex<u64>(1ull<<5));
static_assert(8 == msbIndex<u64>(1ull<<8));
static_assert(17 == msbIndex<u64>(1ull<<17));
static_assert(30 == msbIndex<u64>(1ull<<30));
static_assert(31 == msbIndex<u64>(1ull<<31));

static_assert(0xAA == makeBitmask<2, u8>(0x2));
static_assert(0x0808'0808ull == makeBitmask<8, u32>(0x8));
static_assert(0x0808'0808'0808'0808ull == makeBitmask<8, u64>(0x08ull));
static_assert(0x0101'0101'0101'0101ull == makeBitmask<8, u64>(0x01ull));
static_assert(0x0E0E'0E0E'0E0E'0E0Eull == makeBitmask<8, u64>(0x0Eull));
static_assert(0x0303'0303'0303'0303ull == makeBitmask<8, u64>(0x03ull));

static_assert(0x00 == clearLSB<u8>(0x80));
static_assert(0x80 == clearLSB<u8>(0xC0));
static_assert(0xC0 == clearLSB<u8>(0xE0));
static_assert(0xE0 == clearLSB<u8>(0xF0));
static_assert(0xF0 == clearLSB<u8>(0xF8));
static_assert(0xF8 == clearLSB<u8>(0xFC));
static_assert(0xFC == clearLSB<u8>(0xFE));
static_assert(0xFE == clearLSB<u8>(0xFF));
static_assert(0x00 == clearLSB<u8>(0x00));

static_assert(0x00 == isolateLSB<u8>(0x00));
static_assert(0x10 == isolateLSB<u8>(0xF0));
static_assert(0x20 == isolateLSB<u8>(0xE0));
static_assert(0x40 == isolateLSB<u8>(0xC0));
static_assert(0x80 == isolateLSB<u8>(0x80));

static_assert(0x01 == lowestNBitsMask<1, u8>());
static_assert(0x03 == lowestNBitsMask<2, u8>());
static_assert(0x07 == lowestNBitsMask<3, u8>());
static_assert(0x0F == lowestNBitsMask<4, u8>());

static_assert(0xB == isolate<4>(0x1337'BDBC'2448'ACABull));
static_assert(0xAB == isolate<8>(0x1337'BDBC'2448'ACABull));
static_assert(0xCAB == isolate<12>(0x1337'BDBC'2448'ACABull));
static_assert(0xACAB == isolate<16>(0x1337'BDBC'2448'ACABull));
static_assert(0x3 == isolate<3>(0x1337'BDBC'2448'ACABull));

static_assert(0x00 == clearLSBits<2, u8>(0x80));
static_assert(0x00 == clearLSBits<2, u8>(0xC0));
static_assert(0x80 == clearLSBits<2, u8>(0xE0));
static_assert(0xC0 == clearLSBits<2, u8>(0xF0));
static_assert(0xE0 == clearLSBits<2, u8>(0xF8));
static_assert(0xF8 == clearLSBits<2, u8>(0xFB));
static_assert(0xF0 == clearLSBits<2, u8>(0xFC));
static_assert(0xFC == clearLSBits<2, u8>(0xFF));

static_assert(0x80 == clearLSBits<4, u8>(0xF8));
static_assert(0xC0 == clearLSBits<4, u8>(0xF4));
static_assert(0xE0 == clearLSBits<4, u8>(0xF2));
static_assert(0xF0 == clearLSBits<4, u8>(0xF1));

static_assert(0xF0 == clearLSBits<4, u8>(0xFF));
static_assert(0xC0 == clearLSBits<4, u8>(0xFC));
static_assert(0xE0 == clearLSBits<4, u8>(0xFA));

static_assert(0x80 == isolateLSBits<2, u8>(0x80));
static_assert(0xC0 == isolateLSBits<2, u8>(0xC0));
static_assert(0x60 == isolateLSBits<2, u8>(0xE0));
static_assert(0x30 == isolateLSBits<2, u8>(0xF0));
static_assert(0x18 == isolateLSBits<2, u8>(0xF8));
static_assert(0x03 == isolateLSBits<2, u8>(0xFB));
static_assert(0x0C == isolateLSBits<2, u8>(0xFC));
static_assert(0x03 == isolateLSBits<2, u8>(0xFF));

static_assert(0x0606'0606 == u32(broadcast<8>(SWAR<8, u32>(0x0000'0006))));
static_assert(0x0808'0808 == u32(broadcast<8>(SWAR<8, u32>(0x0000'0008))));
static_assert(0x0B0B'0B0B == u32(broadcast<8>(SWAR<8, u32>(0x0000'000B))));
static_assert(0x0E0E'0E0E == u32(broadcast<8>(SWAR<8, u32>(0x0000'000E))));
static_assert(0x6B6B'6B6B == u32(broadcast<8>(SWAR<8, u32>(0x0000'006B))));
static_assert(0x0808'0808'0808'0808ull == u64(broadcast<8>(SWAR<8, u64>(0x0000'0000'0000'0008ull))));

static_assert(2 == lsbIndex(1<<1));
static_assert(4 == lsbIndex(1<<3));
static_assert(6 == lsbIndex(1<<5));
static_assert(9 == lsbIndex(1<<8));
static_assert(18 == lsbIndex(1<<17));
static_assert(31 == lsbIndex(1<<30));

static_assert(0x80880008 == greaterEqual<3>(SWAR<4, uint32_t>(0x3245'1027)).value());
static_assert(0x88888888 == greaterEqual<0>(SWAR<4, uint32_t>(0x0123'4567)).value());
static_assert(0x88888888 == greaterEqual<0>(SWAR<4, uint32_t>(0x7654'3210)).value());
static_assert(0x00000008 == greaterEqual<7>(SWAR<4, uint32_t>(0x0123'4567)).value());
static_assert(0x80000000 == greaterEqual<7>(SWAR<4, uint32_t>(0x7654'3210)).value());


// Unusual formatting for easy visual verification.
#define GE_MSB_TEST(left, right, result) static_assert(result== greaterEqual_MSB_off<4, u32>(SWAR<4, u32>(left), SWAR<4, u32>(right)).value());

GE_MSB_TEST(0x1000'0010,
            0x0111'1101,
            0x8000'0080)
GE_MSB_TEST(0x4333'3343,
            0x4444'4444,
            0x8000'0080)

GE_MSB_TEST(0x0123'4567,
            0x0000'0000,
            0x8888'8888); 
GE_MSB_TEST(0x0123'4567,
            0x0000'0000,
            0x8888'8888)
GE_MSB_TEST(0x0123'4567,
            0x1111'1111,
            0x0888'8888)
GE_MSB_TEST(0x0123'4567,
            0x2222'2222,
            0x0088'8888)
GE_MSB_TEST(0x0123'4567,
            0x3333'3333,
            0x0008'8888)
GE_MSB_TEST(0x0123'4567,
            0x4444'4444,
            0x0000'8888)
GE_MSB_TEST(0x0123'4567,
            0x5555'5555,
            0x0000'0888)
GE_MSB_TEST(0x0123'4567,
            0x6666'6666,
            0x0000'0088)
GE_MSB_TEST(0x0123'4567,
            0x7777'7777,
            0x0000'0008)

GE_MSB_TEST(0x0000'0000,
            0x0123'4567,
            0x8000'0000)
GE_MSB_TEST(0x1111'1111,
            0x0123'4567,
            0x8800'0000)
GE_MSB_TEST(0x2222'2222,
            0x0123'4567,
            0x8880'0000)
GE_MSB_TEST(0x3333'3333,
            0x0123'4567,
            0x8888'0000)
GE_MSB_TEST(0x4444'4444,
            0x0123'4567,
            0x8888'8000)
GE_MSB_TEST(0x5555'5555,
            0x0123'4567,
            0x8888'8800)
GE_MSB_TEST(0x6666'6666,
            0x0123'4567,
            0x8888'8880)
GE_MSB_TEST(0x7777'7777,
            0x0123'4567,
            0x8888'8888)
