#include "zoo/swar/associative_iteration.h"

#include "catch2/catch.hpp"

#include <string.h>
#include <type_traits>

using namespace zoo;
using namespace zoo::swar;

using S2_64 = SWAR<2, uint64_t>;
using S2_32 = SWAR<2, uint32_t>;
using S2_16 = SWAR<2, uint16_t>;

using S3_16 = SWAR<3, uint16_t>;
using S3_32 = SWAR<4, uint32_t>;

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

auto v = SWAR<3, u16>::LeastSignificantLaneMask;


#define ZOO_PP_UNPARENTHESIZE(...) __VA_ARGS__
#define X(TYPE, av, expected) \
static_assert(\
    SWAR{\
        Literals<ZOO_PP_UNPARENTHESIZE TYPE>,\
        {ZOO_PP_UNPARENTHESIZE av}\
    }.value() ==\
    expected\
);

static_assert(SWAR{Literals<16, u32>, {1, 2}}.value() == 0x0001'0002);

/* Preserved to illustrate a technique, remove in a few revisions
static_assert(SWAR{Literals<32, u64>, {2, 1}}.value() == 0x00000002'00000001);
static_assert(SWAR{Literals<32, u64>, {1, 2}}.value() == 0x00000001'00000002);

static_assert(SWAR{Literals<16, u64>, {4, 3, 2, 1}}.value() == 0x0004'0003'0002'0001);
static_assert(SWAR{Literals<16, u64>, {1, 2, 3, 4}}.value() == 0x0001'0002'0003'0004);

static_assert(SWAR{Literals<16, u32>, {2, 1}}.value() == 0x0002'0001);
static_assert(SWAR{Literals<16, u32>, {1, 2}}.value() == 0x0001'0002);

static_assert(SWAR{Literals<8, u32>, {4, 3, 2, 1}}.value() == 0x04'03'02'01);
static_assert(SWAR{Literals<8, u32>, {1, 2, 3, 4}}.value() == 0x01'02'03'04);
static_assert(SWAR{Literals<8, u32>, {1, 2, 3, 4}}.value() == 0x01'02'03'04);

static_assert(SWAR{Literals<8, u16>, {2, 1}}.value() == 0x0201);
static_assert(SWAR{Literals<8, u16>, {1, 2}}.value() == 0x0102);


static_assert(SWAR{Literals<4, u8>, {2, 1}}.value() == 0x21);
static_assert(SWAR{Literals<4, u8>, {1, 2}}.value() == 0x12);

// Little-endian
static_assert(SWAR{Literals<16, u64>, {1, 2, 3, 4}}.at(0) == 4);
static_assert(SWAR{Literals<16, u64>, {1, 2, 3, 4}}.at(1) == 3);

// Non-power of two
static_assert(SWAR<5, u16>::Lanes == 3);
static_assert(SWAR{Literals<5, u16>, {1, 1, 1}}.value() == 0b0'00001'00001'00001);
static_assert(SWAR{Literals<5, u16>, {31, 31, 31}}.value() == 0b0'11111'11111'11111);
static_assert(SWAR{Literals<5, u16>, {1, 7, 17}}.value() == 0b0'00001'00111'10001);

// Macro required because initializer lists are not constexpr
#define ARRAY_TEST(SwarType, ...)                                              \
    static_assert([]() {                                                       \
        using S = SwarType;                                                    \
        constexpr auto arry = std::array{__VA_ARGS__};                         \
        constexpr auto test_array = S{S::Literal, {__VA_ARGS__}}.to_array();   \
        static_assert(arry.size() == S::Lanes);                                \
        for (auto i = 0; i < S::Lanes; ++i) {                                  \
            if (arry[i] != test_array.at(i)) {                                 \
                return false;                                                  \
            }                                                                  \
        }                                                                      \
        return true;                                                           \
    }());                                                                      \

ARRAY_TEST(S16_64, 1, 2, 3, 4);
ARRAY_TEST(S16_64, 4, 3, 2, 1);

ARRAY_TEST(S8_32, 255, 255, 255, 255);
ARRAY_TEST(S8_64, 255, 255, 255, 255, 255, 255, 255, 255);

ARRAY_TEST(S16_32, 65534, 65534);
ARRAY_TEST(S16_64, 65534, 65534, 65534, 65534);

#define F false
#define T true
using BS = BooleanSWAR<4, u16>;
static_assert(BS{Literals<4, u16>, {F, F, F, F}}.value() == 0b0000'0000'0000'0000);
static_assert(BS{Literals<4, u16>, {T, F, F, F}}.value() == 0b1000'0000'0000'0000);
static_assert(BS{Literals<4, u16>, {F, T, F, F}}.value() == 0b0000'1000'0000'0000);
static_assert(BS{Literals<4, u16>, {F, F, T, F}}.value() == 0b0000'0000'1000'0000);
static_assert(BS{Literals<4, u16>, {F, F, F, T}}.value() == 0b0000'0000'0000'1000);
static_assert(BS{Literals<4, u16>, {T, F, F, F}}.value() == 0b1000'0000'0000'0000);

static_assert(SWAR{Literals<8, u16>, {2, 1}}.value() == 0x0201);
static_assert(SWAR{Literals<8, u16>, {1, 2}}.value() == 0x0102);
*/
#define LITERALS_TESTS \
X(\
    (32, u64),\
    (2, 1),\
    0x00000002'00000001\
);\
X(\
    (8, u32),\
    (255, 255, 255, 255),\
    0xFF'FF'FF'FF\
);\
X(\
    (32, u64),\
    (1, 2),\
    0x00000001'00000002\
);\
X(\
    (16, u64),\
    (4, 3, 2, 1),\
    0x0004'0003'0002'0001\
);\
X(\
    (16, u64),\
    (1, 2, 3, 4),\
    0x0001'0002'0003'0004\
)\
X(\
    (16, u32),\
    (2, 1),\
    0x0002'0001\
)\
X(\
    (16, u32),\
    (1, 2),\
    0x0001'0002\
)\
X(\
    (8, u32),\
    (4, 3, 2, 1),\
    0x04'03'02'01\
)\
X(\
    (8, u32),\
    (1, 2, 3, 4),\
    0x01'02'03'04\
)\
X(\
    (8, u16),\
    (2, 1),\
    0x0201\
)\
X(\
    (8, u16),\
    (1, 2),\
    0x0102\
)\
X(\
    (4, u8),\
    (2, 1),\
    0x21\
)\
X(\
    (4, u8),\
    (1, 2),\
    0x12\
)


LITERALS_TESTS


#define F false
#define T true
static_assert(BooleanSWAR{Literals<4, u16>,
    {F, F, F, F}}.value() ==
    0b0000'0000'0000'0000);
static_assert(BooleanSWAR{Literals<4, u16>,
    {T, F, F, F}}.value() ==
    0b1000'0000'0000'0000);
static_assert(BooleanSWAR{Literals<4, u16>,
    {F, T, F, F}}.value() ==
    0b0000'1000'0000'0000);
static_assert(BooleanSWAR{Literals<4, u16>,
    {F, F, T, F}}.value() ==
    0b0000'0000'1000'0000);
static_assert(BooleanSWAR{Literals<4, u16>,
    {F, F, F, T}}.value() ==
    0b0000'0000'0000'1000);
static_assert(BooleanSWAR{Literals<4, u16>,
    {T, F, F, F}}.value() ==
    0b1000'0000'0000'0000);
#undef F
#undef T

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

TEST_CASE("Old multiply version", "[deprecated][swar]") {
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
    SWAR<2, u32> casesBy2{Examples};
    SWAR<4, u32> casesBy4{Examples};
    SWAR<8, u32> casesBy8{Examples};
    SWAR<16, u32> casesBy16{Examples};
    auto by2 = parity(casesBy2);
    auto by4 = parity(casesBy4);
    auto by8 = parity(casesBy8);
    auto by16 = parity(casesBy16);
    CHECK(by2.value() == 0x0020'a828);
    CHECK(by4.value() == 0x0080'0888);
    CHECK(by8.value() == 0x0080'8000);
    CHECK(by16.value() == 0x8000'8000);
}

TEST_CASE(
    "Isolate",
    "[swar]"
) {
    #if ZOO_USE_LEASTNBITSMASK
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
    {
        u8 allones = ~u8{0};  // Avoid integer promotion.
        CHECK(0x00u == isolate<0, u8>(allones));
        CHECK(0xFFu == isolate<8, u8>(allones));
    }
    {
        u16 allones = ~u16{0};  // Avoid integer promotion.
        CHECK(0x0000u == isolate<0, u16>(allones));
        CHECK(0xFFFFu == isolate<16, u16>(allones));
    }
    {
        u32 allones = ~u32{0};
        CHECK(0x0000u == isolate<0, u32>(allones));
        //CHECK(0xFFFF'FFFFu == isolate<32, u32>(allones));  // Broken until PR/93 goes in.
    }
    {
        u64 allones = ~u64{0};
        CHECK(0x0000u == isolate<0, u64>(allones));
        //CHECK(0xFFFF'FFFF'FFFF'FFFFull == isolate<64, u64>(allones));  // Broken until PR/93 goes in.
    }
    #else
    REQUIRE(true);
    #endif
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

#if ZOO_USE_LEASTNBITSMASK
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
static_assert(0x3Fu == leastNBitsMask<6, u8>());
static_assert(0x7Fu == leastNBitsMask<7, u8>());
static_assert(0xFFu == leastNBitsMask<8, u8>());

static_assert(0x0001u == leastNBitsMask<1, u16>());
static_assert(0x0003u == leastNBitsMask<2, u16>());
static_assert(0x0007u == leastNBitsMask<3, u16>());
static_assert(0x000Fu == leastNBitsMask<4, u16>());
static_assert(0x001Fu == leastNBitsMask<5, u16>());
static_assert(0x003Fu == leastNBitsMask<6, u16>());
static_assert(0x007Fu == leastNBitsMask<7, u16>());
static_assert(0x00FFu == leastNBitsMask<8, u16>());
static_assert(0x01FFu == leastNBitsMask<9, u16>());
static_assert(0x03FFu == leastNBitsMask<10, u16>());
static_assert(0x07FFu == leastNBitsMask<11, u16>());
static_assert(0x0FFFu == leastNBitsMask<12, u16>());
static_assert(0x1FFFu == leastNBitsMask<13, u16>());
static_assert(0x3FFFu == leastNBitsMask<14, u16>());
static_assert(0x7FFFu == leastNBitsMask<15, u16>());
static_assert(0xFFFFu == leastNBitsMask<16, u16>());

static_assert(0x0000'0001ul == leastNBitsMask<1, u32>());
static_assert(0x0000'0003ul == leastNBitsMask<2, u32>());
static_assert(0x0000'0007ul == leastNBitsMask<3, u32>());
static_assert(0x0000'000Ful == leastNBitsMask<4, u32>());
static_assert(0x0000'001Ful == leastNBitsMask<5, u32>());
static_assert(0x7FFF'FFFFul == leastNBitsMask<31, u32>());
static_assert(0xFFFF'FFFFul == leastNBitsMask<32, u32>());

static_assert(0x01ull == leastNBitsMask<1, u64>());
static_assert(0x03ull == leastNBitsMask<2, u64>());
static_assert(0x07ull == leastNBitsMask<3, u64>());
static_assert(0x0Full == leastNBitsMask<4, u64>());
static_assert(0x1Full == leastNBitsMask<5, u64>());
static_assert(0x7FFF'FFFF'FFFF'FFFFull == leastNBitsMask<63, u64>());
static_assert(0xFFFF'FFFF'FFFF'FFFFull == leastNBitsMask<64, u64>());

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
#endif

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

static_assert(S3_16{Literals<3,u16>, {5,4,3,2,5}}.value() == S3_16{Literals<3,u16>, {5,4,3,2,1}}.blitElement(0, 5).value());
static_assert(S3_16{Literals<3,u16>, {5,4,3,5,1}}.value() == S3_16{Literals<3,u16>, {5,4,3,2,1}}.blitElement(1, 5).value());
static_assert(S3_16{Literals<3,u16>, {5,4,5,2,1}}.value() == S3_16{Literals<3,u16>, {5,4,3,2,1}}.blitElement(2, 5).value());
static_assert(S3_16{Literals<3,u16>, {5,1,3,2,1}}.value() == S3_16{Literals<3,u16>, {5,4,3,2,1}}.blitElement(3, 1).value());
static_assert(S3_16{Literals<3,u16>, {1,4,3,2,1}}.value() == S3_16{Literals<3,u16>, {5,4,3,2,1}}.blitElement(4, 1).value());

static_assert(1 == S3_16{Literals<3,u16>, {5,4,3,2,1}}.at(0));
static_assert(2 == S3_16{Literals<3,u16>, {5,4,3,2,1}}.at(1));
static_assert(3 == S3_16{Literals<3,u16>, {5,4,3,2,1}}.at(2));
static_assert(4 == S3_16{Literals<3,u16>, {5,4,3,2,1}}.at(3));
static_assert(5 == S3_16{Literals<3,u16>, {5,4,3,2,1}}.at(4));

#define GE_MSB_TEST(left, right, result) static_assert(result == greaterEqual_MSB_off<4, u32>(SWAR<4, u32>(left), SWAR<4, u32>(right)).value());

GE_MSB_TEST(
    0x1000'0010,
    0x0111'1101,
    0x8000'0080)
GE_MSB_TEST(
    0x4333'3343,
    0x4444'4444,
    0x8000'0080)
GE_MSB_TEST(
    0x0550'0110,
    0x0110'0550,
    0x8888'8008)
GE_MSB_TEST(
    0x4771'1414,
    0x4641'1774,
    0x8888'8008)
GE_MSB_TEST(
    0x0123'4567,
    0x0000'0000,
    0x8888'8888)
GE_MSB_TEST(
    0x0123'4567,
    0x7777'7777,
    0x0000'0008)
GE_MSB_TEST(
    0x0000'0000,
    0x0123'4567,
    0x8000'0000)
GE_MSB_TEST(
    0x7777'7777,
    0x0123'4567,
    0x8888'8888)

// Replicate the msb off tests with the greaterEqual that allows msb on
#define GE_MSB_ON_TEST(left, right, result) static_assert(result == greaterEqual<4, u32>(SWAR<4, u32>(left), SWAR<4, u32>(right)).value());

GE_MSB_ON_TEST(
    0x1000'0010,
    0x0111'1101,
    0x8000'0080)
GE_MSB_ON_TEST(
    0x4333'3343,
    0x4444'4444,
    0x8000'0080)
GE_MSB_ON_TEST(
    0x0550'0110,
    0x0110'0550,
    0x8888'8008)
GE_MSB_ON_TEST(
    0x4771'1414,
    0x4641'1774,
    0x8888'8008)
GE_MSB_ON_TEST(
    0x0123'4567,
    0x0000'0000,
    0x8888'8888)
GE_MSB_ON_TEST(
    0x0123'4567,
    0x7777'7777,
    0x0000'0008)
GE_MSB_ON_TEST(
    0x0000'0000,
    0x0123'4567,
    0x8000'0000)
GE_MSB_ON_TEST(
    0x7777'7777,
    0x0123'4567,
    0x8888'8888)

TEST_CASE(
    "greaterEqualMSBOn",
    "[swar][unsigned-swar]"
) {
    SECTION("single S2_16") {
        for (uint32_t i = 1; i < 4; i++) {
            const auto left = S2_16{0}.blitElement(1,  i);
            const auto right = S2_16{S2_16::AllOnes}.blitElement(1, i-1);
            const auto test = S2_16{0}.blitElement(1, 2);
            CHECK(test.value() == greaterEqual<2, u16>(left, right).value());
        }
    }
    SECTION("single S4_32") {
        for (uint32_t i = 1; i < 15; i++) {
            const auto large = S4_32{0}.blitElement(1,  i+1);
            const auto small = S4_32{S4_32::AllOnes}.blitElement(1, i-1);
            const auto test = S4_32{0}.blitElement(1, 8);
            CHECK(test.value() == greaterEqual<4, u32>(large, small).value());
        }
    }
    SECTION("allLanes S4_32") {
        for (uint32_t i = 1; i < 15; i++) {
            const auto small = S4_32(S4_32::LeastSignificantBit * (i-1));
            const auto large = S4_32(S4_32::LeastSignificantBit * (i+1));
            const auto test = S4_32(S4_32::LeastSignificantBit * 8);
            CHECK(test.value() == greaterEqual<4, u32>(large, small).value());
        }
    }
    // This uncovered an extension required for non-power of two lane sizes for
    // equals/greaterEqual/etc related to the broadword nullcheck technique
    // from taocp pg 152
    // TODO(sbruce) figure out a performant fix.
/*
    SECTION("single 3_16") {
        for (uint32_t i = 1; i < 3; i++) {
            const auto left = S3_16{0}.blitElement(1,  i);
            const auto right = S3_16{S3_16::AllOnes}.blitElement(1, i-1);
            const auto test = S3_16{0}.blitElement(1, 2);
            CHECK(test.value() == greaterEqual<3, u16>(left, right).value());
        }
    }
*/
}

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
    // TODO(sbruce) add non power 2 lane width verifications of this (made easer by literals)
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
    CHECK(S4_16(0x0200).value() == saturatingUnsignedAddition(S4_16(0x0100), S4_16(0x0100)).value());
    CHECK(S4_16(0x0400).value() == saturatingUnsignedAddition(S4_16(0x0100), S4_16(0x0300)).value());
    CHECK(S4_16(0x0B00).value() == saturatingUnsignedAddition(S4_16(0x0800), S4_16(0x0300)).value());
    CHECK(S4_16(0x0F00).value() == saturatingUnsignedAddition(S4_16(0x0800), S4_16(0x0700)).value());
    CHECK(S4_16(0x0F00).value() == saturatingUnsignedAddition(S4_16(0x0800), S4_16(0x0800)).value());
    CHECK(S4_32(0x0F0C'F000).value() == saturatingUnsignedAddition(S4_32(0x0804'F000), S4_32(0x0808'F000)).value());
}

TEST_CASE(
    "first zero lane 4 wide 32 bit",
    "[swar]"
) {
    {
        // The last value / lsb null lane is all these care about, but the
        // implementation is so tied to the execution we check both.
        // firstZeroLane finds the least significant zero (little endian). This
        // results in the least significant zero being marked, and everything more
        // significant than it being turned into garbage data.
        CHECK(S4_32{0x0008'0080}.value() == firstZeroLane<4, uint32_t>( S4_32(0xaaa0'aa0a)).value());
        CHECK(1 == S4_32(firstZeroLane<4, uint32_t>( S4_32(0xaaa0'aa0a))).lsbIndex());
        CHECK(S4_32{0x8888'8888}.value() == firstZeroLane<4, uint32_t>( S4_32(0x1110'1110)).value());
        CHECK(S4_32{0x8800'0000}.value() == firstZeroLane<4, uint32_t>( S4_32(0x1025'3552)).value());
        CHECK(S4_32{0x8880'0000}.value() == firstZeroLane<4, uint32_t>( S4_32(0x1105'5352)).value());

        const char * c = "aaa\0aaa\0";
        uint64_t x;
        memcpy(&x, c, 8);
        S8_64 testBytes{x};
        auto nulls = firstZeroLane<8, uint64_t>(testBytes);
        CHECK(3 == S8_64(nulls).lsbIndex());
    }

    {
        CHECK(S4_32{0x0000'0000}.value() == firstMatchingLane<4, uint32_t>(S4_32(0x7321'4257), S4_32(0x0000'0000)).value());
        CHECK(S4_32{0x0000'8000}.value() == firstMatchingLane<4, uint32_t>(S4_32(0x7325'0257), S4_32(0x0000'0000)).value());
        CHECK(S4_32{0x0000'0000}.value() == firstMatchingLane<4, uint32_t>(S4_32(0x7341'4157), S4_32(0x2222'2222)).value());
        CHECK(S4_32{0x0880'0800}.value() == firstMatchingLane<4, uint32_t>(S4_32(0x7321'4257), S4_32(0x2222'2222)).value());
        CHECK(S4_32{0x0880'0800}.value() == firstMatchingLane<4, uint32_t>(S4_32(0x7321'4357), S4_32(0x2222'3333)).value());
        // Since only the least signficant zero and lower bits have
        // significance, we just mask out the don't care bits to illustrate for
        // utests.  When actually using this to find a lane instead of test for
        // its existence, one should probably just take the least significant
        // index.
        // We know our answer, so we mask out the dontcare higher bits.
        constexpr auto dontCareMask = S4_32{0x0000'0888};
        CHECK(S4_32{0x0000'0800}.value() == (firstMatchingLane<4, uint32_t>(S4_32(0x7321'4257), S4_32(0x2222'2222))&dontCareMask).value());
        CHECK(S4_32{0x0000'0800}.value() == (firstMatchingLane<4, uint32_t>(S4_32(0x7321'4357), S4_32(0x2222'3333))&dontCareMask).value());
    }

}

TEST_CASE(
    "equals S4_16",
    "[swar]"
) {
    // This test is intentionally horrific b/c blit / at / etc do not work right for 16 bit SWARs.
    auto incr_lane_zero = [](S4_16 s, int i)  ->S4_16{
        return s + S4_16{1};
    };
    auto incr_lane_one = [](S4_16 s, int i)  ->S4_16{
        return s + S4_16{0x0010};
    };
    auto incr_lane_two = [](S4_16 s, int i)  ->S4_16{
        return s + S4_16{0x0100};
    };
    auto incr_lane_three = [](S4_16 s, int i)  ->S4_16{
        return s + S4_16{0x1000};
    };
    auto cmp = [](S4_16 test, S4_16 expected, int i) ->S4_16 {return equals(test, expected);};
    auto expectedf_zero = [](S4_16 in, int i) ->S4_16 {return S4_16{S4_16::MostSignificantBit}.clear(0);};
    auto expectedf_one = [](S4_16 in, int i) ->S4_16 {return S4_16{S4_16::MostSignificantBit}.clear(1);};
    auto expectedf_two = [](S4_16 in, int i) ->S4_16 {return S4_16{S4_16::MostSignificantBit}.clear(2);};
    auto expectedf_three = [](S4_16 in, int i) ->S4_16 {return S4_16{S4_16::MostSignificantBit}.clear(3);};

    auto v = S4_16{0x7341};
    for (uint16_t i = 1; i < 15; i++) {
      uint16_t s = 0;
      for (int j = 0; j < i; j++) {
          s +=  v.value();
      }
      auto gen = S4_16{s};
      {
      auto right = incr_lane_zero(gen, i);
      auto expected = expectedf_zero(gen, i).value();
      CHECK(expected == cmp(gen, right, i).value());
      }
      {
      auto right = incr_lane_one(gen, i);
      auto expected = expectedf_one(gen, i).value();
      CHECK(expected == cmp(gen, right, i).value());
      }
      {
      auto right = incr_lane_two(gen, i);
      auto expected = expectedf_two(gen, i).value();
      CHECK(expected == cmp(gen, right, i).value());
      }
      {
      auto right = incr_lane_three(gen, i);
      auto expected = expectedf_three(gen, i).value();
      CHECK(expected == cmp(gen, right, i).value());
      }
    }
}

TEST_CASE(
    "equals S4_32",
    "[swar]"
) {
    auto incr_lane = [](S4_32 s, int lane,  int i)  ->S4_32{
        return s.blitElement(lane, (s.at(lane)+1)%16);
    };
    auto cmp = [](S4_32 test, S4_32 expected, int i) ->S4_32 {return equals(test, expected);};
    auto expectedf = [](S4_32 in, int lane, int i) ->S4_32 {return S4_32{S4_32::MostSignificantBit}.clear(lane);};

    auto v = S4_32{0x1357'acef};
    for (size_t lane = 0; lane < 8; lane++) {
        for (uint32_t i = 1; i < 15; i++) {
            auto gen = v.multiply(i);
            auto right = incr_lane(gen, lane, i);
            auto expected = expectedf(gen, lane, i).value();
            CHECK(expected == cmp(gen, right, i).value());
        }
    }
}

TEST_CASE(
    "equals S3_32",
    "[swar]"
) {
    auto incr_lane = [](S3_32 s, int lane,  int i)  ->S3_32{
        return s.blitElement(lane, (s.at(lane)+1)%8);
    };
    auto cmp = [](S3_32 test, S3_32 expected, int i) ->S3_32 {return equals(test, expected);};
    auto expectedf = [](S3_32 in, int lane, int i) ->S3_32 {return S3_32{S3_32::MostSignificantBit}.clear(lane);};

    auto v = S3_32{0x1357'acef};
    for (size_t lane = 0; lane < 10; lane++) {
        for (uint32_t i = 1; i < 15; i++) {
            auto gen = v.multiply(i);
            auto right = incr_lane(gen, lane, i);
            auto expected = expectedf(gen, lane, i).value();
            CHECK(expected == cmp(gen, right, i).value());
        }
    }
}

TEST_CASE(
    "greaterEqualGeneric S4_32",
    "[swar]"
) {
    auto incr_lane = [](S4_32 s, int lane,  int i)  ->S4_32{
        return s.blitElement(lane, s.at(lane)+1);
    };
    auto cmp = [](S4_32 test, S4_32 expected, int i) ->S4_32 {return greaterEqual(test, expected);};
    auto expectedf = [](S4_32 in, int lane, int i) ->S4_32 {return S4_32{S4_32::MostSignificantBit}.clear(lane);};

    auto v = S4_32{0x1357'acef};
    for (size_t lane = 0; lane < 8; lane++) {
        for (uint32_t i = 1; i < 15; i++) {
            // We want to ensure we check GE and we are incrementing a lane to do it, so all 15s must go
            constexpr auto destroy_fifteen = S4_32{0xeeee'eeee};
            auto gen = v.multiply(i) & destroy_fifteen;
            auto right = incr_lane(gen, lane, i);
            auto expected = expectedf(gen, lane, i).value();
            CHECK(expected == cmp(gen, right, i).value());
        }
    }
}

TEST_CASE(
    "greaterEqualGeneric S3_32",
    "[swar]"
) {
    auto incr_lane = [](S3_32 s, int lane,  int i)  ->S3_32{
        return s.blitElement(lane, s.at(lane)+1);
    };
    auto cmp = [](S3_32 test, S3_32 expected, int i) ->S3_32 {return greaterEqual(test, expected);};
    auto expectedf = [](S3_32 in, int lane, int i) ->S3_32 {return S3_32{S3_32::MostSignificantBit}.clear(lane);};

    auto v = S3_32{0x1357'acef};
    for (size_t lane = 0; lane < 8; lane++) {
        for (uint32_t i = 1; i < 15; i++) {
            // We want to ensure we check GE and we are incrementing a lane to do it, so all 7s must go
            // (use the literals library!!)
            // 11011011'01101101'10110110'110110xx
            constexpr auto destroy_seven = S3_32{0xdb6d'b6d8};
            auto gen = v.multiply(i) & destroy_seven;
            auto right = incr_lane(gen, lane, i);
            auto expected = expectedf(gen, lane, i).value();
            CHECK(expected == cmp(gen, right, i).value());
        }
    }
}

