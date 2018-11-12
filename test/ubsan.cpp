#include <zoo/any.h>

#include <catch2/catch.hpp>

TEST_CASE("UBSAN") {
    struct Large {
        double d1, d2;
    };
    static_assert(sizeof(zoo::Any) <= sizeof(Large), "may use in-place");
    zoo::Any originallyEmpty, originallyNotEmpty{Large{}};
    REQUIRE(not originallyEmpty.has_value());
    REQUIRE(originallyNotEmpty.has_value());
    swap(originallyEmpty, originallyNotEmpty);
    CHECK(originallyEmpty.has_value());
    CHECK(not originallyNotEmpty.has_value());
}
