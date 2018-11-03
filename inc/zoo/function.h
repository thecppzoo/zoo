#ifndef ZOO_FUNCTION
#define ZOO_FUNCTION

#include <zoo/AnyCallable.h>
#include <zoo/any.h>

namespace zoo {

template<typename Signature>
using function = AnyCallable<Any, Signature>;

}

#endif