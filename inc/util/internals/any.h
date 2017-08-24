#pragma once

#include "util/any2.h"

namespace zoo { namespace internals {

struct AnyExtensions: Any {
    static const zoo::Any::Container *ts(const zoo::Any &a) {
        return static_cast<const AnyExtensions *>(&a)->container();
    }

    template<typename T>
    static bool isAValue(const zoo::Any &a) {
        return dynamic_cast<const zoo::ValueContainer<8, 8, T> *>(ts(a));
    }

    template<typename T>
    static bool isReferential(const zoo::Any &a) {
    	return dynamic_cast<const zoo::ReferentialContainer<8, 8, T> *>(ts(a));
    }
};

}}