#include "TightPolicy.h"

static_assert(!is_stringy_type<int>::value, "");
static_assert(is_stringy_type<std::string>::value, "");

struct has_chars {
    char spc[8];
};

static_assert(is_stringy_type<decltype(has_chars::spc)>::value, "");

#ifdef TESTS
    #define CATCH_CONFIG_MAIN
#else
    #define CATCH_CONFIG_RUNNER
#endif
#include "catch.hpp"

TEST_CASE("Encodings", "[TightPolicy]") {
    Tight t;
    SECTION("Empty") {
        Empty e = t.code.empty;
        bool emptiness = !e.isInteger && e.notPointer && !e.isString;
        REQUIRE(emptiness);
    }
    SECTION("Int63 - does not preserve negatives") {
        t.code.integer = -1;
        REQUIRE(-1 == long{t.code.integer});
    }
    SECTION("Int63 - not exactly 63 bits of precision") {
        long maxInt = static_cast<unsigned long>(long{-1}) >> 1;
        REQUIRE(0 < maxInt);
        t.code.integer = maxInt;
        REQUIRE(-1 == t.code.integer);
        auto minInt = maxInt + 1;
        REQUIRE(minInt < 0);
        REQUIRE(-1 == maxInt + minInt);
        t.code.integer = minInt;
        REQUIRE(0 == t.code.integer);
    }
}
