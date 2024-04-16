#include "zoo/swar/SWAR.h"
#include "zoo/swar/associative_iteration.h"

#include "catch2/catch.hpp"

using namespace zoo;
using namespace zoo::swar;

using S4_64 = SWAR<4, uint64_t>;
using S4_32 = SWAR<4, uint32_t>;
using S4_16 = SWAR<4, uint16_t>;
using S4_8 = SWAR<4, uint8_t>;

using S8_64 = SWAR<8, uint64_t>;
using S8_32 = SWAR<8, uint32_t>;
using S8_16 = SWAR<8, uint16_t>;
using S8_8 = SWAR<8, uint8_t>;

using S16_64 = SWAR<16, uint64_t>;
using S16_32 = SWAR<16, uint32_t>;
using S16_16 = SWAR<16, uint16_t>;

using S32_32 = SWAR<32, uint32_t>;

using S64_64 = SWAR<64, uint64_t>;

static_assert(SWAR<16, u64>::MaxUnsignedLaneValue == 65535);
static_assert(SWAR<16, u32>::MaxUnsignedLaneValue == 65535);
static_assert(SWAR<8, u32>::MaxUnsignedLaneValue == 255);
static_assert(SWAR<4, u32>::MaxUnsignedLaneValue == 15);
static_assert(SWAR<2, u32>::MaxUnsignedLaneValue == 3);

