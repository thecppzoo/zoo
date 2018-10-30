#define ZOO_USE_EXPERIMENTAL

#include <zoo/AnyCallable.h>

#include <catch2/catch.hpp>

TEST_CASE("AnyCallable", "[any][type-erasure][functional]") {
    SECTION("Default throws bad_function_call") {
        zoo::AnyCallable<void()> defaultConstructed;
        REQUIRE_THROWS_AS(defaultConstructed(), std::bad_function_call);
    }
    SECTION("Accepts function pointers") {
        auto nonCapturingLambda = [](int i) { return long(i)*i; };
        zoo::AnyCallable<long(int)> ac{nonCapturingLambda};
        REQUIRE(9 == ac(3));
    }
    SECTION("Accepts function objects") {
        int state = 0;
        auto capturingLambda = [&](int arg) {
            state = arg;
            return 2*arg;
        };
        zoo::AnyCallable<int(int)> ac{capturingLambda};
        REQUIRE(2 == ac(1));
        REQUIRE(1 == state);
    }
}
