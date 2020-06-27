//
//  AlignedStorage.cpp
//
//  Created by Eduardo Madrid on 7/2/19.
//

#include "zoo/AlignedStorage.h"

#include "catch2/catch.hpp"

#include <type_traits>

using namespace zoo;

static_assert(VPSize == sizeof(AlignedStorage<>::space_), "incorrect size");
#ifdef __GNUC__
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wgnu-alignof-expression"
#endif
static_assert(alignof(AlignedStorage<>::space_) == VPAlignment, "misaligned");
#endif

using Big = AlignedStorage<2*VPSize>;
static_assert(2*VPSize <= sizeof(Big), "larger size not honored");
using A1 = AlignedStorage<1, 1>;
static_assert(1 == sizeof(A1::space_), "specific size setting not honored");

#ifdef __GNUC__
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wgnu-alignof-expression"
#endif
static_assert(alignof(A1::space_) == 1, "specific alignment not honored");
#endif

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
    impl::Constructible_v<void *, std::nullptr_t>
);

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
    noexcept(std::declval<A &>().build<Constructors>(nullptr)),
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

namespace { // Array support

struct Typical {
    Typical() = default;
    Typical(const Typical &) = default;
    Typical(Typical &&) = default;
    long state_;
};

using namespace std;

using ForTypical8 = zoo::AlignedStorage<sizeof(Typical) * 8>;

Typical arr8[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
Typical arr42[4][2] = {
    { 11, 12 },
    { 21, 22 },
    { 31, 32 },
    { 41, 42 }
};

static_assert(MayCallBuild_<ForTypical8, Typical[8]>(arr8));
static_assert(MayCallBuild_<ForTypical8, Typical[8]>(&arr8[0]));
static_assert(MayCallBuild_<ForTypical8, Typical[4][2]>(arr42));

struct Traces {
    enum Type {
        DEFAULT, COPIED, MOVED, MOVED_FROM
    };
    Type state_;
    Traces(): state_{DEFAULT} {}
    Traces(const Traces &): state_{COPIED} {}
    Traces(Traces &&from) noexcept: state_{MOVED} { from.state_ = MOVED_FROM; }
};

struct WillThrowOnSomeConstructions {
    static int globalOrder;
    static std::vector<int>
        built, destroyed;

    int order_;

    WillThrowOnSomeConstructions() noexcept(false) {
        if(5 == globalOrder) { throw std::runtime_error("disliking 5"); }
        order_ = globalOrder++;
        built.push_back(order_);
    }

    ~WillThrowOnSomeConstructions() {
        destroyed.push_back(order_);
    }
};

int WillThrowOnSomeConstructions::globalOrder = 0;

std::vector<int>
    WillThrowOnSomeConstructions::built,
    WillThrowOnSomeConstructions::destroyed;

}

TEST_CASE(
    "Aligned Storage Nested Arrays",
    "[nested-arrays][aligned-storage][contract][api]"
) {
    ForTypical8 ft8;
    SECTION("Nested Arrays") {
        ft8.build<Typical[8]>(arr8);
        CHECK((*ft8.as<Typical[8]>())[5].state_ == 6);
        ft8.destroy<Typical[8]>();
        ft8.build<Typical[4][2]>(arr42);
        CHECK((*ft8.as<Typical[4][2]>())[3][0].state_ == 41);
    }
    SECTION("Array value category") {
        ft8.build<Traces[8]>();
        auto &t8  = *ft8.as<Traces[8]>();
        ForTypical8 copy;
        copy.build<Traces[8]>(std::move(t8));
        CHECK(Traces::MOVED == (*copy.as<Traces[8]>())[0].state_);
        CHECK(Traces::MOVED_FROM == t8[4].state_);
    }
    SECTION("Partial build failures") {
        WillThrowOnSomeConstructions::globalOrder = 0;
        auto
            &built = WillThrowOnSomeConstructions::built,
            &destroyed = WillThrowOnSomeConstructions::destroyed;
        built.clear();
        destroyed.clear();
        CHECK_THROWS(ft8.build<WillThrowOnSomeConstructions[8]>());
        CHECK(5 == WillThrowOnSomeConstructions::built.size());
        std::vector<int> reversed(built.rbegin(), built.rend());
        CHECK(reversed == destroyed);
    }
}
