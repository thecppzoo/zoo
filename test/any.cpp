#include "util/internals/any.h"

#ifdef TESTS
    #define CATCH_CONFIG_MAIN
#else
    #define CATCH_CONFIG_RUNNER
#endif
#include "catch.hpp"

struct Big { double a, b; };

TEST_CASE("Any", "[contract]") {
    SECTION("Value Semantics") {
        zoo::Any v{5};
        REQUIRE(zoo::internals::AnyExtensions::isAValue<int>(v));
    }
    SECTION("Referential Semantics") {
        zoo::Any v{Big{}};
        REQUIRE(zoo::internals::AnyExtensions::isReferential<Big>(v));
    }
}
