#include "util/internals/any.h"

#ifdef TESTS
    #define CATCH_CONFIG_MAIN
#else
    #define CATCH_CONFIG_RUNNER
#endif
#include "catch.hpp"

struct Destructor {
    int *ptr;

    Destructor(int *p): ptr(p) {}

    ~Destructor() { *ptr = 1; }
};

struct alignas(16) D2: Destructor { using Destructor::Destructor; };

struct Big { double a, b; };

void debug() {};

TEST_CASE("Any", "[contract]") {
    SECTION("Value Destruction") {
        int value;
        {
            zoo::Any a{Destructor{&value}};
            REQUIRE(zoo::internals::AnyExtensions::isAValue<Destructor>(a));
            value = 0;
        }
        REQUIRE(1 == value);
    }
    SECTION("Referential Semantics - Alignment, Destruction") {
        int value;
        {
            zoo::Any a{D2{&value}};
            REQUIRE(zoo::internals::AnyExtensions::isReferential<D2>(a));
            value = 0;
        }
        REQUIRE(1 == value);
    }
    SECTION("Referential Semantics - Size") {
        zoo::Any v{Big{}};
        REQUIRE(zoo::internals::AnyExtensions::isReferential<Big>(v));
    }
    SECTION("Copy constructor") {
        zoo::Any a{5};
        debug();
        zoo::Any b{a};
        debug();
        REQUIRE(zoo::internals::AnyExtensions::isReferential<zoo::Any>(b));
    }
}
