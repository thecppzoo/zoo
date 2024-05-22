#include "catch2/catch.hpp"

#include "zoo/swar/SWAR.h"

uint64_t calculateBase10(zoo::swar::SWAR<8, __uint128_t>) noexcept;

TEST_CASE("Calculate Base10", "[pure-test][swar][atoi]") {
    alignas(16) auto inputString = "1234567898765432";
    using S = zoo::swar::SWAR<8, __uint128_t>;
    S input;
    memcpy(&input.m_v, inputString, 16);
    constexpr S CharZeros{zoo::meta::BitmaskMaker<__uint128_t, '0', 8>::value};
    input = input - CharZeros;

    constexpr auto expected = __int128_t(12345678) * 100'000'000 + 98765432;
    auto calculated = calculateBase10(input);
    REQUIRE(expected == calculated);
}
