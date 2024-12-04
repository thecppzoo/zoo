#include "zoo/Str.h"

#include <catch2/catch.hpp>

using S = zoo::Str<void *>;

static_assert(std::is_nothrow_default_constructible_v<S>);

char BufferAsBigAsStr[sizeof(S)];
static_assert(noexcept(S{BufferAsBigAsStr}));

char BufferLargerThanStr[sizeof(S) * 2];
static_assert(not noexcept(S{BufferLargerThanStr}));
static_assert(std::is_nothrow_move_constructible_v<S>);

constexpr char chars257[] = "\
0123456789ABCDEF0123456789abcdef0123456789ABCDEF0123456789abcdef\
we can put anything here, since we can see it is 64-bytes wide--\
continuing, spaces are also good                               -\
this is the last line                                         !!\
";
static_assert(257 == sizeof(chars257));

template<typename StringType>
auto stringChecks(const char *givenString, bool onHeap) {
    auto facilitateComparisons = std::string(givenString);
    StringType s(givenString, facilitateComparisons.length() + 1);
    auto isOnHeap = s.onHeap();
    CHECK(onHeap == isOnHeap);

    auto c_str = s.c_str();
    CHECK(c_str == facilitateComparisons);
    CHECK(strlen(c_str) == facilitateComparisons.size());
    auto zooSize = s.size();
    CHECK(zooSize == facilitateComparisons.size());
};

TEST_CASE("String", "[str][api]") {
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
    constexpr char Exactly7[] = { "1234567" };
    char Exactly15[] = "123456789ABCDEF";
    constexpr char Quixote[] = "\
En un lugar de la Mancha que no quiero recordar\
";
    auto *checks = stringChecks<S>;
    SECTION("Local") {
        SECTION("Not boundary") {
            checks("Hello", false);
        }
        SECTION("Boundary of all chars needed") {
            static_assert(8 == sizeof(S));
            checks(Exactly7, false);
        }
    }
    SECTION("Heap") {
        checks(Exactly15, true);
        checks(Quixote, true);
        SECTION("Case in which 1 < the bytes for size") {
            checks(chars257, true);
        }
    }
    SECTION("Larger than void *") {
        using S2 = zoo::Str<void *[2]>;
        stringChecks<S2>(Exactly7, false);
        stringChecks<S2>(Exactly15, false);
        stringChecks<S2>(Quixote, true);
    }
}
