#include "util/any.h"

struct alignas(32) aligned_to_32 {};

static_assert(alignof(zoo::detail::AlignedStorage<1, 32>) == 32, "");

zoo::Any makeAny(int val) { return zoo::Any{val}; }

