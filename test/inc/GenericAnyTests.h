#pragma once

#include "util/ExtendedAny.h"

namespace zoo {

template<typename T, int Size, int Alignment>
bool isRuntimeValue(IAnyContainer<Size, Alignment> *ptr) {
    return dynamic_cast<ValueContainer<Size, Alignment, T> *>(ptr);
}

template<typename T, int Size, int Alignment>
bool isRuntimeReference(IAnyContainer<Size, Alignment> *ptr) {
    return dynamic_cast<ReferentialContainer<Size, Alignment, T> *>(ptr);
}

template<typename T, int Size, int Alignment>
bool isRuntimeValue(ConverterContainer<Size, Alignment> *ptr) {
    return dynamic_cast<ConverterValue<T> *>(ptr->driver());
}

template<typename T, int Size, int Alignment>
bool isRuntimeReference(ConverterContainer<Size, Alignment> *ptr) {
    return dynamic_cast<ConverterReferential<T> *>(ptr->driver());
}

template<typename Policy>
struct RVD {
    template<typename T>
    static bool runtimeReference(typename Policy::MemoryLayout *ptr) {
        return isRuntimeReference<T>(ptr);
    }

    template<typename T>
    static bool runtimeValue(typename Policy::MemoryLayout *ptr) {
        return isRuntimeValue<T>(ptr);
    }
};

template<typename T, typename Policy>
bool isRuntimeValue(AnyContainer<Policy> &a) {
    return RVD<Policy>::template runtimeValue<T>(a.container());
}

template<typename T, typename Policy>
bool isRuntimeReference(AnyContainer<Policy> &a) {
    return RVD<Policy>::template runtimeReference<T>(a.container());
}

}

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

    Moves(Moves &&moveable) noexcept: kind{MOVING} {
        moveable.kind = MOVED;
    }
};

struct BuildsFromInt {
    BuildsFromInt(int) {};
};

struct TakesInitializerList {
    int s;
    double v;
    TakesInitializerList(std::initializer_list<int> il, double val):
        s(il.size()), v(val)
    {}
};

struct TwoArgumentConstructor {
    bool boolean;
    int value;

    TwoArgumentConstructor(void *p, int q): boolean(p), value(q) {};
};

template<typename>
bool movedFromTest(zoo::Any &a) {
    auto ac = a.container();
        // strange warning
        // warning: expression with side effects
        // will be evaluated despite being used as an operand to 'typeid'
        // [-Wpotentially-evaluated-expression]
    return typeid(*ac) == typeid(zoo::Any::Container);
}

template<typename W, int S, int A>
bool movedFromTest(zoo::AnyContainer<zoo::ConverterPolicy<S, A>> &a) {
        return nullptr == zoo::anyContainerCast<W>(&a);
}

