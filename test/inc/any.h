#ifndef TEST_ANY
#define TEST_ANY

#include <stdexcept>

#include <zoo/any.h>

static constexpr auto BigSize = 32;
using LargePolicy = zoo::RuntimePolymorphicAnyPolicy<BigSize, 8>;
using LargeTypeEraser = zoo::AnyContainer<LargePolicy>;

namespace zoo {

template<typename T, typename Policy>
bool isRuntimeReference(const AnyContainer<Policy> &a) {
    if(typeid(T) != a.type()) {
        throw std::runtime_error("Not the same type of value");
    }
    using C = typename Policy::template Builder<T>;
    return C::IsReference;
}

template<typename T, typename Policy>
bool isRuntimeValue(const AnyContainer<Policy> &a) {
    if(typeid(T) != a.type()) {
        throw std::runtime_error("Not the same type of value");
    }
    using C = typename Policy::template Builder<T>;
    return !C::IsReference;
}

}

#endif
