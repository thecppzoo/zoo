#include "zoo/root/mem.h"

#include "catch2/catch.hpp"

#include <stdint.h>

TEST_CASE("Block aligned load", "[root][mem][api]") {
    alignas(8) char aBuffer[32] = "0123456789ABCDEFfedcba987654321";
    auto plus5 = aBuffer + 5;
    alignas(8) char destination[9];
    auto uPtr = reinterpret_cast<uint64_t *>(destination);
    auto [base, misalignment] = zoo::blockAlignedLoad(plus5, uPtr);
    REQUIRE(base == aBuffer);
    REQUIRE(5 == misalignment);
    destination[8] = '\0';
    REQUIRE(std::string(aBuffer, aBuffer + 8) == destination);
}
