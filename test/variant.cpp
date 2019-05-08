#include "zoo/variant.h"
#include <catch2/catch.hpp>

struct HasDestructor {
    int *ip_;
    HasDestructor(int *ip): ip_{ip} { *ip_ = 1; }
    ~HasDestructor() { *ip_ = 0; }
};

struct IntReturns1 {
    template<typename T>
    int operator()(T) { return 0; }
    int operator()(int) { return 1; }
};

struct MoveThrows {
    MoveThrows() = default;
    MoveThrows(const MoveThrows &) = default;
    MoveThrows(MoveThrows &&) noexcept(false);
};

TEST_CASE("Variant", "[variant]") {
    int value = 4;
    using V2 = zoo::Variant<int, char>;
    static_assert(std::is_nothrow_move_constructible_v<V2>, "");
    using V3 = zoo::Variant<int, MoveThrows, char>;
    static_assert(!std::is_nothrow_move_constructible_v<V3>, "");
    static_assert(
        noexcept(std::swap(std::declval<V2 &>(), std::declval<V2 &>())), ""
    );
    static_assert(
        !noexcept(std::swap(std::declval<V3 &>(), std::declval<V3 &>())), ""
    );
    using V = zoo::Variant<int, HasDestructor>;
    SECTION("Proper construction") {
        V var{std::in_place_index_t<0>{}, 77};
        REQUIRE(77 == *var.as<int>());
    }
    SECTION("Destructor Called") {
        {
            V var{std::in_place_index_t<1>{}, &value};
            REQUIRE(1 == value);
        }
        REQUIRE(0 == value);
    }
    SECTION("visit") {
        V instance;
        auto result = zoo::visit<int>(IntReturns1{}, instance);
        REQUIRE(0 == result);
        instance = V{std::in_place_index_t<1>{}, &value};
        result = zoo::visit<int>(IntReturns1{}, instance);
        REQUIRE(0 == result);
        value = 5;
        instance = V{std::in_place_index_t<0>{}, 77};
        SECTION("Internally held object is destroyed on move assignment") {
            REQUIRE(0 == value);
        }
        result = zoo::visit<int>(IntReturns1{}, instance);
        REQUIRE(1 == result);
    }
    SECTION("GCC Visit") {
        V instance;
        REQUIRE(0 == zoo::GCC_visit<int>(IntReturns1{}, instance));
        instance = V{std::in_place_index_t<0>{}, 99};
        REQUIRE(1 == zoo::GCC_visit<int>(IntReturns1{}, instance));
    }
}
