#include "catch2/catch.hpp"
#include "../inc/zoo/swar/SWAR.h"
#include "../inc/zoo/swar/SWAR_string_utils.h"
#include <cstdint>

namespace zoo::swar {

using S = swar::SWAR<8, uint64_t>;
using BS = swar::BooleanSWAR<8, uint64_t>;

template <typename S>
constexpr static auto findEmptyLanes(const S &bytes) noexcept {
  constexpr auto NBits = S::NBits;
  using Type = typename S::type;
  using BS = zoo::swar::BooleanSWAR<NBits, Type>;
  constexpr auto Ones = S{S::LeastSignificantBit};

  // Subtracting 1 from a number which is null will cause a borrow from the lane
  // above which will cause the MSB to be set. This is the key to the algorithm.
  auto firstNullTurnsOnMSB = bytes - Ones;
  auto inverseBytes = ~bytes;

  // If the whole byte was null, the MBS will be set, AND the previous byte
  // will have been zero.
  auto firstMSBsOnIsFirstNull = firstNullTurnsOnMSB & inverseBytes;
  auto onlyMSBs = zoo::swar::convertToBooleanSWAR(firstMSBsOnIsFirstNull).value();
  return BS{onlyMSBs};
}

template <typename S>
constexpr static auto findFullLanes(const S &bytes) noexcept {
  constexpr auto NBits = S::NBits;
  using Type = typename S::type;
  using BS = zoo::swar::BooleanSWAR<NBits, Type>;

  constexpr auto Ones = S{S::LeastSignificantBit};
  constexpr auto Twos = Ones.value() << 1;

  auto noOnes = bytes & ~Ones;
  auto addTwos = noOnes.value() + Twos;
  auto firstFullTurnsOnMSB = addTwos >> 1;
  auto onlyMSBs = zoo::swar::convertToBooleanSWAR(S{firstFullTurnsOnMSB}).value();
  return BS{onlyMSBs};
}
static_assert(((((0x00'FF & ~0x0101) + 0x02'02) >> 1) & 0x8080)  == 0x00'80);
static_assert(((((0xFF'00 & ~0x0101) + 0x02'02) >> 1) & 0x8080)  == 0x80'00);

static_assert(findFullLanes(S{0x00'FF}).value() == 0x80);
static_assert(findFullLanes(S{0xFF'00}).value() == 0x80'00);
static_assert(findFullLanes(S{0xAA'BB'CC'FF}).value() == 0x00'00'00'80);
static_assert(findFullLanes(S{0xAA'FF'CC'FF}).value() == 0x00'80'00'80);

static_assert(findFullLanes(S{0xAA'FF'FC'FF}).value() == 0x00'80'00'80);
static_assert(findFullLanes(S{0xFF'FF'FF'FF}).value() == 0x80'80'80'80);

std::size_t strcmp(const char *a, const char *b) {
    using S = swar::SWAR<8, uint64_t>;
    constexpr auto BytesPerIteration = sizeof(S::type);

    constexpr auto Zeros = S::type{0};
    constexpr auto AllOnes = ~Zeros;

    S a_initialBytes{};
    auto [a_alignedBase, a_misalignment] = blockAlignedLoad(a, &a_initialBytes.m_v);
    auto a_bytes = adjustMisalignmentWithPattern<S, Zeros>(a_initialBytes, a_misalignment);

    S b_initialBytes{};
    auto [b_alignedBase, b_misalignment] = blockAlignedLoad(b, &b_initialBytes.m_v);
    auto b_bytes = adjustMisalignmentWithPattern<S, Zeros>(b_initialBytes, b_misalignment);

    for(;;) {
        auto diff = a_bytes.value() - b_bytes.value();
        auto either = a_bytes | b_bytes;
        auto emptyLanes = findEmptyLanes(either);

        if (diff != 0 || emptyLanes) {
            auto fill_ones = a_bytes.value() & AllOnes;
            return fill_ones - b_bytes.value();
        }

        a_alignedBase += BytesPerIteration;
        memcpy(&a_bytes, a_alignedBase, BytesPerIteration);

        b_alignedBase += BytesPerIteration;
        memcpy(&b_bytes, b_alignedBase, BytesPerIteration);
    }

    return 0;
}

}

constexpr auto isPositive(int value) {
    return value > 0;
}

constexpr auto isNegative(int value) {
    return value < 0;
}

constexpr auto isZero(int value) {
    return value == 0;
}

// don't care about dumb impl because testing for now, will fix later.
constexpr auto normalizeTernary(int value) {
    if(isPositive(value)) {
        return 1;
    } else if(isNegative(value)) {
        return -1;
    } else {
        return 0;
    }
}

using sv = std::string;
template <typename TestFn, typename GroundTruthFn>
constexpr auto testGroundTruth(const char* a, const char* b, TestFn&& testFunction, GroundTruthFn&& groundTruth) {
    auto result = testFunction(a, b);
    auto expected = groundTruth(a, b);
    auto normalizedResult = normalizeTernary(result);
    auto normalizedExpected = normalizeTernary(expected);
    return normalizedResult == normalizedExpected;
}

#define STRCMP_TESTS \
    X("aaaa", "aaaa") \
    X("a", "b") \
    X("b", "a") \
    X("hello", "hello") \
    X("1234", "123") \
    X("1234", "1235") \
    X("1115", "1194")


TEST_CASE("zoo::strcmp") {
    #define X(a, b) \
        REQUIRE(testGroundTruth(a, b, zoo::swar::strcmp, strcmp));
        STRCMP_TESTS
    #undef X
}


// #define STRLEN_FUNCTIONS \
//     X(_ZOO_STRLEN, jamie_demo::c_strLength) \
//     X(_NAIEVE_STRLEN, naieve) \
//     X(_LIBC_STRLEN, strlen) \
//     X(_ZOO_NEON, strlen) \
//
// #define X(FunctionName, FunctionToCall) \
//     struct Invoke##FunctionName { int operator()(const char *p) { return FunctionToCall(p); } };
//     STRLEN_FUNCTIONS
// #undef X
//
// #define X(Typename, _) \
//     BENCHMARK(jamie_demo::runBenchmark<CorpusStringLength, Invoke##Typename>);
//     STRLEN_FUNCTIONS
// #undef X
//
