#include "CallableHelper.h"

#include <catch2/catch.hpp>

template<typename ErasureProvider>
struct CallableTests {
    template<typename Signature>
    using ZFunction = zoo::AnyCallable<ErasureProvider, Signature>;

    inline static void execute();
};

template<typename ErasureProvider>
void CallableTests<ErasureProvider>::execute() {
    SECTION("Default throws bad_function_call") {
        ZFunction<void()> defaultConstructed;
        REQUIRE_THROWS_AS(defaultConstructed(), std::bad_function_call);
    }
    SECTION("Accepts function pointers") {
        auto nonCapturingLambda = [](int i) { return long(i)*i; };
        ZFunction<long(int)> ac{nonCapturingLambda};
        REQUIRE(9 == ac(3));
    }
    SECTION("Accepts function objects") {
        int state = 0;
        auto capturingLambda = [&](int arg) {
            state = arg;
            return 2*arg;
        };
        ZFunction<int(int)> ac{capturingLambda};
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
            MovableCallable(const MovableCallable &) {};
            MovableCallable(MovableCallable &&other) noexcept {
                other.moved_ = true;
            }
            void operator()() {}
        };
        ZFunction<void()>
            ac{MovableCallable{}},
            empty;
        SECTION("Move assignment") {
            auto &heldObject = *ac.template state<MovableCallable>();
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
            ZFunction<long(int)> ac;
            CHECK(!static_cast<bool>(ac));
            ac = myCallable;
            CHECK(static_cast<bool>(ac));
        }
        SECTION("swap()") {
            ZFunction<long(int)> ac1, ac2;
            SECTION("Synopsis") {
                ac2 = myCallable;
                CHECK(25 == ac2(5));
                CHECK(!static_cast<bool>(ac1));
                CHECK(static_cast<bool>(ac2));
                ac1.swap(ac2);
                CHECK(static_cast<bool>(ac1));
                CHECK(!static_cast<bool>(ac2));
                CHECK(25 == ac1(5));
                std::swap(ac1, ac2);
                CHECK(25 == ac2(5));
                CHECK(!static_cast<bool>(ac1));
                CHECK(static_cast<bool>(ac2));
            }
        }
        SECTION("target_type()") {
            ZFunction<long(int)> ac;
            CHECK(ac.target_type() == typeid(void));
            ac = myCallable;
            CHECK(ac.target_type() == typeid(MyCallable));
        }
        SECTION("target() non-const") {
            ZFunction<long(int)> ac;
            CHECK(ac.template target<int>() == nullptr);
            CHECK(ac.template target<MyCallable>() == nullptr);
            ac = myCallable;
            CHECK(ac.template target<int>() == nullptr);
            CHECK(ac.template target<MyCallable>() != nullptr);
        }
        SECTION("target() const") {
            const ZFunction<long(int)> ac { myCallable };
            CHECK(ac.template target<int>() == nullptr);
            CHECK(ac.template target<MyCallable>() != nullptr);
        }
        SECTION("comparison to nullptr") {
            ZFunction<long(int)> acEmpty;
            ZFunction<long(int)> acNonEmpty { myCallable };

            CHECK(acEmpty == nullptr);
            CHECK(nullptr == acEmpty);
            CHECK(!(acEmpty != nullptr));
            CHECK(!(nullptr != acEmpty));

            CHECK(acNonEmpty != nullptr);
            CHECK(nullptr != acNonEmpty);
            CHECK(!(acNonEmpty == nullptr));
            CHECK(!(nullptr == acNonEmpty));
        }
        SECTION("empty()") {
            ZFunction<long(int)> acEmpty;
            ZFunction<long(int)> acNonEmpty { myCallable };
            CHECK(acEmpty.empty());
            CHECK(!acNonEmpty.empty());
        }
    }
}
