//
//  AlignedStorage.cpp
//
//  Created by Eduardo Madrid on 7/2/19.
//

#include "zoo/AlignedStorage.h"

#include "catch2/catch.hpp"

#include <type_traits>

using namespace zoo;

#pragma GCC diagnostic ignored "-Wgnu-alignof-expression"

static_assert(VPSize == sizeof(AlignedStorage<>::space_), "incorrect size");
static_assert(alignof(AlignedStorage<>::space_) == VPAlignment, "misaligned");

using Big = AlignedStorage<2*VPSize>;
static_assert(2*VPSize <= sizeof(Big), "larger size not honored");
using A1 = AlignedStorage<1, 1>;
static_assert(1 == sizeof(A1::space_), "specific size setting not honored");
static_assert(alignof(A1::space_) == 1, "specific alignment not honored");

template<typename Class, typename T>
std::false_type MayCallBuild(...);
template<typename Class, typename T, typename... As>
auto MayCallBuild(int, As &&...as) ->
    decltype(
        std::declval<Class &>().template build<T>(std::forward<As>(as)...),
        std::true_type{}
    );
template<typename Class, typename T, typename... As>
constexpr auto MayCallBuild_(As &&...as) {
    return decltype(MayCallBuild<Class, T>(0, std::forward<As>(as)...))::value;
}

static_assert(
    MayCallBuild_<AlignedStorage<>, void *>(nullptr),
    "the default AlignedStorage must be able to build a void *"
);

static_assert(
    !MayCallBuild_<AlignedStorage<1>, void *>(nullptr),
    "a small AlignedStorage must reject building too big a type"
);

static_assert(
    !MayCallBuild_<AlignedStorage<VPSize, 1>, void *>(),
    "must reject building a type with superior alignment"
);

static_assert(
    MayCallBuild_<Big, char *>(nullptr),
    "must accept building a type with a divisor alignment"
);

struct Constructors {
    Constructors(double) noexcept(false);
    constexpr explicit Constructors(const void *) noexcept {};
    Constructors(int, Constructors &&);
};

static_assert(
    MayCallBuild_<AlignedStorage<>, Constructors>(5.5),
    "must accept building an acceptable type"
);

static_assert(
    MayCallBuild_<AlignedStorage<>, Constructors>(5),
    "must allow the conversions the held object supports"
);

static_assert(
    !MayCallBuild_<AlignedStorage<>, Constructors>(4.5, nullptr),
    "must reject building if there are no suitable constructors"
);

static_assert(
    MayCallBuild_<AlignedStorage<>, Constructors>(4, Constructors{nullptr}),
    "must preserve the r-value category of the arguments"
);

static_assert(
    !MayCallBuild_<
        AlignedStorage<>, Constructors,
        // makes explicit the argument list to get an l-value reference
        int, const Constructors &
    >(4, Constructors{nullptr}),
    "must preserve the l-value category of the arguments"
);

using A = AlignedStorage<>;

static_assert(
    std::is_nothrow_constructible_v<Constructors, void *&&>
);

static_assert(
    noexcept(std::declval<A &>().build<Constructors, void *&&>(nullptr)),
    "Failed to preserve noexceptness of constructor"
);

static_assert(
    !noexcept(std::declval<A &>().build<Constructors>(4.4)),
    "Failed to preserve may-throwness of constructor"
);

struct Fields {
    enum Trace: char { ZERO, ONE, TWO };
    Trace a, b, c;

    Fields(): a{ZERO}, b{ONE}, c{TWO} {}
    Fields(const Fields &) = default;
    Fields(Fields &&): a{TWO}, b{TWO}, c{TWO} {}
    Fields(int): a{ONE}, b{ONE}, c{ONE} {}


    bool operator==(Fields f) const {
        return a == f.a && b == f.b && c == f.c;
    }
};

struct TraceDestructor {
    int &trace_;
    TraceDestructor(int &trace): trace_{trace} {}
    ~TraceDestructor() { trace_ = 1; }
};

TEST_CASE("Aligned Storage", "[aligned-storage][contract][api]") {
    SECTION("build activates the constructor") {
        A any;
        any.build<Fields>();
        Fields defaulted;
        CHECK(defaulted == *any.as<Fields>());
    }
    SECTION("destroy activates the destructor") {
        int trace = 9;
        A any;
        any.build<TraceDestructor>(trace);
        CHECK(9 == any.as<TraceDestructor>()->trace_);
        any.destroy<TraceDestructor>();
        CHECK(1 == trace);
    }
}
