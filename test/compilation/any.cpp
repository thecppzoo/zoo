#include "util/any.h"

struct alignas(32) aligned_to_32 {};

static_assert(alignof(zoo::detail::AlignedStorage<1, 32>) == 32, "");

zoo::Any makeAny(int &val) { return zoo::Any{val}; }

struct NotCopyConstructible {
	NotCopyConstructible(const NotCopyConstructible &) = delete;
};

#ifdef MALFORMED_ON_BUILDING_ANY_ON_NOT_COPY_CONSTRUCTIBLE_VALUE
zoo::Any makeAny(NotCopyConstructible &ncc) { return zoo::Any{ncc}; }
#endif
