#ifndef TYPE_MODEL_H
#define TYPE_MODEL_H

#include <type_traits>

namespace zoo { namespace meta {

template<typename TypeToModel, typename ValueCategoryAndConstnessReferenceType>
struct TypeModel;

template<typename Type, typename LValue>
struct TypeModel<Type, LValue &> {
    using type =
        std::conditional_t<std::is_const_v<LValue>, const Type &, Type &>;
};

template<typename Type, typename RValue>
struct TypeModel<Type, RValue &&> {
    using type =
        std::conditional_t<std::is_const_v<RValue>, const Type &&, Type &&>;
};

template<typename Type, typename Reference>
using TypeModel_t = typename TypeModel<Type, Reference>::type;

#ifdef COMPILE_TIME_TESTS

static_assert(std::is_same_v<char &, TypeModel_t<char, int &>>);
static_assert(std::is_same_v<char &&, TypeModel_t<char, int &&>>);
static_assert(std::is_same_v<const char &, TypeModel_t<char, const int &>>);
static_assert(std::is_same_v<const char &&, TypeModel_t<char, const int &&>>);

#endif

}}

#endif
