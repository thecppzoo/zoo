#include "zoo/Str.h"

#include <catch2/catch.hpp>

TEST_CASE("String", "[str][api]") {
    using S = zoo::Str<void *>;
    SECTION("Potential regression") {
        char buff[8] = { "****#!-" };
        char Hi[] = { "Hello" };
        REQUIRE('\0' == buff[7]);
        auto inPlaced = new(buff) S(Hi, sizeof(Hi));
        REQUIRE('\0' == buff[5]);
        REQUIRE('-' == buff[6]);
        REQUIRE('\x2' == buff[7]);
    }
    SECTION("Empty/defaulted") {
        S empty;
        CHECK(not empty.onHeap());
        CHECK(0 == empty.size());
        CHECK('\0' == *empty.c_str());
    }
    auto checks =
        [](auto &givenString, bool onHeap) {
            auto facilitateComparisons = std::string(givenString);
            S s(givenString, facilitateComparisons.length() + 1);
            auto isOnHeap = s.onHeap();
            CHECK(onHeap == isOnHeap);

            auto c_str = s.c_str();
            CHECK(c_str == facilitateComparisons);
            CHECK(strlen(c_str) == facilitateComparisons.size());
            auto zooSize = s.size();
            CHECK(zooSize == facilitateComparisons.size());
        };
    SECTION("Local") {
        SECTION("Not boundary") {
            checks("Hello", false);
        }
        SECTION("Boundary of all chars needed") {
            static_assert(8 == sizeof(S));
            constexpr char Exactly7[] = { "1234567" };
            checks(Exactly7, false);
        }
    }
    SECTION("Heap") {
        constexpr char Quixote[] = "\
En un lugar de la Mancha que no quiero recordar\
";
        checks(Quixote, true);
    }
}
