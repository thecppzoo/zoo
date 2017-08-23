#pragma once

#include "util/any.h"

namespace zoo { namespace internals {

struct AnyExtensions: Any {
    static const zoo::Any::TypeSwitch *ts(const zoo::Any &a) {
        return &static_cast<const AnyExtensions *>(&a)->m_typeSwitch;
    }

    template<typename T>
    static bool isAValue(const zoo::Any &a) {
        return dynamic_cast<const zoo::Any::Value<T> *>(ts(a));
    }

    template<typename T>
    static bool isReferential(const zoo::Any &a) {
    	return dynamic_cast<const zoo::Any::Referential<T> *>(ts(a));
    }
};

}}