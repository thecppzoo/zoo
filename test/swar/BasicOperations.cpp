#include "zoo/swar/SWAR.h"

#include "catch2/catch.hpp"

#include <type_traits>

using namespace zoo;
using namespace zoo::swar;

TEST_CASE(
    "Bitmasking",
    "[swar]"
) {
#define HE(nbits, t, v0, v1) \
    CHECK(horizontalEquality<nbits, t>(\
        SWAR<nbits, t>(v0),\
        SWAR<nbits, t>(meta::BitmaskMaker<t, v1, nbits>::value)\
    ));\
    CHECK(\
        SWAR<nbits, t>(v0).value() ==\
        SWAR<nbits, t>(meta::BitmaskMaker<t, v1, nbits>::value).value());

HE(8, u64, 0x0808'0808'0808'0808, 0x8);
HE(4, u64, 0x1111'1111'1111'1111, 0x1);
HE(3, u64, 0xFFFF'FFFF'FFFF'FFFF, 0x7);
HE(12, u64, 0xFFFF'FFFF'FFFF'FFFF, 0xFFF);
HE(33, u64, 0x0000'0001'FFFF'FFFF, 0x1'FFFF'FFFF);
HE(8, u32, 0x0808'0808, 0x8);
HE(8, u16, 0x0808, 0x8);
HE(7, u8, 0x7F, 0x7F);
HE(3, u8, 0xFF, 0x7);
HE(2, u8, 0xAA, 0x2);
#undef HE
}

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

// Bitmasks.
static_assert(0xFF == zoo::meta::BitmaskMaker<uint8_t, 0x7, 3>::value);
static_assert(0x92 == zoo::meta::BitmaskMaker<uint8_t, 0x2, 3>::value);
static_assert(0x3F == 
    (zoo::meta::BitmaskMaker<uint8_t, 0x7, 3>::value &
     zoo::meta::BitmaskMakerClearTop<uint8_t, 3>::TopBlit));
static_assert(0x12 == 
    (zoo::meta::BitmaskMaker<uint8_t, 0x2, 3>::value &
     zoo::meta::BitmaskMakerClearTop<uint8_t, 3>::TopBlit));
static_assert(0x00FE'DFED == (
    zoo::meta::BitmaskMaker<uint32_t, 0xFED, 12>::value  &
    zoo::meta::BitmaskMakerClearTop<uint32_t, 12>::TopBlit));
static_assert(0x0FFF'FFFF'FFFF'FFFF == (
    zoo::meta::BitmaskMaker<u64, 0xFFF, 12>::value & 
    zoo::meta::BitmaskMakerClearTop<u64, 12>::TopBlit));


static_assert(1 == popcount<5>(0x100ull));
static_assert(1 == popcount<5>(0x010ull));
static_assert(1 == popcount<5>(0x001ull));
static_assert(4 == popcount<5>(0xF00ull));
static_assert(8 == popcount<5>(0xFF0ull));
static_assert(9 == popcount<5>(0xEEEull));
static_assert(0x210 == popcount<1>(0x320));
static_assert(0x4321 == popcount<2>(0xF754));
static_assert(0x50004 == popcount<4>(0x3E001122));

static_assert(1 == msbIndex<u64>(1ull<<1));
static_assert(3 == msbIndex<u64>(1ull<<3));
static_assert(5 == msbIndex<u64>(1ull<<5));
static_assert(8 == msbIndex<u64>(1ull<<8));
static_assert(17 == msbIndex<u64>(1ull<<17));
static_assert(30 == msbIndex<u64>(1ull<<30));
static_assert(31 == msbIndex<u64>(1ull<<31));

namespace {
using namespace zoo::meta;

static_assert(0xAA == BitmaskMaker<u8, 2, 2>::value);
static_assert(0x0808'0808ull == BitmaskMaker<u32, 8, 8>::value);
static_assert(0x0808'0808'0808'0808ull == BitmaskMaker<u64, 0x08ull, 8>::value);
static_assert(0x0101'0101'0101'0101ull == BitmaskMaker<u64, 0x01ull, 8>::value);
static_assert(0x0E0E'0E0E'0E0E'0E0Eull == BitmaskMaker<u64, 0x0Eull, 8>::value);
static_assert(0x0303'0303'0303'0303ull == BitmaskMaker<u64, 0x03ull, 8>::value);
}

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

static_assert(0x80u == mostNBitsMask<1, u8>());
static_assert(0xC0u == mostNBitsMask<2, u8>());
static_assert(0xE0u == mostNBitsMask<3, u8>());
static_assert(0xF0u == mostNBitsMask<4, u8>());
static_assert(0xF8u == mostNBitsMask<5, u8>());
static_assert(0xFCu == mostNBitsMask<6, u8>());

static_assert(0x8000'0000ul == mostNBitsMask<1, u32>());
static_assert(0xC000'0000ul == mostNBitsMask<2, u32>());
static_assert(0xE000'0000ul == mostNBitsMask<3, u32>());
static_assert(0xF000'0000ul == mostNBitsMask<4, u32>());
static_assert(0xF800'0000ul == mostNBitsMask<5, u32>());
static_assert(0xFC00'0000ul == mostNBitsMask<6, u32>());

static_assert(0x01u == leastNBitsMask<1, u8>());
static_assert(0x03u == leastNBitsMask<2, u8>());
static_assert(0x07u == leastNBitsMask<3, u8>());
static_assert(0x0Fu == leastNBitsMask<4, u8>());
static_assert(0x1Fu == leastNBitsMask<5, u8>());

static_assert(0x0000'01ul == leastNBitsMask<1, u32>());
static_assert(0x0000'03ul == leastNBitsMask<2, u32>());
static_assert(0x0000'07ul == leastNBitsMask<3, u32>());
static_assert(0x0000'0Ful == leastNBitsMask<4, u32>());
static_assert(0x0000'1Ful == leastNBitsMask<5, u32>());

static_assert(0x01ull == leastNBitsMask<1, u64>());
static_assert(0x03ull == leastNBitsMask<2, u64>());
static_assert(0x07ull == leastNBitsMask<3, u64>());
static_assert(0x0Full == leastNBitsMask<4, u64>());
static_assert(0x1Full == leastNBitsMask<5, u64>());

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
static_assert(0x0808'0808'0808'0808ull ==
    u64(broadcast<8>(SWAR<8, u64>(0x0000'0000'0000'0008ull))));

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
#define GE_MSB_TEST(left, right, result) static_assert(result == \
    greaterEqual_MSB_off<4, u32>(SWAR<4, u32>(left), SWAR<4, u32>(right)).value());

GE_MSB_TEST(0x1000'0010,
            0x0111'1101,
            0x8000'0080)
GE_MSB_TEST(0x4333'3343,
            0x4444'4444,
            0x8000'0080)
GE_MSB_TEST(0x0550'0110,
            0x0110'0550,
            0x8888'8008)
GE_MSB_TEST(0x4771'1414,
            0x4641'1774,
            0x8888'8008)

GE_MSB_TEST(0x0123'4567,
            0x0000'0000,
            0x8888'8888)
GE_MSB_TEST(0x0123'4567,
            0x7777'7777,
            0x0000'0008)

GE_MSB_TEST(0x0000'0000,
            0x0123'4567,
            0x8000'0000)
GE_MSB_TEST(0x7777'7777,
            0x0123'4567,
            0x8888'8888)

// Unusual formatting for easy visual verification.
using Sub22u32 = SWARWithSubLanes<2,2,u32>;
using SWAR4u32 = SWAR<4,u32>;
using SWAR12u32 = SWAR<12,u32>;
#define GE_LEAST_TEST(left, right, result) static_assert(result == greaterEqualLeast<2,2, u32>(Sub22u32(left), Sub22u32(right)).value());

GE_LEAST_TEST(0x1000'0010,
              0x0111'1101,
              0x8000'0080)
GE_LEAST_TEST(0x0000'0000,
              0x0000'0000,
              0x8888'8888)
GE_LEAST_TEST(0x1111'1111,
              0x1111'1111,
              0x8888'8888)
// We only compare least significant lanes
GE_LEAST_TEST(0x8484'2151,
              0x8448'1215,
              0x8888'8088)
GE_LEAST_TEST(0x0011'2233,
              0x4859'56EF,
              0x8888'8888)

#define GE_MOST_TEST(left, right, result) static_assert(result == \
    greaterEqualMost<2,2, u32>(Sub22u32(left), Sub22u32(right)).value());

GE_MOST_TEST(0x0000'0000,
             0x0000'0000,
             0x8888'8888)
GE_MOST_TEST(0x1111'1111,
             0x1111'1111,
             0x8888'8888)
GE_MOST_TEST(0x2211'3330,
             0x2121'1233,
             0x8888'8888)
GE_MOST_TEST(0x8945'1111,
             0x1111'8945,
             0x8888'0000)
GE_MOST_TEST(0x4000'0040,
             0x4111'1141,
             0x8888'8888)

#define GE_LEAST_VS_MSBOFF_TEST(left, right) static_assert( \
    greaterEqual_MSB_off<4, u32>(SWAR4u32(Sub22u32(left).least()), \
        SWAR4u32(Sub22u32(right).least())).value() ==  \
    greaterEqualLeast<2,2, u32>(Sub22u32(left), Sub22u32(right)).value());

GE_LEAST_VS_MSBOFF_TEST(0x1000'0010,
                        0x0111'1101)
GE_LEAST_VS_MSBOFF_TEST(0x0000'0000,
                        0x0000'0000)
GE_LEAST_VS_MSBOFF_TEST(0x8484'2151,
                        0x8448'1215)
GE_LEAST_VS_MSBOFF_TEST(0x0011'2233,
                        0x4859'56EF)

#define GE_LEAST_BLIT(left, right, result) CHECK(result == \
    greaterEqual_laneblit<4, u32>(SWAR4u32(left), SWAR4u32(right)).value());

TEST_CASE(
    "LaneBlit4",
    "[swar]"
) {

GE_LEAST_BLIT(0x1234'5678,
              0x1111'1111,
              0x8888'8888)

GE_LEAST_BLIT(0x888C'0000,
              0x44C8'0000,
              0x8808'8888)

GE_LEAST_BLIT(0x8800'0021,
              0x4432'2100,
              0x8800'0088)

GE_LEAST_BLIT(0x1248'1111,
              0x1111'1248,
              0x8888'8000)

GE_LEAST_BLIT(0xEF44'44FE,
              0xFE44'44EF,
              0x0888'8880)
}

#define GE_LEAST_BLIT12(left, right, result) CHECK(result == \
    greaterEqual_laneblit<12, u32>(SWAR12u32(left), SWAR12u32(right)).value());

TEST_CASE(
    "LaneBlit12",
    "[swar]"
) {

GE_LEAST_BLIT12(0x1234'5678,
                0x1111'1111,
                0x0080'0800)

GE_LEAST_BLIT12(0x888C'0000,
                0x44C8'0000,
                0x0000'0800)

GE_LEAST_BLIT12(0x8800'0121,
                0x4432'2084,
                0x0000'0800)

GE_LEAST_BLIT12(0x1248'1111,
                0x1111'1248,
                0x0080'0000)

GE_LEAST_BLIT12(0xEF44'44FE,
                0xFE44'44EF,
                0x0080'0800)
}

// 3 bits on msb side, 5 bits on lsb side.
using Lanes = SWARWithSubLanes<3,5,u32>;
using S8u32 = SWAR<8, u32>;
static constexpr inline u32 allF = broadcast<8>(S8u32(0x0000'00FFul)).value();

static_assert(allF == Lanes(allF).value());
static_assert(0xFFFF'FFFF == Lanes(allF).value());

static_assert(0xFFFF'FFE0 == Lanes(allF).least(0,0).value());
static_assert(0xFFFF'FFE1 == Lanes(allF).least(1,0).value());
static_assert(0xFFFF'E0FF == Lanes(allF).least(0,1).value());
static_assert(0xFFFF'E1FF == Lanes(allF).least(1,1).value());

static_assert(0xFFE0'FFFF == Lanes(allF).least(0,2).value());
static_assert(0xFFE1'FFFF == Lanes(allF).least(1,2).value());
static_assert(0xE0FF'FFFF == Lanes(allF).least(0,3).value());
static_assert(0xE1FF'FFFF == Lanes(allF).least(1,3).value());

static_assert(0xFFFF'FF1F == Lanes(allF).most(0,0).value());
static_assert(0xFFFF'FF3F == Lanes(allF).most(1,0).value());
static_assert(0xFFFF'1FFF == Lanes(allF).most(0,1).value());
static_assert(0xFFFF'3FFF == Lanes(allF).most(1,1).value());

static_assert(0xFF1F'FFFF == Lanes(allF).most(0,2).value());
static_assert(0xFF3F'FFFF == Lanes(allF).most(1,2).value());
static_assert(0x1FFF'FFFF == Lanes(allF).most(0,3).value());
static_assert(0x3FFF'FFFF == Lanes(allF).most(1,3).value());

static_assert(0x1F1F'1F1F == Lanes(allF).least());
static_assert(0xE0E0'E0E0 == Lanes(allF).most());

static_assert(0x0000'001F == Lanes(allF).least(0));
static_assert(0x0000'1F00 == Lanes(allF).least(1));
static_assert(0x001F'0000 == Lanes(allF).least(2));
static_assert(0x1F00'0000 == Lanes(allF).least(3));

static_assert(0x0000'00E0 == Lanes(allF).most(0));
static_assert(0x0000'E000 == Lanes(allF).most(1));
static_assert(0x00E0'0000 == Lanes(allF).most(2));
static_assert(0xE000'0000 == Lanes(allF).most(3));

