#include "GenericAnyTests.h"

#include <zoo/ConverterAny.h>

TEST_CASE("IAnyContainer") {
    using namespace zoo;
    using Container = IAnyContainer<sizeof(void *), alignof(void *)>;

    alignas(Container) char bytes[sizeof(Container)];
    new(bytes) Container;
    auto &container = *reinterpret_cast<Container *>(bytes);
    auto setTo0x33 = [](auto ptr, auto count) {
        for(auto i = count; i--; ) {
            ptr[i] = 0x33;
        }
    };
    auto check0x33 = [](auto ptr, auto count) {
        for(auto i = count; i--; ) {
            if(0x33 != ptr[i]) { return false; }
        }
        return true;
    };
    setTo0x33(container.m_space, sizeof(container.m_space));
    SECTION("Performance issue: constructor changes bytes") {
        new(bytes) Container;
        CHECK(check0x33(container.m_space, sizeof(container.m_space)));
    }
    Container c;
    SECTION("Copy does not initialize destination") {
        c.copy(&container);
        // the binary representation of a valid IAnyContainer can't be all bytes
        // set to 0x33
        CHECK(!check0x33(bytes, sizeof(bytes)));
    }
    static_assert(noexcept(c.move(&container)), "move is noexcept(true)");
    SECTION("Move does not initialize destination") {
        c.move(&container);
        CHECK(!check0x33(bytes, sizeof(bytes)));
    }
    static_assert(noexcept(c.value()), "value may throw");
    const auto containerPtr = &container;
    static_assert(noexcept(containerPtr->nonEmpty()), "nonEmpty may throw");
    SECTION("nonEmpty is true") {
        CHECK(!container.nonEmpty());
    }
    static_assert(noexcept(containerPtr->type()), "type may throw");
    SECTION("Type info is not void") {
        CHECK(typeid(void) == containerPtr->type());
    }
}

TEST_CASE("Resolved bugs") {
    using namespace zoo;
    SECTION("ReferentialContainer not a BaseContainer") {
        ReferentialContainer<1, 1, char> c{'a'};
        using Base = BaseContainer<1, 1>;
        CHECK(dynamic_cast<Base *>(&c));
        c.destroy();
    }
}

void debug() {};

void canonicalOnlyTests() {
    SECTION("Copy constructor - value held is not an \"Any\"") {
        zoo::Any a{5};
        zoo::Any b{a};
        REQUIRE(zoo::isRuntimeValue<int>(b));
    }
    SECTION("any_cast") {
        zoo::Any empty;
        REQUIRE_THROWS_AS(zoo::any_cast<int>(empty), std::bad_cast);
        REQUIRE_THROWS_AS(zoo::any_cast<int>(empty), zoo::bad_any_cast);
        REQUIRE(nullptr == zoo::any_cast<int>(&empty));
        const zoo::Any *constAny = nullptr;
        REQUIRE(nullptr == zoo::any_cast<int>(constAny));
        constAny = &empty;
        empty = 7;
        REQUIRE(nullptr != zoo::any_cast<int>(constAny));
    }
    SECTION("Multiple argument constructor -- value") {
        zoo::Any mac{TwoArgumentConstructor{nullptr, 3}};
        REQUIRE(zoo::isRuntimeValue<TwoArgumentConstructor>(mac));
        auto ptr = zoo::any_cast<TwoArgumentConstructor>(&mac);
        REQUIRE(false == ptr->boolean);
        REQUIRE(3 == ptr->value);
    }
}

TEST_CASE("Any", "[contract][canonical]") {
    testAnyImplementation<zoo::Any>();
    canonicalOnlyTests();
}

TEST_CASE("AnyExtensions", "[contract]") {
    testAnyImplementation<
        zoo::AnyContainer<zoo::ConverterPolicy<8, 8>>
    >();
}

namespace pseudo_std {

// swap2 simulates std::swap
template<typename T>
void swap2(T &t1, T &t2) { std::swap(t1, t2); }

template<typename T>
struct Container {
    T t1_, t2_;
    Container() { swap2(t1_, t2_); }
    Container(T t1, T t2): t1_(t1), t2_(t2) { swap2(t1_, t2_); }
};

}

namespace zoo {

/* this would introduce an ambiguity:
template<typename T>
inline
void swap2(T &t1, T &t2) { std::swap(t1, t2); }
*/

using pseudo_std::swap2;

inline void swap2(Any &a1, Any &a2) { swap(a1, a2); }

struct Derived: Any {};

struct A { int value_; };

}

TEST_CASE("Example of swap ambiguity") {
    pseudo_std::Container<zoo::Derived> usedToGiveError;
    zoo::A a5{5}, a6{6};
    pseudo_std::Container<zoo::A> swaps(a5, a6);
    CHECK(swaps.t1_.value_ == 6);
    CHECK(swaps.t2_.value_ == 5);
    swap2(a5, a6);
    CHECK(6 == a5.value_);
}
