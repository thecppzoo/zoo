#ifndef TYPE_MODEL_H
#define TYPE_MODEL_H

#include <type_traits>

namespace zoo { namespace meta {

template<typename TypeToModel, typename ValueCategoryAndConstnessReferenceType>
struct copy_cr;

template<typename Type, typename LValue>
struct copy_cr<Type, LValue &> {
    using type =
        std::conditional_t<std::is_const_v<LValue>, const Type &, Type &>;
};

template<typename Type, typename RValue>
struct copy_cr<Type, RValue &&> {
    using type =
        std::conditional_t<std::is_const_v<RValue>, const Type &&, Type &&>;
};

template<typename Type, typename Reference>
using copy_cr_t = typename copy_cr<Type, Reference>::type;

template<typename T>
using remove_cr_t = std::remove_const_t<std::remove_reference_t<T>>;

#ifdef COMPILE_TIME_TESTS

static_assert(std::is_same_v<char &, copy_cr_t<char, int &>>);
static_assert(std::is_same_v<char &&, copy_cr_t<char, int &&>>);
static_assert(std::is_same_v<const char &, copy_cr_t<char, const int &>>);
static_assert(std::is_same_v<const char &&, copy_cr_t<char, const int &&>>);

#endif

}}

#endif
