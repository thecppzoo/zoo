#ifndef ZOO_ANY_CALLABLE
#define ZOO_ANY_CALLABLE

#include <meta/NotBasedOn.h>

#ifndef SIMPLIFY_PREPROCESSING
#include <type_traits>
#include <functional>
#endif

namespace zoo {

/// \brief Type erasure of callables
///
/// The unrestricted template is not useful
/// use through the function signature specializaton
template<typename, typename>
struct AnyCallable;


/// \tparam TypeErasureProvider Must implement a \c state template instance
/// function that returns a pointer to the held object
///
/// Why inheritance: Client code should be able to enquire the target
/// as they would any type erased value
template<typename TypeErasureProvider, typename R, typename... Args>
struct AnyCallable<TypeErasureProvider, R(Args...)>: TypeErasureProvider {
    static constexpr auto emptyInvoker_ =
        [](Args..., TypeErasureProvider &) -> R {
            throw std::bad_function_call{};
        };

    template<typename T>
    static R invokeTarget(Args... as, TypeErasureProvider &obj) {
        return (*obj.template state<T>())(as...);
    }

    R (*targetInvoker_)(Args..., TypeErasureProvider &);

    AnyCallable():
        targetInvoker_{ emptyInvoker_ }
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
        TypeErasureProvider{std::forward<Callable>(target)},
        targetInvoker_{invokeTarget<Decayed>}
    {}

    template<typename... RealArguments>
    R operator()(RealArguments &&... args) const {
        return
            targetInvoker_(
                std::forward<RealArguments>(args)...,
                const_cast<AnyCallable &>(*this)
            );
    }

    void swap(AnyCallable& other) noexcept {
        TypeErasureProvider::swap(other);
        std::swap(targetInvoker_, other.targetInvoker_);
    }

    explicit operator bool() const noexcept {
        return !empty();
    }

    const std::type_info& target_type() const noexcept {
        return empty() ? typeid(void) : this->type();
    }

    bool empty() const noexcept {
        return targetInvoker_ == emptyInvoker_;
    }

    template< class T >
    T* target() noexcept {
        using uncvr_t = std::remove_cv_t<std::remove_reference_t<T>>;
        if (!empty() && target_type() == typeid(uncvr_t))
            return this->template state<T>();

        return nullptr;
    }
    
    template< class T >
    T const* target() const noexcept {
        return const_cast<AnyCallable*>(this)->target<T>();
    }
};

// nullptr comparison
template<typename TypeErasureProvider, typename R, typename... Args>
bool operator==(AnyCallable<TypeErasureProvider, R(Args...)> const& ac, std::nullptr_t) {
    return ac.empty();
}

template<typename TypeErasureProvider, typename R, typename... Args>
bool operator==(std::nullptr_t, AnyCallable<TypeErasureProvider, R(Args...)> const& ac) {
    return ac.empty();
}

template<typename TypeErasureProvider, typename R, typename... Args>
bool operator!=(AnyCallable<TypeErasureProvider, R(Args...)> const& ac, std::nullptr_t) {
    return !ac.empty();
}

template<typename TypeErasureProvider, typename R, typename... Args>
bool operator!=(std::nullptr_t, AnyCallable<TypeErasureProvider, R(Args...)> const& ac) {
    return !ac.empty();
}

} // namespace zoo

#endif
