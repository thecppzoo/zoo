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

struct Moves {
    enum Kind {
        DEFAULT,
        COPIED,
        MOVING,
        MOVED
    } kind;

    Moves(): kind{DEFAULT} {}

    Moves(const Moves &): kind{COPIED} {}

    Moves(Moves &&moveable): kind{MOVING} {
        moveable.kind = MOVED;
    }
};

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
    SECTION("Copy constructor - value held is not an \"Any\"") {
        zoo::internals::AnyExtensions a{5};
        zoo::Any b{a};
        REQUIRE(zoo::internals::AnyExtensions::isAValue<int>(b));
    }
    SECTION("Move constructor -- Value") {
        zoo::Any movingFrom{Moves{}};
        REQUIRE(zoo::internals::AnyExtensions::isAValue<Moves>(movingFrom));
        zoo::Any movedTo{std::move(movingFrom)};
        auto ptrFrom = zoo::any_cast<Moves>(&movingFrom);
        auto ptrTo = zoo::any_cast<Moves>(&movedTo);
        REQUIRE(Moves::MOVED == ptrFrom->kind);
        REQUIRE(Moves::MOVING == ptrTo->kind);
    }
    SECTION("Move constructor -- Referential") {
        zoo::Any movingFrom{Big{}};
        REQUIRE(zoo::internals::AnyExtensions::isReferential<Big>(movingFrom));
        auto original = zoo::any_cast<Big>(&movingFrom);
        zoo::Any movingTo{std::move(movingFrom)};
        auto afterMove = zoo::any_cast<Big>(&movingTo);
        REQUIRE(original == afterMove);
        REQUIRE(nullptr == zoo::any_cast<Big>(&movingFrom));
    }
    SECTION("Initializer constructor -- copying") {
        Moves value;
        zoo::Any copied{value};
        auto ptr = zoo::any_cast<Moves>(&copied);
        REQUIRE(Moves::COPIED == ptr->kind);
    }
    SECTION("Initializer constructor -- moving") {
        Moves def;
        CHECK(Moves::DEFAULT == def.kind);
        zoo::Any moving{std::move(def)};
        REQUIRE(Moves::MOVED == def.kind);
        REQUIRE(Moves::MOVING == zoo::any_cast<Moves>(&moving)->kind);
    }
}
