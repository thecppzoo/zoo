#include "any.h"

#include <catch2/catch.hpp>

#include <memory>

TEST_CASE("Extended AnyContainer tests") {
    SECTION("No leaks") {
        struct TracesDestruction {
            int &counter_, state_ = 0;
            TracesDestruction(int &trace): counter_{trace} {}
            ~TracesDestruction() { ++counter_; }
        };
        struct AOD {
            std::shared_ptr<TracesDestruction> ptr_;
            AOD() = default;
            AOD(const AOD &) = default;
            AOD(int &c): ptr_{new TracesDestruction{c}} {}
        };
        int d1 = 0, d2 = 0;
        {
            zoo::AnyContainer<LargePolicy>
                a1{TracesDestruction{d1}},
                a2{TracesDestruction{d2}};
            a1.state<TracesDestruction>()->state_ = 1;
            a2.state<TracesDestruction>()->state_ = 2;
            REQUIRE(1 == d1);
            REQUIRE(1 == d2);
            swap(a1, a2);
            REQUIRE(2 == a1.state<TracesDestruction>()->state_);
            REQUIRE(1 == a2.state<TracesDestruction>()->state_);
            // there was a temporary deleted while at swap that
            // corresponds to the second argument
            REQUIRE(2 == d1);
            REQUIRE(3 == d2);
        }
        CHECK(3 == d1);
        CHECK(4 == d2);
    }
}
