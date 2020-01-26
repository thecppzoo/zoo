#ifndef ZOO_FUNCTIONPOLICY_H
#define ZOO_FUNCTIONPOLICY_H

#include "zoo/Any/DerivedVTablePolicy.h"

#include "zoo/AnyContainer.h"

namespace zoo {

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

template<typename R, typename... As>
struct Executor<R(As...)> {
    R (*executor_)(As..., void *) = [](As..., void *) -> R { throw 5; };

    Executor() = default;
    Executor(R (*ptr)(As..., void *)): executor_(ptr) {}
};

template<typename Signature>
Executor(Signature *) -> Executor<Signature>;

template<typename, typename>
struct Function;

template<typename ModelType, typename R, typename... As>
struct Function<ModelType, R(As...)>: Executor<R(As...)>, AnyContainer<Policy<ModelType, Destroy, Move>> {
    using ModelPolicy = Policy<ModelType, Destroy, Move>;
    using ContainerBase = AnyContainer<ModelPolicy>;

    template<typename Target>
    static R invokeTarget(As... args, void *p) {
        return (*static_cast<Function *>(p)->template state<Target>())(args...);
    }

    Function() = default;
    Function(const Function &) = default;
    Function(Function &&) = default;

    template<
        typename Target,
        typename D = std::decay_t<Target>,
        int = std::enable_if_t<!std::is_base_of_v<Function, D>, int>(0)
    >
    Function(Target &&a):
        Executor<R(As...)>(invokeTarget<std::decay_t<Target>>),
        ContainerBase(std::forward<Target>(a))
    {}

    Function(typename ContainerBase::TokenType):
        Executor<R(As...)>(nullptr),
        ContainerBase(ContainerBase::Token)
    {}

    template<typename... CallArguments>
    R operator()(CallArguments &&...cas) {
        return this->executor_(std::forward<CallArguments>(cas)..., this);
    }

    template<typename T>
    void emplaced(T *ptr) noexcept {
        this->executor_ = invokeTarget<T>;
    }
};

}


#endif //ZOO_FUNCTIONPOLICY_H
