#include "demo/type_erasure_shared_pointer_value_manager.hpp"

#include "zoo/Any/VTablePolicy.h"
#include "zoo/AnyContainer.h"
#include "zoo/FunctionPolicy.h"

#include <catch2/catch.hpp>
#include <sstream>
#include <type_traits>


// Primary template: defaults to false
template <typename, typename = std::void_t<>>
struct ExclusiveAwareTrait: std::false_type {};

// Specialization: if T has T::ExclusiveAware, this will match and be true
template <typename T>
struct ExclusiveAwareTrait<T, std::void_t<typename T::ExclusiveAware>>: std::true_type {};


struct MyCoolStruct {
    struct ExclusiveAware {};
};

static_assert(ExclusiveAwareTrait<int>::value == false);
static_assert(ExclusiveAwareTrait<MyCoolStruct>::value);


struct StringInputOutput {
    struct VTableEntry {
        std::string (*str)(const void *);
        void (*fromString)(void *, const std::string &);
    };

    template<typename>
    constexpr static inline VTableEntry Default = {
        [](const void *) { return std::string(); },
        [](void *, const std::string &) {}
    };

    template<typename ConcreteValueManager>
    constexpr static inline VTableEntry Operation = {
        [](const void *container) {
            auto c = const_cast<void *>(container);
            auto cvm = static_cast<ConcreteValueManager *>(c);
            std::ostringstream oss;
            oss << *cvm->value();
            return oss.str();
        },
        [] (void *container, const std::string &str) {
            auto c = const_cast<void *>(container);
            auto cvm = static_cast<ConcreteValueManager *>(c);

            if constexpr (ExclusiveAwareTrait<ConcreteValueManager>::value) {
                if (!cvm->isExclusive()) {
                    cvm->makeExclusive();
                }
            }

            std::istringstream iss{str};
            iss >> *cvm->value();
        },
    };

    // No extra state/functions needed in the memory layout
    template<typename Container>
    struct Mixin {};

    template<typename AnyC>
    struct UserAffordance {

        std::string stringize() const {
            auto container =
                const_cast<AnyC *>(static_cast<const AnyC *>(this))->container();
            return container->template vTable<StringInputOutput>()->str(container);
        }

        auto fromString(const std::string &str) {
            auto container = static_cast<AnyC *>(this)->container();
            auto vt = container->template vTable<StringInputOutput>();
            vt->fromString(container, str);
        }
    };
};


namespace user {

template<typename V, typename Policy>
auto extractSharedPointer(zoo::AnyContainer<Policy> &a) {
    using VBuilder = typename Policy::template Builder<V>;
    auto valueManager = static_cast<VBuilder *>(a.container());
    return valueManager->sharedPointer();
}

template<typename V, typename Policy>
auto isExclusive(zoo::AnyContainer<Policy> &a) {
    if constexpr (SharedPointerOptIn<V>::value) {
        using VBuilder = typename Policy::template Builder<V>;
        auto valueManager = static_cast<VBuilder *>(a.container());
        return valueManager->isExclusive();
    }

    return true;
}



}

using LocalBuffer = void *[4];
static_assert(sizeof(std::shared_ptr<int>) <= sizeof(LocalBuffer));

using UAny = zoo::AnyContainer<
    user::SharedPointerPolicy<
        LocalBuffer,
        zoo::Destroy, zoo::Move, zoo::RTTI
    >
>;


TEST_CASE("Shared Pointer Value Manager", "[demo][type-erasure][shared-pointer-policy]") {
    SECTION("shared pointer value management basics") {
        UAny uAny{9.9};
        CHECK(9.9 == *uAny.state<double>());

        REQUIRE(user::isExclusive<double>(uAny));

        user::ExplicitDestructor ed;
        REQUIRE(nullptr == user::ExplicitDestructor::last);
        uAny = ed;
        CHECK(nullptr == user::ExplicitDestructor::last);
        REQUIRE(typeid(user::ExplicitDestructor) == uAny.type());
        auto spp = user::extractSharedPointer<user::ExplicitDestructor>(uAny);
        auto sp = *spp;

        REQUIRE(2 == sp.use_count());

        REQUIRE(! user::isExclusive<user::ExplicitDestructor>(uAny));

        CHECK(nullptr == user::ExplicitDestructor::last);
        const auto oldAddress = uAny.state<user::ExplicitDestructor>();
        REQUIRE(oldAddress == &*sp);
        sp.reset();
        REQUIRE(user::isExclusive<user::ExplicitDestructor>(uAny));

        uAny = 5;
        REQUIRE(oldAddress == user::ExplicitDestructor::last);
    }

    SECTION("roundtrip io affordance") {
       using IOType = zoo::AnyContainer<zoo::Policy<LocalBuffer, zoo::Destroy, zoo::Move, StringInputOutput>>;
       IOType io = 8;
       REQUIRE("8" == io.stringize());
       io.fromString("42");
       REQUIRE(42 == *io.state<int>());
    }

    SECTION("copy on write") {
        using CopyOnWritable = zoo::AnyContainer<user::SharedPointerPolicy<LocalBuffer, zoo::Destroy, zoo::Move, zoo::Copy, StringInputOutput>>;
        CopyOnWritable a = std::string{"foo"};
        auto aState =  a.state<std::string>();
        REQUIRE("foo" == *aState);

        REQUIRE(user::isExclusive<std::string>(a));


        auto b = a;
        REQUIRE(!user::isExclusive<std::string>(a));
        // proves that a and b share the same object
        REQUIRE(aState == b.state<std::string>());
        // now we're going to write to b! then the copy must happen.
        b.fromString("bar");

        REQUIRE(user::isExclusive<std::string>(a));
        REQUIRE(user::isExclusive<std::string>(b));

        auto bState = b.state<std::string>();
        REQUIRE(aState != bState);
        REQUIRE("foo" == *aState);
        REQUIRE("bar" == *bState);
    }
}
