#include "zoo/swar/SWAR.h"

#include "catch2/catch.hpp"

#include <type_traits>

namespace zoo::swar {

/// \note This code should be substituted by an application of "progressive" algebraic iteration
/// \note There is also parallelPrefix (to be implemented)
template<int NB, typename B>
constexpr SWAR<NB, B> parallelSuffix(SWAR<NB, B> input) {
    using S = SWAR<NB, B>;
    auto
        shiftClearingMask = S{~S::MostSignificantBit},
        doubling = input,
        result = S{0};
    auto
        bitsToXOR = NB,
        power = 1;
    for(;;) {
        if(1 & bitsToXOR) {
            result = result ^ doubling;
            doubling = doubling.shiftIntraLaneLeft(power, shiftClearingMask);
        }
        bitsToXOR >>= 1;
        if(!bitsToXOR) { break; }
        auto shifted = doubling.shiftIntraLaneLeft(power, shiftClearingMask);
        doubling = doubling ^ shifted;
        // 01...1
        // 001...1
        // 00001...1
        // 000000001...1
        shiftClearingMask =
            shiftClearingMask & S{shiftClearingMask.value() >> power};
        power <<= 1;
    }
    return S{result};
}

/// \todo because of the desirability of "accumuating" the XORs at the MSB,
/// the parallel suffix operation is more suitable.
template<int NB, typename B>
constexpr SWAR<NB, B> parity(SWAR<NB, B> input) {
    using S = SWAR<NB, B>;
    auto preResult = parallelSuffix(input);
    auto onlyMSB = preResult.value() & S::MostSignificantBit;
    return S{onlyMSB};
}


/*
Execution trace at two points:
1. when checking `if(1 & count)`
2. when checking `if(!count)`
If the variable did not change from the last value, it may be ommitted
input       Count       x       d       power   mask
1           1           0       x0      1       01111...
            0           x0
2           2           0       x0      1       01111...
            1           0       x0      1       01111...
            1           0       x01     2       00111...
            0           x01
3           3           0       x0      1       01111...
            1           x0      x1      1       01111...
            1           x0      x12     2       00111...
            0           x012    x23
4           4           0       x0      1       01111...
            2           0       x0      1       01111...
            2           0       x01     2       00111...
            1           0       x01
            1           0       x0123   4       00001...
            0           x0123   x01234567
5           5           0       x0      1       01111...
            2           x0      x1
            2           x0      x12     2       00111...
            1
            1                   x1234   4       00001...
            0           x01234
6           6           0       x0      1       01111...
            3
            3                   x01     2       00111...
            1           x01     x23     
            1                   x2345   4       00001...
            0           x012345 x6789
7           7           0       x0      1       01......
            3           x0      x1
            3                   x12     2       001.....
            1           x012    x34
            1                   x3456   4       00001...
            0           x0-6    x789A
25 = 16 + 8 + 1
25          25          0       x0      1       01111...
            12          x0      x1
            12                  x12     2       00111...
            6
            6                   x1234   4       {0}4
            3
            3                   x1-8    8       {0}8
            1           x0-8    x9-16
            1                   x9-24   16      {0}16
            0           x0-24   x25-?
*/

template<int NB, typename B>
struct ArithmeticResultTriplet {
    SWAR<NB, B> result;
    BooleanSWAR<NB, B> carry, overflow;
};

namespace impl {
template<int NB, typename B>
constexpr auto makeLaneMaskFromMSB_and_LSB(SWAR<NB, B> msb, SWAR<NB, B> lsb) {
    auto msbCopiedDown = msb - lsb;
    auto msbReintroduced = msbCopiedDown | msb;
    return msbReintroduced;
}
}

template<int NB, typename B>
constexpr auto makeLaneMaskFromLSB(SWAR<NB, B> input) {
    using S = SWAR<NB, B>;
    auto lsb = input & S{S::LeastSignificantBit};
    auto lsbCopiedToMSB = S{lsb.value() << (NB - 1)};
    return impl::makeLaneMaskFromMSB_and_LSB(lsbCopiedToMSB, lsb);
}

template<int NB, typename B>
constexpr auto makeLaneMaskFromMSB(SWAR<NB, B> input) {
    using S = SWAR<NB, B>;
    auto msb = input & S{S::MostSignificantBit};
    auto msbCopiedToLSB = S{msb.value() >> (NB - 1)};
    return impl::makeLaneMaskFromMSB_and_LSB(msb, msbCopiedToLSB);
}

template<int NB, typename B>
constexpr auto fullAddition(SWAR<NB, B> s1, SWAR<NB, B> s2) {
    using S = SWAR<NB, B>;
    constexpr auto
        SignBit = S{S::MostSignificantBit},
        LowerBits = SignBit - S{S::LeastSignificantBit};
    // prevent overflow by clearing the most significant bits
    auto
        s1prime = LowerBits & s1,
        s2prime = LowerBits & s2,
        resultPrime = s1prime + s2prime,
        s1Sign = SignBit & s1,
        s2Sign = SignBit & s2,
        signPrime = SignBit & resultPrime,
        result = resultPrime ^ s1Sign ^ s2Sign,
        // carry is set whenever at least two of the sign bits of s1, s2,
        // signPrime are set
        carry = (s1Sign & s2Sign) | (s1Sign & signPrime) | (s2Sign & signPrime),
        // overflow: the inputs have the same sign and different to result
        // same sign: s1Sign ^ s2Sign
        overflow = (s1Sign ^ s2Sign ^ SignBit) & (s1Sign ^ result);
    return ArithmeticResultTriplet<NB, B>(result, carry, overflow);
}

/// \brief Performs a generalized iterated application of an associative operator to a base
///
/// In algebra, the repeated application of an operator to a "base" has different names depending on the
/// operator, for example "a + a + a + ... + a" n-times would be called "repeated addition",
/// if * is numeric multiplication, "a * a * a * ... * a" n-times would be called "exponentiation of a to the n power"
/// the generic term we use is "iteration" for naming this function.
/// Since * and "product" are frequently used in Algebra to denote the application of a general operator, we
/// keep the option to use the imprecise language of "product, base and exponent".  "Iteration" has a very
/// different meaning in programming and especially different in C++.
/// There may be iteration over an operator that is not associative (such as quaternion multiplication), this
/// function leverages the associative property of the operator to "halve" the count of iterations at each step.
/// \note There is a symmetrical operation to be implemented of associative iteration in the
/// "progressive" direction: instead of starting with the most significant bit of the count, down to the lsb,
/// and doing "op(result, base, count)"; going from lsb to msb doing "op(result, square, exponent)"
/// \tparam Operator a callable with three arguments: the left and right arguments to the operation
/// and the count to be used, the "count" is an artifact of this generalization
/// \tparam IterationCount loosely models the "exponent" in "exponentiation", however, it may not
/// be a number, the iteration count is part of the execution context to apply the operator
/// \param forSquaring is an artifact of this generalization
/// \param log2Count is to potentially reduce the number of iterations if the caller a-priory knows
/// there are fewer iterations than what the type of exponent would allow
template<
    typename Base, typename IterationCount, typename Operator,
    // the critical use of associativity is that it allows halving the
    // iteration count
    typename CountHalver
>
constexpr auto associativeOperatorIterated_regressive(
    Base base, Base neutral, IterationCount count, IterationCount forSquaring,
    Operator op, unsigned log2Count, CountHalver ch
) {
    auto result = neutral;
    if(!log2Count) { return result; }
    for(;;) {
        result = op(result, base, count);
        if(!--log2Count) { break; }
        result = op(result, result, forSquaring);
        count = ch(count);
    }
    return result;
}

template<int ActualBits, int NB, typename T>
constexpr auto multiplication_OverflowUnsafe_SpecificBitCount(
    SWAR<NB, T> multiplicand, SWAR<NB, T> multiplier
) {
    using S = SWAR<NB, T>;

    auto operation = [](auto left, auto right, auto counts) {
        auto addendums = makeElementMaskFromMSB(counts);
        return left + (addendums & right);
    };

    auto halver = [](auto counts) {
        auto msbCleared = counts & ~S{S::MostSignificantBit};
        return S{msbCleared.value() << 1};
    };

    multiplier = S{multiplier.value() << (NB - ActualBits)};
    return associativeOperatorIterated_regressive(
        multiplicand, S{0}, multiplier, S{S::MostSignificantBit}, operation,
        ActualBits, halver
    );
}

/// \note Not removed yet because it is an example of "progressive" associative exponentiation
template<int ActualBits, int NB, typename T>
constexpr auto multiplication_OverflowUnsafe_SpecificBitCount_deprecated(
    SWAR<NB, T> multiplicand,
    SWAR<NB, T> multiplier
) {
    using S = SWAR<NB, T>;
    constexpr auto LeastBit = S::LeastSignificantBit;
    auto multiplicandDoubling = multiplicand.value();
    auto mplier = multiplier.value();
    auto product = S{0};
    for(auto count = ActualBits;;) {
        auto multiplicandDoublingMask = makeLaneMaskFromLSB(S{mplier});
        product = product + (multiplicandDoublingMask & S{multiplicandDoubling});
        if(!--count) { break; }
        multiplicandDoubling <<= 1;
        auto leastBitCleared = mplier & ~LeastBit;
        mplier = leastBitCleared >> 1;
    }
    return product;
}

template<int NB, typename T>
constexpr auto multiplication_OverflowUnsafe(
    SWAR<NB, T> multiplicand,
    SWAR<NB, T> multiplier
) {
    return
        multiplication_OverflowUnsafe_SpecificBitCount<NB>(
            multiplicand, multiplier
        );
}

template<int NB, typename T>
struct SWAR_Pair{
    SWAR<NB, T> even, odd;
};

template<int NB, typename T>
constexpr SWAR<NB, T> doublingMask() {
    using S = SWAR<NB, T>;
    static_assert(0 == S::Lanes % 2, "Only even number of elements supported");
    using D = SWAR<NB * 2, T>;
    return S{(D::LeastSignificantBit << NB) - D::LeastSignificantBit};
}

template<int NB, typename T>
constexpr auto doublePrecision(SWAR<NB, T> input) {
    using S = SWAR<NB, T>;
    static_assert(
        0 == S::NSlots % 2,
        "Precision can only be doubled for SWARs of even element count"
    );
    using RV = SWAR<NB * 2, T>;
    constexpr auto DM = doublingMask<NB, T>();
    return SWAR_Pair<NB * 2, T>{
        RV{(input & DM).value()},
        RV{(input.value() >> NB) & DM.value()}
    };
}

template<int NB, typename T>
constexpr auto halvePrecision(SWAR<NB, T> even, SWAR<NB, T> odd) {
    using S = SWAR<NB, T>;
    static_assert(0 == NB % 2, "Only even lane-bitcounts supported");
    using RV = SWAR<NB/2, T>;
    constexpr auto HalvingMask = doublingMask<NB/2, T>();
    auto
        evenHalf = RV{even.value()} & HalvingMask,
        oddHalf = RV{(RV{odd.value()} & HalvingMask).value() << NB/2};
    return evenHalf | oddHalf;
}

}

