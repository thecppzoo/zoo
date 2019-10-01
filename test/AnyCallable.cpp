#include "AnyCallableGeneric.h"

TEST_CASE("function", "[any][type-erasure][functional]") {
    CallableTests<zoo::AnyContainer<zoo::CanonicalPolicy>>::execute();
}

using NonCopiable = zoo::AnyMovable<zoo::CanonicalPolicy>;

template<typename S>
using MoveOnlyCallable = zoo::AnyCallable<NonCopiable, S>;

static_assert(std::is_copy_constructible_v<zoo::function<void()>>);
static_assert(std::is_copy_assignable_v<zoo::function<void()>>);

static_assert(std::is_nothrow_move_constructible_v<MoveOnlyCallable<void()>>);
static_assert(std::is_nothrow_move_assignable_v<MoveOnlyCallable<void()>>);
