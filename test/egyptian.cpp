#include "Operations.h"

struct ModuloN {
    static int N;
    int _value;

    ModuloN() {}
    ModuloN(int v): _value{v} {}

    ModuloN operator+(ModuloN other) const { return (_value + other._value) % N; }
    ModuloN operator*(ModuloN other) const { return (_value * other._value) % N; }
    ModuloN operator/(ModuloN other) const { return (_value / other._value) % N; }
    int operator%(ModuloN other) const { return _value % other._value; }

    int value() const noexcept { return _value; }
};

int ModuloN::N = 0;

bool operator!=(ModuloN x, ModuloN y) {
    return x._value != y._value;
}

template<typename E>
E egyptianExponentiation(E x, E y) {
    return zoo::egyptian(x, y, zoo::Exponentiation{});
}

#include "Operations_impl.h"

#include <catch2/catch.hpp>

TEST_CASE("Dummy", "[egyptian]") {
    SECTION("Normal integer exponentiation") {
        CHECK(243 == egyptianExponentiation(3, 5));
    }
    SECTION("Exponentiation modulo 7") {
        ModuloN::N = 7;
        auto v = egyptianExponentiation(ModuloN{3}, ModuloN{5});
        CHECK(243 % 7 == v.value());
    }
    SECTION("Fermat's little theorem") {
        ModuloN::N = 23;
        auto modulo9to22 = egyptianExponentiation(ModuloN{9}, ModuloN{22});
        SECTION("Complete cycle") {
            CHECK(1 == modulo9to22.value());
        }
        SECTION("Normal integer overflows") {
            auto nineTo22 = egyptianExponentiation(9, 22) % 23;
            CHECK(nineTo22 != modulo9to22.value());
        }
    }
}