using namespace zoo;
using namespace zoo::swar;

namespace Multiplication {

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

// 3 bits on msb side, 5 bits on lsb side.
using Lanes = SWARWithSubLanes<5, 3, u32>;
using S8u32 = SWAR<8, u32>;
static constexpr inline u32 all0 = 0;
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

static_assert(0x0000'001f == Lanes(all0).least(31, 0).most(0, 0).value());
static_assert(0x0000'1f00 == Lanes(all0).least(31, 1).most(0, 1).value());
static_assert(0x001f'0000 == Lanes(all0).least(31, 2).most(0, 2).value());
static_assert(0x1f00'0000 == Lanes(all0).least(31, 3).most(0, 3).value());

static_assert(0x0000'00e0 == Lanes(all0).least(0, 0).most(31, 0).value());
static_assert(0x0000'e000 == Lanes(all0).least(0, 1).most(31, 1).value());
static_assert(0x00e0'0000 == Lanes(all0).least(0, 2).most(31, 2).value());
static_assert(0xe000'0000 == Lanes(all0).least(0, 3).most(31, 3).value());

static_assert(0x1F1F'1F1F == Lanes(allF).least().value());
static_assert(0xE0E0'E0E0 == Lanes(allF).most().value());

static_assert(0x0000'001F == Lanes(allF).least(0).value());
static_assert(0x0000'1F00 == Lanes(allF).least(1).value());
static_assert(0x001F'0000 == Lanes(allF).least(2).value());
static_assert(0x1F00'0000 == Lanes(allF).least(3).value());

static_assert(0x0000'00E0 == Lanes(allF).most(0).value());
static_assert(0x0000'E000 == Lanes(allF).most(1).value());
static_assert(0x00E0'0000 == Lanes(allF).most(2).value());
static_assert(0xE000'0000 == Lanes(allF).most(3).value());

static_assert(0x123 == SWAR<4, uint32_t>(0x173).blitElement(1, 2).value());
static_assert(0 == isolateLSB(u32(0)));

constexpr auto aBooleansWithTrue = booleans(SWAR<4, u32>{0x1});
static_assert(aBooleansWithTrue);
static_assert(!aBooleansWithTrue); // this is a pitfall, but lesser evil?
static_assert(false == !bool(aBooleansWithTrue));
