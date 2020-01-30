//
//  AnyCallSignature.h
//
//  Created by Eduardo Madrid on 7/22/19.
//

#ifndef ZOO_AnyCallSignature_h
#define ZOO_AnyCallSignature_h

#include "AnyCallable.h"

namespace zoo {

class TypeToken {};

template<template<typename> class Fun>
struct AnyCallingSignature:
    protected Fun<TypeToken(TypeToken)>
{
    AnyCallingSignature() = default;

    template<typename S, typename... Args>
    #define PP_ZOO_CONSTRUCTION_EXPR Fun<S>(std::forward<Args>(args)...)
    AnyCallingSignature(std::in_place_type_t<S>, Args &&...args)
        noexcept(noexcept(PP_ZOO_CONSTRUCTION_EXPR))
    {
        new(this) PP_ZOO_CONSTRUCTION_EXPR;
    #undef PP_ZOO_CONSTRUCTION_EXPR
    }

    template<typename Signature>
    Fun<Signature> &as() {
        return reinterpret_cast<Fun<Signature> &>(*this);
    }
};

template<typename TypeErasure>
struct AnyCallableProjector {
    template<typename Signature>
    using projection = AnyCallable<TypeErasure, Signature>;
};

template<typename TypeErasure>
using AnyCallSignature =
    AnyCallingSignature<AnyCallableProjector<TypeErasure>::template projection>;

}

#endif /* AnyCallSignature_h */
