//
//  AnyCallSignature.h
//
//  Created by Eduardo Madrid on 7/22/19.
//

#ifndef ZOO_AnyCallSignature_h
#define ZOO_AnyCallSignature_h

#include "zoo/AlignedStorage.h"
#include "meta/NotBasedOn.h"

#include <utility>

namespace zoo {

template<typename>
struct ReturnType_t;
template<typename R, typename... Args>
struct ReturnType_t<R(Args...)> { // pending noexcept
    using type = R;
};

template<typename S>
using ReturnType = typename ReturnType_t<S>::type;

template<typename, typename>
struct MakeInvoker;
template<typename Container, typename R, typename... Args>
struct MakeInvoker<Container, R(Args...)> {
    using type = R(*)(Args..., void *);

    template<typename T>
    static R function(Args... args, void *who) {
        return
            (*static_cast<Container *>(who)->template state<T>()) (
                std::forward<Args>(args)...
            );
    }
};

template<typename TypeErasureProvider>
struct AnyCallSignature: TypeErasureProvider {
    /*template<
        typename Argument,
        typename =
            std::enable_if_t<
                std::is_base_of_v<AnyCallSignature, std::decay_t<Argument>>
            >
    >
    AnyCallSignature(Argument &&a):
        TypeErasureProvider(std::forward<Argument>(a))
    {}*/

    using TypeErasureProvider::TypeErasureProvider;

protected:
    template<typename S>
    using FunctionPointer = typename MakeInvoker<TypeErasureProvider, S>::type;

    using AnySignature = void(*)();
    AlignedStorage<sizeof(AnySignature), alignof(AnySignature)> space_;

public:
    template<typename Target, typename Signature>
    AnyCallSignature(Target &&t, std::in_place_type_t<Signature>):
        TypeErasureProvider(std::forward<Target>(t))
    {
        space_.build<FunctionPointer<Signature>>(
            &MakeInvoker<TypeErasureProvider, Signature>::
                template function<std::decay_t<Target>>
        );
    }

    template<typename Signature, typename... As>
    ReturnType<Signature> call(As &&...as) {
        return
            (*space_.as<FunctionPointer<Signature>>())(
                std::forward<As>(as)..., this
            );
    }
};

}

#endif /* AnyCallSignature_h */
