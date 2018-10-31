#ifndef ZOO_ANY_CALLABLE
#define ZOO_ANY_CALLABLE

#ifndef ZOO_USE_EXPERIMENTAL
#error Experimental code needs to define ZOO_USE_EXPERIMENTAL
#endif

#include <zoo/any.h>

#ifndef SIMPLIFY_PREPROCESSING
#include <functional>
#endif

namespace zoo {

/// \brief Type erasure of callables
///
/// The unrestricted template is not useful
/// use through the function signature specializaton
template<typename>
struct AnyCallable;

template<typename R, typename... Args>
struct AnyCallable<R(Args...)> {
    Any typeErasedTarget_;
    R (*compress_)(Args..., Any &);

    AnyCallable():
        compress_{
            [](Args..., Any &) -> R { throw std::bad_function_call{}; }
        }
    {}

    template<
        typename Callable,
        typename Decayed =
            std::enable_if_t<
                meta::NotBasedOn<Callable, AnyCallable>(),
                std::decay_t<Callable>
            >
    >
    AnyCallable(Callable &&target):
        typeErasedTarget_{std::forward<Callable>(target)},
        compress_{
            [](Args... arguments, Any &erased) {
                return (*erased.state<Decayed>())(arguments...);
            }
        }
    {}

    template<typename... RealArguments>
    R operator()(RealArguments &&... args) {
        return
            compress_(std::forward<RealArguments>(args)..., typeErasedTarget_);
    }
};

}

#endif
