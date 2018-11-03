#define ZOO_USE_EXPERIMENTAL

#include <zoo/function.h>

#include <catch2/catch.hpp>

TEST_CASE("function", "[any][type-erasure][functional]") {
    SECTION("Default throws bad_function_call") {
        zoo::function<void()> defaultConstructed;
        REQUIRE_THROWS_AS(defaultConstructed(), std::bad_function_call);
    }
    SECTION("Accepts function pointers") {
        auto nonCapturingLambda = [](int i) { return long(i)*i; };
        zoo::function<long(int)> ac{nonCapturingLambda};
        REQUIRE(9 == ac(3));
    }
    SECTION("Accepts function objects") {
        int state = 0;
        auto capturingLambda = [&](int arg) {
            state = arg;
            return 2*arg;
        };
        zoo::function<int(int)> ac{capturingLambda};
        REQUIRE(2 == ac(1));
        REQUIRE(1 == state);
        SECTION("Copy construction") {
            auto copy = ac;
            CHECK(14 == copy(7));
            CHECK(7 == state);
            SECTION("Copy construction copies the target, not the wrapper") {
                CHECK(
                    typeid(decltype(capturingLambda)).name() ==
                    copy.type().name()
                );
            }
        }
    }
    SECTION("Assignment") {
        struct MovableCallable {
            bool moved_ = false;
            MovableCallable() = default;
            MovableCallable(const MovableCallable &) = default;
            MovableCallable(MovableCallable &&other) noexcept {
                other.moved_ = true;
            }
            void operator()() {}
        };
        zoo::function<void()>
            ac{MovableCallable{}},
            empty;
        SECTION("Move assignment") {
            auto &heldObject = *ac.state<MovableCallable>();
            REQUIRE_FALSE(heldObject.moved_);
            empty = std::move(ac);
            CHECK(heldObject.moved_);
        }
    }
}
