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
    SECTION("std::function compatibility") {
        struct MyCallable
         {
            auto operator()(int i) {
                return long(i)*i;
            }
        };
        MyCallable myCallable;
        SECTION("operator bool()") {
            zoo::function<long(int)> ac;
            CHECK(!static_cast<bool>(ac));
            ac = myCallable;
            CHECK(static_cast<bool>(ac));
        }
        SECTION("swap()") {
            zoo::function<long(int)> ac1, ac2;
            ac2 = myCallable;
            CHECK(25 == ac2(5));
            CHECK(!static_cast<bool>(ac1));
            CHECK(static_cast<bool>(ac2));
            ac1.swap(ac2);
            CHECK(static_cast<bool>(ac1));
            CHECK(!static_cast<bool>(ac2));
            CHECK(25 == ac1(5));
        }
        SECTION("target_type()") {
            zoo::function<long(int)> ac;
            CHECK(ac.target_type() == typeid(void));
            ac = myCallable;
            CHECK(ac.target_type() == typeid(MyCallable));
        }
        SECTION("target() non-const") {
            zoo::function<long(int)> ac;
            CHECK(ac.target<int>() == nullptr);
            CHECK(ac.target<MyCallable>() == nullptr);
            ac = myCallable;
            CHECK(ac.target<int>() == nullptr);
            CHECK(ac.target<MyCallable>() != nullptr);
        }
        SECTION("target() const") {
            const zoo::function<long(int)> ac { myCallable };
            CHECK(ac.target<int>() == nullptr);
            CHECK(ac.target<MyCallable>() != nullptr);
        }
    }
}
