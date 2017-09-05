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

#include <memory>

template<typename> struct Trick;

TEST_CASE("WEIRD", "") {
    long maxInt = static_cast<unsigned long>(long{-1}) >> 1;
    auto minInt = maxInt + 1;
    auto z = minInt < 0;
    REQUIRE(z);
}

TEST_CASE("Encodings", "[TightPolicy]") {
    Tight t;
    auto &e = t.code.empty;
    SECTION("Empty") {
        bool emptiness = !e.isInteger && e.notPointer && !e.isString;
        REQUIRE(emptiness);
    }
    SECTION("Int63 - does not preserve negatives") {
        t.code.integer = -1;
        REQUIRE(e.isInteger);
        REQUIRE(-1 == long{t.code.integer});
    }
    SECTION("Int63 - not exactly 63 bits of precision") {
        long maxInt = static_cast<unsigned long>(long{-1}) >> 1;
        REQUIRE(0 < maxInt);
        t.code.integer = maxInt;
        REQUIRE(-1 == t.code.integer);
        //long minInt = static_cast<unsigned long>(maxInt) + 1;
        auto minInt = maxInt + 1;
        static_assert(std::is_same<decltype(minInt), long>::value, "");
        auto lessThanZero = minInt < long(0);
        std::cout << std::hex << minInt << std::endl;
        REQUIRE(lessThanZero);
        REQUIRE(-1 == maxInt + minInt);
        t.code.integer = minInt;
        REQUIRE(0 == t.code.integer);
    }
    SECTION("Pointer62") {
        std::unique_ptr<int> forget{new int{8}};
        auto ptr = forget.get();
        t.code.pointer = ptr;
        auto pointerness = !e.isInteger && !e.notPointer;
        REQUIRE(pointerness);
        REQUIRE(ptr == t.code.pointer);
    }
    SECTION("String7") {
        char noNull[] = { 'H', 'e', 'l', 'l', 'o', '!' };
        static_assert(std::is_same<char[6], decltype(noNull)>::value, "");
        t.code.string = noNull;
        bool stringness = !e.isInteger && e.notPointer && e.isString;
        REQUIRE(stringness);
        REQUIRE(6 == t.code.string.count);
        std::string
            asString = t.code.string,
            helloStr{noNull, noNull + sizeof(noNull)};
        REQUIRE(helloStr == asString);
        char eightBytes[] = "1234567";
        static_assert(8 == sizeof(eightBytes), "");
        t.code.string = String7{eightBytes, 7};
        asString = t.code.string;
        REQUIRE(eightBytes == asString);
        asString = "!";
        asString += "!";
        t.code.string = asString;
        REQUIRE(2 == t.code.string.count);
    }
}