template<typename ExtAny>
void testAnyImplementation() {
    SECTION("Value Destruction") {
        int value;
        {
            ExtAny a{Destructor{&value}};
            REQUIRE(zoo::isRuntimeValue<Destructor>(a));
            value = 0;
        }
        REQUIRE(1 == value);
    }
    SECTION("Referential Semantics - Alignment, Destruction") {
        int value;
        {
            ExtAny a{D2{&value}};
            REQUIRE(zoo::isRuntimeReference<D2>(a));
            value = 0;
        }
        REQUIRE(1 == value);
    }
    SECTION("Referential Semantics - Size") {
        ExtAny v{Big{}};
        REQUIRE(zoo::isRuntimeReference<Big>(v));
        REQUIRE(v.has_value());
    }
    SECTION("Move constructor -- Value") {
        ExtAny movingFrom{Moves{}};
        REQUIRE(zoo::isRuntimeValue<Moves>(movingFrom));
        ExtAny movedTo{std::move(movingFrom)};
        auto ptrFrom = zoo::anyContainerCast<Moves>(&movingFrom);
        auto ptrTo = zoo::anyContainerCast<Moves>(&movedTo);
        REQUIRE(Moves::MOVED == ptrFrom->kind);
        REQUIRE(Moves::MOVING == ptrTo->kind);
    }
    SECTION("Move constructor -- Referential") {
        ExtAny movingFrom{Big{}};
        REQUIRE(zoo::isRuntimeReference<Big>(movingFrom));
        auto original = zoo::anyContainerCast<Big>(&movingFrom);
        ExtAny movingTo{std::move(movingFrom)};
        auto afterMove = zoo::anyContainerCast<Big>(&movingTo);
        CHECK(!movingFrom.has_value());
        REQUIRE(original == afterMove);
        REQUIRE(movedFromTest<Moves>(movingFrom));
    }
    SECTION("Initializer constructor -- copying") {
        Moves value;
        ExtAny copied{value};
        auto ptr = zoo::anyContainerCast<Moves>(&copied);
        REQUIRE(Moves::COPIED == ptr->kind);
    }
    SECTION("Initializer constructor -- moving") {
        Moves def;
        CHECK(Moves::DEFAULT == def.kind);
        ExtAny moving{std::move(def)};
        REQUIRE(Moves::MOVED == def.kind);
        REQUIRE(Moves::MOVING == zoo::anyContainerCast<Moves>(&moving)->kind);
    }
    SECTION("Assignments") {
        using namespace zoo;
        ExtAny integer{5};
        int willChange = 0;
        ExtAny willBeTrampled{Destructor{&willChange}};
        willBeTrampled = integer;
        auto asInt = anyContainerCast<int>(&willBeTrampled);
        REQUIRE(nullptr != asInt);
        REQUIRE(5 == *asInt);
        REQUIRE(1 == willChange);
        willChange = 0;
        ExtAny anotherTrampled{D2{&willChange}};
        *asInt = 9;
        anotherTrampled = willBeTrampled;
        asInt = anyContainerCast<int>(&anotherTrampled);
        REQUIRE(nullptr != asInt);
        REQUIRE(9 == *asInt);
        REQUIRE(1 == willChange);
        integer = Moves{};
        auto movPtr = anyContainerCast<Moves>(&integer);
        REQUIRE(nullptr != movPtr);
        REQUIRE(Moves::MOVING == movPtr->kind);
        willBeTrampled = *movPtr;
        auto movPtr2 = anyContainerCast<Moves>(&willBeTrampled);
        REQUIRE(nullptr != movPtr2);
        REQUIRE(Moves::COPIED == movPtr2->kind);
        anotherTrampled = std::move(*movPtr2);
        REQUIRE(Moves::MOVED == movPtr2->kind);
        auto p = anyContainerCast<Moves>(&anotherTrampled);
        REQUIRE(Moves::MOVING == p->kind);
    }
    ExtAny empty;
    SECTION("reset()") {
        REQUIRE(!empty.has_value());
        empty = 5;
        REQUIRE(empty.has_value());
        empty.reset();
        REQUIRE(!empty.has_value());
    }
    SECTION("typeid") {
        REQUIRE(typeid(void) == empty.type());
        empty = Big{};
        REQUIRE(typeid(Big) == empty.type());
    }
    SECTION("swap") {
        ExtAny other{5};
        anyContainerSwap(empty, other);
        REQUIRE(typeid(int) == empty.type());
        REQUIRE(typeid(void) == other.type());
        auto valuePointerAtEmpty = zoo::anyContainerCast<int>(&empty);
        REQUIRE(5 == *valuePointerAtEmpty);
    }
    SECTION("inplace") {
        ExtAny bfi{std::in_place_type<BuildsFromInt>, 5};
        REQUIRE(typeid(BuildsFromInt) == bfi.type());
        ExtAny il{std::in_place_type<TakesInitializerList>, { 9, 8, 7 }, 2.2};
        REQUIRE(typeid(TakesInitializerList) == il.type());
        auto ptr = zoo::anyContainerCast<TakesInitializerList>(&il);
        REQUIRE(3 == ptr->s);
        REQUIRE(2.2 == ptr->v);
    }
    SECTION("Multiple argument constructor -- value") {
        ExtAny mac{TwoArgumentConstructor{nullptr, 3}};
        REQUIRE(zoo::isRuntimeValue<TwoArgumentConstructor>(mac));
        auto ptr = zoo::anyContainerCast<TwoArgumentConstructor>(&mac);
        REQUIRE(false == ptr->boolean);
        REQUIRE(3 == ptr->value);
    }
}
