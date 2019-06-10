#include "AnyCallable.h"

TEST_CASE("function", "[any][type-erasure][functional]") {
    CallableTests<zoo::AnyContainer<zoo::CanonicalPolicy>>::execute();
}
