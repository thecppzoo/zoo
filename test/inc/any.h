#include <zoo/ExtendedAny.h>

static constexpr auto BigSize = 32;
using LargePolicy = zoo::RuntimePolymorphicAnyPolicy<BigSize, 8>;
using LargeTypeEraser = zoo::AnyContainer<LargePolicy>;
