#include <catch2/catch.hpp>

#define STRING7_TESTS


#include "TightPolicy.h"

#include <GenericAnyTests.h>

String7::ConstructorOverload lastOverload;
void String7::c(ConstructorOverload o) { lastOverload = o; }

static_assert(!is_stringy_type<int>::value, "");
static_assert(is_stringy_type<std::string>::value, "");


#include <memory>

namespace zoo {

using BeforeMovingType = const char *;

BeforeMovingType beforeMoving(const std::string &s) {
    return s.data();
}

auto bufferWasMoved(const std::string &to, BeforeMovingType b) {
    return to.data() == b;
}

}


TEST_CASE("Encodings", "[TightPolicy]") {
    Tight t;
    auto &e = t.code.empty;
    SECTION("Empty") {
        bool emptiness = !e.isInteger && !e.isPointer && e.isNotString;
        REQUIRE(emptiness);
    }
    SECTION("Int63 - does not preserve negatives") {
        t.code.integer = -1;
        REQUIRE(e.isInteger);
        REQUIRE(-1 == long{t.code.integer});
    }
    SECTION("Int63 - not exactly 63 bits of precision") {
        long maxInt = static_cast<unsigned long>(long{-1}) >> 1;
        SECTION("Overflows at 63 bits") {
            REQUIRE(0 < maxInt);
            t.code.integer = maxInt;
            REQUIRE(-1 == t.code.integer);
        }
        SECTION("Drops the most significant bit") {
            long minInt = static_cast<unsigned long>(maxInt) + 1;
            REQUIRE(minInt < 0);
            REQUIRE(-1 == maxInt + minInt);
            t.code.integer = minInt;
            REQUIRE(0 == t.code.integer);
        }
    }
    SECTION("Pointer62") {
        std::unique_ptr<int> forget{new int{8}};
        auto ptr = forget.get();
        t.code.pointer = ptr;
        auto pointerness = t.isPointer();
        REQUIRE(pointerness);
        REQUIRE(ptr == t.code.pointer);
    }
    SECTION("String7") {
        char noNull[] = { 'H', 'e', 'l', 'l', 'o', '!' };
        static_assert(std::is_same<char[6], decltype(noNull)>::value, "");
        t.code.string = noNull;
        SECTION("Correct encoding") {
            bool stringness = t.isString();
            REQUIRE(stringness);
            CHECK(String7::ARRAY == lastOverload);
        }
        SECTION("Correct count from char[]") {
            REQUIRE(6 == t.code.string.count());
        }
        std::string asString = "hi!";
        SECTION("Conversion to std::string") {
            std::string helloStr{noNull, noNull + sizeof(noNull)};
            asString = t.code.string;
            REQUIRE(helloStr == asString);
        }
    }
}

template<typename T>
struct DBuilder: Builder<T> {
    using Builder<T>::Builder;
 
    ~DBuilder() { this->destroy(); }
};

TEST_CASE("Builders", "[TightPolicy]") {
    SECTION("Fallback") {
        DBuilder<double> b{3.1415265};
        REQUIRE(b.isPointer());
    }
    SECTION("Stringies") {
        static_assert(is_stringy_type<char[343]>::value, "");
        static_assert(is_stringy_type<const char[3]>::value, "");
        static_assert(is_stringy_type<const std::string>::value, "");
        static_assert(is_stringy_type<const char (&)[2]>::value, "");
        static_assert(!is_stringy_type<char>::value, "");

        char hello[] = "Hello!";
        SECTION("Small string") {
            std::string ss{hello};
            DBuilder<std::string> built{ss};
            REQUIRE(built.isString());
            CHECK(String7::POINTER_COUNT == lastOverload);
        }
        SECTION("Large string") {
            std::string ls{"This is a large string"};
            DBuilder<std::string> built{ls};
            REQUIRE(built.isPointer());
        }
        SECTION("Temporary small string") {
            DBuilder<std::string> b{std::string{"Hi!"}};
            REQUIRE(b.isString());
            CHECK(String7::POINTER_COUNT == lastOverload);
        }
        SECTION("Temporary large string") {
            std::string s{"This is large enough to create buffer"};
            auto characterBufferToMove = zoo::beforeMoving(s);
            DBuilder<std::string> b{std::move(s)};
            REQUIRE(b.isPointer());
            auto f = fallback(b.code.pointer);
            REQUIRE(typeid(std::string) == f->type());
            auto *ptr = zoo::anyContainerCast<std::string>(f);
            #ifdef ENABLE_STRING_MOVE_CHECK
            CHECK(zoo::bufferWasMoved(*ptr, characterBufferToMove));
            #endif
        }
        SECTION("Small array") {
            DBuilder<char[5]> small{hello};
            REQUIRE(small.isString());
        }
        SECTION("Large array") {
            char large[] = "This is a large buffer";
            DBuilder<char[5]> allFine{large};
            REQUIRE(allFine.isPointer());
            REQUIRE(
                typeid(std::string) == fallback(allFine.code.pointer)->type()
            );
        }
    }
    SECTION("Integral") {
        Builder<int> bint{5};
        REQUIRE(bint.code.empty.isInteger);
        CHECK(5 == bint.code.integer);
    }
}

