#include "demo/type_erasure_shared_pointer_value_manager.hpp"

#include "zoo/FunctionPolicy.h"

#include <catch2/catch.hpp>

namespace user {

template<typename V, typename Policy>
auto extractSharedPointer(zoo::AnyContainer<Policy> &a) {
    using VBuilder = typename Policy::template Builder<V>;
    auto downcasted = static_cast<VBuilder *>(a.container());
    return downcasted->sharedPointer();
}

}

using LocalBuffer = void *[4];
static_assert(sizeof(std::shared_ptr<int>) <= sizeof(LocalBuffer));

using UAny = zoo::AnyContainer<
    user::SharedPointerPolicy<
        LocalBuffer,
        zoo::Destroy, zoo::Move, zoo::Copy, zoo::RTTI
    >
>;

TEST_CASE("Shared Pointer Value Manager", "[demo][type-erasure][shared-pointer-policy]") {
    UAny uAny{9.9};
    CHECK(9.9 == *uAny.state<double>());
    user::ExplicitDestructor ed;
    REQUIRE(nullptr == user::ExplicitDestructor::last);
    uAny = ed;
    CHECK(nullptr == user::ExplicitDestructor::last);
    REQUIRE(typeid(user::ExplicitDestructor) == uAny.type());
    auto spp = user::extractSharedPointer<user::ExplicitDestructor>(uAny);
    auto sp = *spp;
    REQUIRE(2 == sp.use_count());
    CHECK(nullptr == user::ExplicitDestructor::last);
    const auto oldAddress = uAny.state<user::ExplicitDestructor>();
    REQUIRE(oldAddress == &*sp);
    sp.reset();
    REQUIRE(1 == spp->use_count());
    uAny = 5;
    REQUIRE(oldAddress == user::ExplicitDestructor::last);
}
