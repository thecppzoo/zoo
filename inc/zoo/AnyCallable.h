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
    AnyCallable(): targetInvoker_{emptyInvoker} {}

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

    void swap(AnyCallable &other) noexcept(
        noexcept(swapBase(*this, other))
    ) {
        swapBase(*this, other);
        std::swap(targetInvoker_, other.targetInvoker_);
    }

    explicit operator bool() const noexcept {
        return !empty();
    }

    const std::type_info &target_type() const noexcept {
        return empty() ? typeid(void) : this->type();
    }

    bool empty() const noexcept {
        return emptyInvoker == targetInvoker_;
    }

    template<typename T>
    T *target() noexcept {
        using uncvr_t = std::remove_cv_t<std::remove_reference_t<T>>;
        if (!empty() && target_type() == typeid(uncvr_t))
            return this->template state<T>();

        return nullptr;
    }
    
    template< class T >
    T const* target() const noexcept {
        return const_cast<AnyCallable*>(this)->target<T>();
    }

    bool operator==(std::nullptr_t) const noexcept {
        return empty();
    }

    bool operator!=(std::nullptr_t) const noexcept {
        return not(*this == nullptr);
    }

private:
    R (*targetInvoker_)(Args..., TypeErasureProvider &);

    static R emptyInvoker(Args..., TypeErasureProvider &) {
        throw std::bad_function_call{};
    }

    template<typename T>
    static R invokeTarget(Args... as, TypeErasureProvider &obj) {
        return (*obj.template state<T>())(as...);
    }

    /// Used to perform casting and the implicit checks of std::swap in
    /// C++ 17 with regards to swappability and move constructibility
    void swapBase(TypeErasureProvider &thy, TypeErasureProvider &other)
        noexcept(noexcept(std::swap(thy, other)))
    {
        std::swap(thy, other);
    }
};

template<typename TypeErasureProvider, typename R, typename... Args>
bool operator==(
    std::nullptr_t, AnyCallable<TypeErasureProvider, R(Args...)> const &ac
) noexcept {
    return ac == nullptr;
}

template<typename TypeErasureProvider, typename R, typename... Args>
bool operator!=(
    std::nullptr_t, AnyCallable<TypeErasureProvider, R(Args...)> const &ac
) noexcept {
    return ac != nullptr;
}

} // namespace zoo

#endif
