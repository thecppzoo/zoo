#include "catch2/catch.hpp"

#include "zoo/swar/SWAR.h"
#include "c_str.h"

uint64_t calculateBase10(zoo::swar::SWAR<8, __uint128_t>) noexcept;
namespace zoo {
int64_t c_strToL(const char *str) noexcept;
}

template<int RequiredAlignment>
void requireAlignment(const void *p) {
    std::uintptr_t asNumber = reinterpret_cast<std::uintptr_t>(p);
    REQUIRE(asNumber % RequiredAlignment == 0);
}

TEST_CASE("Calculate Base10", "[pure-test][swar][atoi]") {
    alignas(16) char inputString[] = "1234567898765432";
    requireAlignment<16>(inputString);
    using S = zoo::swar::SWAR<8, __uint128_t>;
    S input;
    memcpy(&input.m_v, inputString, 16);
    constexpr S CharZeros{zoo::meta::BitmaskMaker<__uint128_t, '0', 8>::value};
    input = input - CharZeros;

    constexpr auto expected = __int128_t(12345678) * 100'000'000 + 98765432;
    auto calculated = calculateBase10(input);
    REQUIRE(expected == calculated);
}

TEST_CASE("Atol", "[pure-test][swar][atoi]") {
    // Because of the code sharing between the implementation of atoi and atol,
    // and because atoi has been thoroughly tested in the benchmarking,
    // there isn't much to test of atol, other than being able to deal with
    // numbers higher than representable with an integer and 16-byte
    // misalignment.
    alignas(16) char inputString[] = "0123456789-\f9123456789987654321";
    // notice 91...99...1 is very close to 2^63 - 1, max for 64 signed
    // log2(9123456789987654321) ~= 62.984
    requireAlignment<16>(inputString);
    // Notice the second number, 91...99...1 is misaligned by 12 bytes
    SECTION("Aligned atol") {
        auto expected = 123456789;
        auto converted = zoo::c_strToL(inputString);
        REQUIRE(converted == expected);
    }
    SECTION("Misaligned atol") {
        auto expected = 9123456789987654321ll;
        auto secondNumber = inputString + 11;
        REQUIRE(std::string("\f9123456789987654321") == secondNumber);
        auto converted = zoo::c_strToL(secondNumber);
        REQUIRE(converted == expected);
    }
}

TEST_CASE("String function regressions", "[pure-test][atoi][swar]") {
    auto input = "\x80\0";
    REQUIRE(1 == zoo::c_strLength(input));
}
