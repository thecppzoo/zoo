#include "AnyCallable.h"

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
        SECTION("comparison to nullptr") {
            zoo::function<long(int)> acEmpty;
            zoo::function<long(int)> acNonEmpty { myCallable };

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
            zoo::function<long(int)> acEmpty;
            zoo::function<long(int)> acNonEmpty { myCallable };
            CHECK(acEmpty.empty());
            CHECK(!acNonEmpty.empty());
        }
    }
}

TEST_CASE(
    "Non zoo::function tests",
    "[any][type-erasure][functional][zoo-enhancements]"
) {
    int copies = 0, moves = 0, destruction = 0;
    Traces t{copies, moves, destruction};

    SECTION("Uses type erasure optimized swap") {
        SECTION("Uses instance function") {
            SECTION("Erasure type defined before AnyCallable") {
                zoo::AnyCallable<BeforeAnyCallableEraserADL, void(int)>
                    lt1 = t,
                    lt2 = std::move(t);
                SECTION("Expected initialization") {
                    CHECK(1 == copies);
                    CHECK(1 == moves);
                    CHECK(0 == destruction);
                }
                SECTION("Swap using instance function") {
                    lt1.swap(lt2);
                    CHECK(4 == moves);
                    CHECK(1 == copies);
                    CHECK(0 == destruction);
                }
            }
            SECTION("Erasure type defined after AnyCallable, works") {
                zoo::AnyCallable<AfterAnyCallableEraser, void(int)>
                    eac1{t}, eac2{std::move(t)};
                eac1.swap(eac2);
                CHECK(0 == destruction);
            }
        }
        SECTION("Uses zoo::swap global") {
            SECTION("Uses ADL and upcasting to invoke erasure swap") {
                zoo::AnyCallable<BeforeAnyCallableEraserADL, void(int)>
                    lt1 = t,
                    lt2 = std::move(t);
                swap(lt1, lt2);
                CHECK(0 == destruction);
            }
            SECTION("Uses overload on template policy to invoke erasure swap") {
                zoo::AnyCallable<BeforeAnyCallableEraserP, void(int)>
                    lt1 = t,
                    lt2 = std::move(t);
                swap(lt1, lt2);
                CHECK(0 == destruction);
            }
            SECTION("Erasure type defined after AnyCallable") {
                zoo::AnyCallable<AfterAnyCallableEraser, void(int)>
                    lt1 = t,
                    lt2 = std::move(t);
                swap(lt1, lt2);
                CHECK(0 == destruction); // overload not available
            }
        }
    }

    SECTION("Optimized type-erasure swap *Not* available") {
        SECTION("Uses template on type overload") {
            zoo::AnyCallable<BeforeAnyCallableEraserT, void(int)>
                lt1 = t,
                lt2 = std::move(t);
            swap(lt1, lt2);
            CHECK(1 == destruction);
        }
    }
}
