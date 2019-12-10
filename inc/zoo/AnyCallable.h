#ifndef ZOO_ANY_CALLABLE
#define ZOO_ANY_CALLABLE

#include "zoo/meta/NotBasedOn.h"

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

template<typename E, typename S>
inline
void swap(AnyCallable<E, S> &ac1, AnyCallable<E, S> &ac2) noexcept;

/// \tparam TypeErasureProvider Must implement a \c state template instance
/// function that returns a pointer to the held object
///
/// Why inheritance: Client code should be able to enquire the target
/// as they would any type erased value
template<typename TypeErasureProvider, typename R, typename... Args>
struct AnyCallable<TypeErasureProvider, R(Args...)>: TypeErasureProvider {
    AnyCallable(): targetInvoker_{emptyInvoker} {}

    AnyCallable(std::nullptr_t): AnyCallable() {}

    template<
        typename Callable,
        typename Decayed =
            std::enable_if_t<
                meta::NotBasedOn<Callable, AnyCallable>(),
                std::decay_t<Callable>
            >,
        typename = decltype(std::declval<Decayed>()(std::declval<Args>()...))
    >
    AnyCallable(Callable &&target):
        TypeErasureProvider{std::forward<Callable>(target)},
        targetInvoker_{invokeTarget<Decayed>}
    {}

    template<typename Argument>
    AnyCallable &operator=(Argument &&a) noexcept(
        noexcept(std::decay_t<Argument>(std::forward<Argument>(a)))
    ) {
        this->container()->destroy();
        new(this) AnyCallable(std::forward<Argument>(a));
        return *this;
    }

    template<typename... RealArguments>
    R operator()(RealArguments &&... args) const {
        return
            targetInvoker_(
                std::forward<RealArguments>(args)...,
                const_cast<AnyCallable &>(*this)
            );
    }

    void swap(AnyCallable &other) noexcept {
        auto &upcasted = static_cast<TypeErasureProvider &>(*this);
        static_assert(noexcept(upcasted.swap(other)), "");
        upcasted.swap(other);
        zoo::swap(targetInvoker_, other.targetInvoker_);
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

    /// \note \c target should not be used, it is a consequence of what we
    /// believe is the design mistake in the standard library of not basing
    /// std::function on a user-available type erasure component,
    /// the correct way to reach the held object is through \c state
    /// and choose between safe access and unsafe
    template<typename T>
    T *target() noexcept {
        using Decayed = std::decay_t<T>;
        if(typeid(Decayed) != this->type()) { return nullptr; }
        return this->template state<T>();
    }
    
    template<typename T>
    T const *target() const noexcept {
        return const_cast<AnyCallable*>(this)->target<T>();
    }

    bool operator==(std::nullptr_t) const noexcept { return empty(); }

    bool operator!=(std::nullptr_t) const noexcept {
        return not(*this == nullptr);
    }

private:
    template<typename E, typename S>
    friend inline
    void swap(AnyCallable<E, S> &, AnyCallable<E, S> &) noexcept;

    R (*targetInvoker_)(Args..., TypeErasureProvider &);

    static R emptyInvoker(Args..., TypeErasureProvider &) {
        throw std::bad_function_call{};
    }

    template<typename T>
    static R invokeTarget(Args... as, TypeErasureProvider &obj) {
        return (*obj.template state<T>())(std::forward<Args>(as)...);
    }
};

template<typename TypeErasureProvider, typename Signature>
inline
bool operator==(
    std::nullptr_t, AnyCallable<TypeErasureProvider, Signature> const &ac
) noexcept {
    return ac == nullptr;
}

template<typename TypeErasureProvider, typename Signature>
inline
bool operator!=(
    std::nullptr_t, AnyCallable<TypeErasureProvider, Signature> const &ac
) noexcept {
    return ac != nullptr;
}

template<typename E, typename S>
inline
void swap(AnyCallable<E, S> &c1, AnyCallable<E, S> &c2) noexcept {
    auto &upcasted = static_cast<E &>(c1);
    static_assert(
        noexcept(swap(upcasted, c2)),
        "Requires swap of type eraser to be noexcept"
    );
    swap(upcasted, c2);
    swap(c1.targetInvoker_, c2.targetInvoker_);
}

} // namespace zoo

#endif