namespace zoo {

template<>
struct RuntimeReferenceOrValueDiscriminator<TightPolicy> {
    template<typename>
    static bool runtimeValue(Tight *e) {
        return e->isInteger() || e->isString();
    }

    template<typename>
    static bool runtimeReference(Tight *e) {
        return e->isPointer();
    }
};

}

TEST_CASE("TightAny", "[TightAny][contract]") {
    using ExtAny = TightAny;
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
        CHECK(zoo::isRuntimeReference<Big>(v));
        CHECK(v.has_value());
    }
    SECTION("Move constructor -- Referential") {
        ExtAny movingFrom{Big{}};
        REQUIRE(zoo::isRuntimeReference<Big>(movingFrom));
        auto original = zoo::anyContainerCast<Big>(&movingFrom);
        ExtAny movingTo{std::move(movingFrom)};
        auto afterMove = zoo::anyContainerCast<Big>(&movingTo);
        CHECK(!movingFrom.has_value());
        CHECK(original == afterMove);
    }
    SECTION("Initializer constructor -- copying") {
        Moves value;
        ExtAny copied{value};
        auto &contained = copied.type();
        CHECK(contained.name() == typeid(Moves).name());
        REQUIRE(typeid(Moves) == copied.type());
        auto ptr = zoo::anyContainerCast<Moves>(&copied);
        CHECK(Moves::COPIED == ptr->kind);
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
        int willChange = 0;
        ExtAny willBeTrampled{Destructor{&willChange}};
        ExtAny integer{5};
        SECTION("Destroys trampled object") {
            willBeTrampled = integer;
            REQUIRE(typeid(long) == willBeTrampled.type());
            auto asInt = tightCast<int>(willBeTrampled);
            CHECK(5 == asInt);
            CHECK(1 == willChange);
        }
        SECTION("Move and copy assignments") {
            integer = Moves{};
            auto movPtr = anyContainerCast<Moves>(&integer);
            CHECK(Moves::MOVING == movPtr->kind);
            willBeTrampled = *movPtr;
            auto movPtr2 = anyContainerCast<Moves>(&willBeTrampled);
            REQUIRE(nullptr != movPtr2);
            CHECK(Moves::COPIED == movPtr2->kind);
            ExtAny anotherTrampled{D2{&willChange}};
            anotherTrampled = std::move(*movPtr2);
            CHECK(Moves::MOVED == movPtr2->kind);
            auto p = anyContainerCast<Moves>(&anotherTrampled);
            CHECK(Moves::MOVING == p->kind);
        }
    }
    SECTION("Stringy TightAny") {
        SECTION("Short String") {
            std::string hello{"Hello"};
            ExtAny aString{std::in_place_type<std::string>, hello};
            REQUIRE(zoo::isRuntimeValue<void>(aString));
            REQUIRE(typeid(String7) == aString.type());
            auto val = tightCast<std::string>(aString);
            CHECK(val == hello);
        }
        SECTION("Gutenberg") {
            char gutenberg[] = "Gutenberg";
            ExtAny inplaced{
                std::in_place_type<std::string>, gutenberg, sizeof(gutenberg) - 1
            };
            REQUIRE(zoo::isRuntimeReference<void>(inplaced));
            REQUIRE(typeid(std::string) == inplaced.type());
            auto val = tightCast<std::string>(inplaced);
            CHECK(val == gutenberg);
        }
        SECTION("Large string") {
            std::string large{"Requires TightAny fallback"};
            ExtAny str{large};
            REQUIRE(zoo::isRuntimeReference<void>(str));
            REQUIRE(typeid(std::string) == str.type());
            auto val = tightCast<std::string>(str);
            CHECK(val == large);
        }
        SECTION("Constructors") {
            std::string gutenberg("Gutenberg");
            ExtAny initial{gutenberg};
            SECTION("Copies TightAny of string") {
                ExtAny copiedTo{initial};
                REQUIRE(zoo::isRuntimeReference<void>(copiedTo));
                REQUIRE(typeid(std::string) == copiedTo.type());
                auto val = tightCast<std::string>(copiedTo);
                CHECK(val == gutenberg);
            }
            SECTION("Moves TightAny of string") {
                std::string gutenberg("Gutenberg");
                ExtAny initial{gutenberg};
                ExtAny movedTo{std::move(initial)};
                REQUIRE(zoo::isRuntimeReference<void>(movedTo));
                REQUIRE(typeid(std::string) == movedTo.type());
                auto val = tightCast<std::string>(movedTo);
                CHECK(val == gutenberg);
            }
        }
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
        swap(empty, other);
        REQUIRE(typeid(long) == empty.type());
        REQUIRE(typeid(void) == other.type());
        auto valuePointerAtEmpty = tightCast<int>(empty);
        REQUIRE(5 == valuePointerAtEmpty);
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
    SECTION("Multiple argument constructor -- Referential") {
        ExtAny mac{TwoArgumentConstructor{nullptr, 3}};
        REQUIRE(zoo::isRuntimeReference<TwoArgumentConstructor>(mac));
        auto ptr = zoo::anyContainerCast<TwoArgumentConstructor>(&mac);
        REQUIRE(false == ptr->boolean);
        REQUIRE(3 == ptr->value);
    }
}

