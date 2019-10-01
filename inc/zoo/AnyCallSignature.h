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

template<typename TypeErasure>
struct AnyCallSignature:
    protected AnyCallable<TypeErasure, TypeToken(TypeToken)>
{
    AnyCallSignature() = default;

    template<typename S, typename... Args>
    #define PP_ZOO_CONSTRUCTION_EXPR AnyCallable<TypeErasure, S>(std::forward<Args>(args)...)
    AnyCallSignature(std::in_place_type_t<S>, Args &&...args)
        noexcept(noexcept(PP_ZOO_CONSTRUCTION_EXPR))
    {
        new(this) PP_ZOO_CONSTRUCTION_EXPR;
    #undef PP_ZOO_CONSTRUCTION_EXPR
    }

    template<typename Signature>
    AnyCallable<TypeErasure, Signature> &as() {
        return reinterpret_cast<AnyCallable<TypeErasure, Signature> &>(*this);
    }
};

}

#endif /* AnyCallSignature_h */