static_assert(SWAR{Literals<8, u32>, {0, 0, 0, 0}}.value() == 0);
static_assert(SWAR{Literals<8, u32>, {0, 0, 0, 1}}.value() == 1);
static_assert(SWAR{Literals<8, u32>, {8, 3, 2, 1}}.value() == 0x08'03'02'01);
static_assert(SWAR{Literals<8, u32>, {42, 42, 42, 42}}.value() == 0x2A'2A'2A'2A);
static_assert(SWAR{Literals<4, u32>, {0, 0, 0, 0, 0, 0, 0, 0}}.value() == 0);
static_assert(SWAR{Literals<4, u32>, {0, 0, 0, 0, 0, 0, 0, 1}}.value() == 1);
static_assert(SWAR{Literals<4, u32>, {0, 0, 0, 0, 0, 0, 0, 0}}.value() == 0);
static_assert(SWAR{Literals<4, u32>, {8, 7, 6, 5, 4, 3, 2, 1}}.value() == 0x8765'4321);
static_assert(SWAR{Literals<4, u32>, {8, 7, 6, 5, 4, 3, 2, 7}}.value() == 0x8765'4327);

static_assert(BooleanSWAR{Literals<4, u16>, {false, false, false, false}}.value() == 0);
static_assert(BooleanSWAR{Literals<4, u16>, {true, true, true, true}}.value() == 0b1000'1000'1000'1000);
static_assert(BooleanSWAR{Literals<8, u32>, {true, true, true, true}}.value() == 0b10000000'10000000'10000000'10000000);

static_assert(BooleanSWAR<4, u16>::fromBooleanLiterals({false, false, false, false}).value() == 0);

namespace Multiplication {

static_assert(~int64_t(0) == negate(S4_64{S4_64::LeastSignificantBit}).value());
static_assert(0x0F0F0F0F == doublingMask<4, uint32_t>().value());

constexpr auto PrecisionFixtureTest = 0x89ABCDEF;
constexpr auto Doubled =
    doublePrecision(SWAR<4, uint32_t>{PrecisionFixtureTest});

static_assert(0x090B0D0F == Doubled.even.value());
static_assert(0x080A0C0E == Doubled.odd.value());
static_assert(PrecisionFixtureTest == halvePrecision(Doubled.even, Doubled.odd).value());

constexpr SWAR<8, u32> Micand{0x5030201};
constexpr SWAR<8, u32> Mplier{0xA050301};

// expected:
// 5*0xA = 5*10 = 50 = 0x32,
// 3*5 = 15 = 0xF,
// 3*2 = 6,
// 1*1 = 1
constexpr auto Expected = 0x320F0601;

static_assert(
    Expected == multiplication_OverflowUnsafe(Micand, Mplier).value()
);
static_assert(
    0x320F0601 != // intentionally use a too-small bit count
    multiplication_OverflowUnsafe_SpecificBitCount<3>(Micand, Mplier).value()
);

}

#define HE(nbits, t, v0, v1) \
    static_assert(horizontalEquality<nbits, t>(\
        SWAR<nbits, t>(v0),\
        SWAR<nbits, t>(meta::BitmaskMaker<t, v1, nbits>::value)\
    ));
HE(8, u64, 0x0808'0808'0808'0808, 0x8);
HE(4, u64, 0x1111'1111'1111'1111, 0x1);
HE(3, u64, 0xFFFF'FFFF'FFFF'FFFF, 0x7);
HE(8, u32, 0x0808'0808, 0x8);
HE(8, u16, 0x0808, 0x8);
HE(3, u8, 0xFF, 0x7);
HE(2, u8, 0xAA, 0x2);
#undef HE

TEST_CASE("Old version", "[deprecated][swar]") {
    SWAR<8, u32> Micand{0x5030201};
    SWAR<8, u32> Mplier{0xA050301};
    auto Expected = 0x320F0601;
    auto result =
        multiplication_OverflowUnsafe_SpecificBitCount_deprecated<4>(
            Micand, Mplier
        );
    CHECK(Expected == result.value());
}

TEST_CASE("Parity", "[swar]") {
    // For each nibble, E indicates (E)ven and O (O)dd parities
    //                EEOEEOOO
    auto Examples = 0xFF13A7E4;
    SWAR<4, u32> casesBy4{Examples};
    SWAR<8, u32> casesBy8{Examples};
    auto by4 = parity(casesBy4);
    auto by8 = parity(casesBy8);
    CHECK(by4.value() == 0x00800888);
    CHECK(by8.value() == 0x00808000);
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

TEST_CASE("Compress/Expand", "[swar]") {
    unsigned
        Mask =   0b0001'0011'0111'0111'0110'1110'1100'1010,
        ToMove = 0b0101'0101'0101'0101'0101'0101'0101'0101,
        // Selection: 1   01  101  101  10  010  01   0 0
        result = 0b0001'0'1'1'0'1'1'0'1'10'0'10'0'1'0'0;
    auto q = compress(S32_32{ToMove}, S32_32{Mask});
    CHECK(result == q.value());
    SECTION("Regression 1") {
        u64
            input =   0b1010'1001'0110'0001'1001'0000'0010'1010'0100'0111'1110'1001'1111'0001'1110'1011,
            mask =    0b0110'0000'0001'0101'0101'1111'0101'1100'0110'1111'0100'0111'0001'1000'0101'0010,
            expected =0b0001'0000'0000'0001'0001'0000'0000'0010'0010'0111'0001'0001'0001'0000'0010'0001;
        using S = S4_64;
        auto v = compress(S{input}, S{mask});
        CHECK(expected == v.value());
    }
}

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
static_assert(0x0808'0808'0808'0808ull == u64(broadcast<8>(SWAR<8, u64>(0x0000'0000'0000'0008ull))));

static_assert(1 == lsbIndex(1<<1));
static_assert(3 == lsbIndex(1<<3));
static_assert(5 == lsbIndex(1<<5));
static_assert(8 == lsbIndex(1<<8));
static_assert(17 == lsbIndex(1<<17));
static_assert(30 == lsbIndex(1<<30));

/*
These tests were not catching errors known to have been present
static_assert(0x80880008 == greaterEqual<3>(SWAR<4, uint32_t>(0x3245'1027)).value());
static_assert(0x88888888 == greaterEqual<0>(SWAR<4, uint32_t>(0x0123'4567)).value());
static_assert(0x88888888 == greaterEqual<0>(SWAR<4, uint32_t>(0x7654'3210)).value());
static_assert(0x00000008 == greaterEqual<7>(SWAR<4, uint32_t>(0x0123'4567)).value());
static_assert(0x80000000 == greaterEqual<7>(SWAR<4, uint32_t>(0x7654'3210)).value());
*/

// Unusual formatting for easy visual verification.
#define GE_MSB_TEST(left, right, result) static_assert(result== greaterEqual_MSB_off<4, u32>(SWAR<4, u32>(left), SWAR<4, u32>(right)).value());

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

static_assert(0x123 == SWAR<4, uint32_t>(0x173).blitElement(1, 2).value());
static_assert(0 == isolateLSB(u32(0)));

constexpr auto aBooleansWithTrue = booleans(SWAR<4, u32>{0x1});
static_assert(aBooleansWithTrue);
//static_assert(~aBooleansWithTrue);
static_assert(false == !bool(aBooleansWithTrue));

TEST_CASE(
    "fullAddition",
    "[swar][signed-swar][unsigned-swar]"
) {
    SECTION("fullAddition overflow") {
        const auto sum = fullAddition(SWAR<4, u32>(0x0000'1000), SWAR<4, u32>(0x0000'7000));
        CHECK(SWAR<4, u32>(0x0000'0000).value() == sum.carry.value());
        CHECK(SWAR<4, u32>(0x0000'8000).value() == sum.overflow.value());
        CHECK(SWAR<4, u32>(0x0000'8000).value() == sum.result.value());
    }
    SECTION("no carry or overflow for safe values") {
        const auto sum = fullAddition(SWAR<4, u32>(0x0000'8000), SWAR<4, u32>(0x0000'7000));
        CHECK(SWAR<4, u32>(0x0000'0000).value() == sum.carry.value());
        CHECK(SWAR<4, u32>(0x0000'0000).value() == sum.overflow.value());
        CHECK(SWAR<4, u32>(0x0000'F000).value() == sum.result.value());
    }
    SECTION("fullAddition signed overflow") {
        const auto sum = fullAddition(SWAR<4, u32>(0x0000'5000), SWAR<4, u32>(0x0000'5000));
        CHECK(SWAR<4, u32>(0x0000'0000).value() == sum.carry.value());
        CHECK(SWAR<4, u32>(0x0000'8000).value() == sum.overflow.value());
        CHECK(SWAR<4, u32>(0x0000'A000).value() == sum.result.value());
    }
    SECTION("0x0111 (7) + 0x0111 (7) is 0x1110 (0x1110->0x1101->0x0010) (0xe unsigned, 0x2 signed) (signed and unsigned check)") {
        const auto sum = fullAddition(SWAR<4, u32>(0x0000'7000), SWAR<4, u32>(0x0000'7000));
        CHECK(SWAR<4, u32>(0x0000'0000).value() == sum.carry.value());
        CHECK(SWAR<4, u32>(0x0000'8000).value() == sum.overflow.value());
        CHECK(SWAR<4, u32>(0x0000'e000).value() == sum.result.value());
    }
    SECTION("both carry and overflow") {
        const auto sum = fullAddition(SWAR<4, u32>(0x0000'a000), SWAR<4, u32>(0x0000'a000));
        CHECK(SWAR<4, u32>(0x0000'8000).value() == sum.carry.value());
        CHECK(SWAR<4, u32>(0x0000'8000).value() == sum.overflow.value());
    }
}

TEST_CASE(
    "BooleanSWAR MSBtoLaneMask",
    "[swar]"
) {
    // BooleanSWAR as a mask:
    auto bswar =BooleanSWAR<4, u32>(0x0808'0000);
    auto mask = S4_32(0x0F0F'0000);
    CHECK(bswar.MSBtoLaneMask().value() == mask.value());
}

constexpr auto fullAddSumTest = fullAddition(S4_32(0x0111'1101), S4_32(0x1000'0010));
static_assert( S4_32(0x1111'1111).value() == fullAddSumTest.result.value());
static_assert( S4_32(0x0000'0000).value() == fullAddSumTest.carry.value());
static_assert( S4_32(0x0000'0000).value() == fullAddSumTest.overflow.value());

// Verify that saturation works (saturates and doesn't saturate as appropriate)
static_assert( S4_16(0x0000).value() == saturatingUnsignedAddition(S4_16(0x0000), S4_16(0x0000)).value());
static_assert( S4_16(0x0200).value() == saturatingUnsignedAddition(S4_16(0x0100), S4_16(0x0100)).value());
static_assert( S4_16(0x0400).value() == saturatingUnsignedAddition(S4_16(0x0300), S4_16(0x0100)).value());
static_assert( S4_16(0x0A00).value() == saturatingUnsignedAddition(S4_16(0x0300), S4_16(0x0700)).value());
static_assert( S4_16(0x0F00).value() == saturatingUnsignedAddition(S4_16(0x0800), S4_16(0x0700)).value());
static_assert( S4_16(0x0F00).value() == saturatingUnsignedAddition(S4_16(0x0800), S4_16(0x0800)).value());

TEST_CASE(
    "saturatingUnsignedAddition",
    "[swar][saturation]"
) {
    CHECK(SWAR<4, u16>(0x0200).value() == saturatingUnsignedAddition(SWAR<4, u16>(0x0100), SWAR<4, u16>(0x0100)).value());
    CHECK(SWAR<4, u16>(0x0400).value() == saturatingUnsignedAddition(SWAR<4, u16>(0x0100), SWAR<4, u16>(0x0300)).value());
    CHECK(SWAR<4, u16>(0x0B00).value() == saturatingUnsignedAddition(SWAR<4, u16>(0x0800), SWAR<4, u16>(0x0300)).value());
    CHECK(SWAR<4, u16>(0x0F00).value() == saturatingUnsignedAddition(SWAR<4, u16>(0x0800), SWAR<4, u16>(0x0700)).value());
    CHECK(SWAR<4, u16>(0x0F00).value() == saturatingUnsignedAddition(SWAR<4, u16>(0x0800), SWAR<4, u16>(0x0800)).value());
    CHECK(S4_32(0x0F0C'F000).value() == saturatingUnsignedAddition(S4_32(0x0804'F000), S4_32(0x0808'F000)).value());
}
