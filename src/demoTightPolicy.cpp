#define STRING7_TESTS

#include "TightPolicy.h"

String7::ConstructorOverload lastOverload;
void String7::c(ConstructorOverload o) { lastOverload = o; }

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
        SECTION("Overflows at 63 bits") {
            REQUIRE(0 < maxInt);
            t.code.integer = maxInt;
            REQUIRE(-1 == t.code.integer);
        }
        SECTION("Drops the most significant bit") {
            long minInt = static_cast<unsigned long>(maxInt) + 1;
            REQUIRE(minInt < 0);
            REQUIRE(-1 == maxInt + minInt);
            t.code.integer = minInt;
            REQUIRE(0 == t.code.integer);
        }
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
        SECTION("Correct encoding") {
            bool stringness = !e.isInteger && e.notPointer && e.isString;
            REQUIRE(stringness);
            CHECK(String7::ARRAY == lastOverload);
        }
        SECTION("Correct count from char[]") {
            REQUIRE(6 == t.code.string.count);
        }
        std::string asString = "hi!";
        SECTION("Conversion to std::string") {
            std::string helloStr{noNull, noNull + sizeof(noNull)};
            asString = t.code.string;
            REQUIRE(helloStr == asString);
        }
    }
}

auto isPointer(Tight t) {
    auto e = t.code.empty;
    return !e.isInteger && !e.notPointer;
}

template<typename T>
struct DBuilder: Builder<T> {
    using Builder<T>::Builder;
 
    ~DBuilder() {
        if(!isPointer(*this)) { return; }
        void *p = this->code.pointer;
        delete static_cast<Fallback *>(p);
    }
};

TEST_CASE("Builders", "[TightPolicy]") {
    SECTION("Fallback") {
        DBuilder<double> b{3.1415265};
        bool fellback = !b.code.empty.isInteger && !b.code.empty.notPointer;
        REQUIRE(fellback);
    }
    SECTION("Stringies") {
        auto isString = [](Tight t) {
            auto e = t.code.empty;
            return !e.isInteger && e.notPointer && e.isString;
        };
        char hello[] = "Hello!";
        SECTION("Small strings") {
            std::string ss{hello};
            DBuilder<std::string> built(ss);
            REQUIRE(isString(built));
            CHECK(String7::POINTER_COUNT == lastOverload);
        }
        SECTION("Large string") {
            std::string ls{"This is a large string"};
            DBuilder<std::string> built(ls);
            REQUIRE(isPointer(built));
        }
    }
}
