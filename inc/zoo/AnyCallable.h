#ifndef ZOO_ANY_CALLABLE
#define ZOO_ANY_CALLABLE

#ifndef ZOO_USE_EXPERIMENTAL
#error Experimental code needs to define ZOO_USE_EXPERIMENTAL
#endif

#include <zoo/any.h>

namespace zoo {

/// \brief Type erasure of callables
///
/// The unrestricted template is not useful
/// use through the function signature specializaton
template<typename>
struct AnyCallable;

template<typename R, typename... Args>
struct AnyCallable<R(Args...)> {
    
};

}

#endif