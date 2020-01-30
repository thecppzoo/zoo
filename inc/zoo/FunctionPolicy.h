#ifndef ZOO_FUNCTIONPOLICY_H
#define ZOO_FUNCTIONPOLICY_H

#include "zoo/Any/DerivedVTablePolicy.h"

#include "zoo/AnyContainer.h"

#include <functional>

namespace zoo {

namespace impl {

template<typename, typename, typename = void>
struct MayBeCalled: std::false_type {};

template<typename R, typename... As, typename Target>
struct MayBeCalled<
        #define PP_INVOCATION_DECLVAL decltype(std::declval<Target &>()(std::declval<As>()...))
    R(As...), Target, std::void_t<PP_INVOCATION_DECLVAL>
>: std::conjunction<std::is_constructible<R, PP_INVOCATION_DECLVAL>> {};
        #undef PP_INVOCATION_DECLVAL

}

template<std::size_t LocalBufferInPointers, typename Signature>
using CallableViaVTablePolicy =
    typename GenericPolicy<
        void *[LocalBufferInPointers],
        Destroy, Move, CallableViaVTable<Signature>
    >::Policy;

template<std::size_t Pointers, typename Signature>
using NewZooFunction = AnyContainer<CallableViaVTablePolicy<Pointers, Signature>>;
template<std::size_t Pointers, typename Signature>
using NewCopyableFunction =
    AnyContainer<DerivedVTablePolicy<NewZooFunction<Pointers, Signature>, Copy>>;

template<typename>
struct Executor;

struct bad_function_call: std::bad_function_call {
    using std::bad_function_call::bad_function_call;
};

template<typename R, typename... As>
struct Executor<R(As...)> {
    constexpr static R (*DefaultExecutor)(As..., void *) =
        [](As..., void *) -> R {
            throw bad_function_call();
        };

    R (*executor_)(As..., void *);

    Executor(): executor_(DefaultExecutor) {}
    Executor(R (*ptr)(As..., void *)): executor_(ptr) {}

    void swap(Executor &other) noexcept {
        std::swap(*this, other);
    }
};

template<typename Signature>
Executor(Signature *) -> Executor<Signature>;

template<typename, typename>
struct Function;

template<typename ContainerBase, typename R, typename... As>
struct Function<ContainerBase, R(As...)>:
    Executor<R(As...)>, ContainerBase
{
    template<typename Target>
    static R invokeTarget(As... args, void *p) {
        return (*static_cast<Function *>(p)->template state<Target>())(args...);
    }

    Function() = default;
    Function(const Function &) = default;
    Function(Function &&) = default;

protected:
    using Ex = Executor<R(As...)>;

    Function(typename ContainerBase::TokenType):
        Ex(nullptr),
        ContainerBase(ContainerBase::Token)
    {}
    Function(typename ContainerBase::TokenType, std::nullptr_t):
        ContainerBase(ContainerBase::Token)
    {}
    Function(typename ContainerBase::TokenType, Function &&f) noexcept:
        Ex(f.ptr_),
        ContainerBase(ContainerBase::Token, std::move(f))
    {}
    Function(typename ContainerBase::TokenType, const Function &f):
        Ex(f.ptr_),
        ContainerBase(ContainerBase::Token, f)
    {}

    template<typename T>
    void emplaced(T *ptr) noexcept {
        this->executor_ = invokeTarget<T>;
    }

public:
    Function(std::nullptr_t): Function() {}

    template<
        typename Target,
        typename D = std::decay_t<Target>,
        int =
            std::enable_if_t<
                !std::is_base_of_v<Function, D> &&
                    impl::MayBeCalled<R(As...), D>::value,
                int
            >(0)
    >
    Function(Target &&a):
        Executor<R(As...)>(invokeTarget<std::decay_t<Target>>),
        ContainerBase(std::forward<Target>(a))
    {}

    template<typename... CallArguments>
    R operator()(CallArguments &&...cas) {
        return this->executor_(std::forward<CallArguments>(cas)..., this);
    }

    void swap(Function &other) noexcept {
        auto &base = static_cast<ContainerBase &>(*this);
        base.swap(other);
        auto &executor = static_cast<Executor<R(As...)> &>(*this);
        executor.swap(other);
    }
};

template<typename C, typename Signature>
void swap(Function<C, Signature> &f1, Function<C, Signature> &f2) {
    f1.swap(f2);
}

template<typename ContainerBase>
struct FunctionProjector {
    template<typename Signature>
    using projection = Function<ContainerBase, Signature>;
};

}


#endif //ZOO_FUNCTIONPOLICY_H
