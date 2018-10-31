#include "GenericAnyTests.h"

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
