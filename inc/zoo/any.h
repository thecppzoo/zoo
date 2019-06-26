#ifndef ZOO_ANY_H
#define ZOO_ANY_H

#include "zoo/AnyContainer.h"
#include "zoo/PolymorphicContainer.h"

namespace zoo {

using CanonicalPolicy =
RuntimePolymorphicAnyPolicy<sizeof(void *), alignof(void *)>;

using Any = AnyContainer<CanonicalPolicy>;

struct bad_any_cast: std::bad_cast {
    using std::bad_cast::bad_cast;

    const char *what() const noexcept override {
        return "Incorrect Any casting";
    }
};

template<class ValueType>
ValueType any_cast(const Any &operand) {
    using U = std::decay_t<ValueType>;
    static_assert(std::is_constructible<ValueType, const U &>::value, "");
    if(typeid(U) != operand.type()) { throw bad_any_cast{}; }
    return *anyContainerCast<U>(&operand);
}

template<class ValueType>
ValueType any_cast(Any &operand) {
    using U = std::decay_t<ValueType>;
    static_assert(std::is_constructible<ValueType, U &>::value, "");
    if(typeid(U) != operand.type()) { throw bad_any_cast{}; }
    return *anyContainerCast<U>(&operand);
}

template<class ValueType>
ValueType any_cast(Any &&operand) {
    using U = std::decay_t<ValueType>;
    static_assert(std::is_constructible<ValueType, U>::value, "");
    if(typeid(U) != operand.type()) { throw bad_any_cast{}; }
    return std::move(*anyContainerCast<U>(&operand));
}

template<class ValueType>
ValueType *any_cast(Any *operand) {
    if(!operand) { return nullptr; }
    using U = std::decay_t<ValueType>;
    if(typeid(U) != operand->type()) { return nullptr; }
    return anyContainerCast<U>(operand);
}

template<typename ValueType>
const ValueType *any_cast(const Any *ptr) {
    return any_cast<ValueType>(const_cast<Any *>(ptr));
}

template<typename T, typename... Args>
Any make_any(Args &&... args) {
    return Any(std::in_place_type<T>, std::forward<Args>(args)...);
}

template<typename Policy>
typename Policy::Visit *visits(const AnyContainer<Policy> &a) {
    return a.container()->visits();
}

}

#endif
