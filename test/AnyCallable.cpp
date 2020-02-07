#include "AnyCallableGeneric.h"

template<typename S>
using Functor = zoo::AnyCallable<zoo::AnyContainer<zoo::CanonicalPolicy>, S>;

TEST_CASE("function", "[any][type-erasure][functional]") {
    CallableTests<Functor>::execute();
}
