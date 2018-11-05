struct SwapMayThrow {};
void swap(SwapMayThrow &, SwapMayThrow &) noexcept(false);

#include "valid_expression.h"

#include <zoo/any.h>

using std::declval;

struct alignas(32) aligned_to_32 {};

static_assert(alignof(zoo::IAnyContainer<1, 32>) == 32, "");

struct NotCopyConstructible {
	NotCopyConstructible(const NotCopyConstructible &) = delete;
};

VALID_EXPRESSION(ConstructsCopy, T, zoo::Any{declval<T &>()});

static_assert(ConstructsCopy<int>::value, "Checks CopyConstructible");
static_assert(
    !ConstructsCopy<NotCopyConstructible>::value,
    "Any is copying NotCopyConstructible"
);


template<typename T>
using MayThrowOnSwap =
    std::conditional_t<
        noexcept(swap(declval<T &>(), declval<T &>())),
        std::false_type,
        std::true_type
    >;

static_assert(!MayThrowOnSwap<zoo::Any>::value, "");
static_assert(MayThrowOnSwap<SwapMayThrow>::value, "");
