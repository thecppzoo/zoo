#pragma once

#include "zoo/ConverterAny.h"

namespace zoo {

template<typename T, int Size, int Alignment>
bool isRuntimeValue(IAnyContainer<Size, Alignment> *ptr) {
    return dynamic_cast<ValueContainer<Size, Alignment, T> *>(ptr);
}

template<typename T, int Size, int Alignment>
bool isRuntimeReference(IAnyContainer<Size, Alignment> *ptr) {
    return dynamic_cast<ReferentialContainer<Size, Alignment, T> *>(ptr);
}

template<typename T, int Size, int Alignment>
bool isRuntimeValue(ConverterContainer<Size, Alignment> *ptr) {
    return dynamic_cast<ConverterValueDriver<T> *>(ptr->driver());
}

template<typename T, int Size, int Alignment>
bool isRuntimeReference(ConverterContainer<Size, Alignment> *ptr) {
    return dynamic_cast<ConverterReferentialDriver<T> *>(ptr->driver());
}

template<typename Policy>
struct RuntimeReferenceOrValueDiscriminator {
    template<typename T>
    static bool runtimeReference(typename Policy::MemoryLayout *ptr) {
        return isRuntimeReference<T>(ptr);
    }

    template<typename T>
    static bool runtimeValue(typename Policy::MemoryLayout *ptr) {
        return isRuntimeValue<T>(ptr);
    }
};

template<typename T, typename Policy>
bool isRuntimeValue(AnyContainer<Policy> &a) {
    return RuntimeReferenceOrValueDiscriminator<Policy>::
        template runtimeValue<T>(a.container());
}

template<typename T, typename Policy>
bool isRuntimeReference(AnyContainer<Policy> &a) {
    return RuntimeReferenceOrValueDiscriminator<Policy>::
        template runtimeReference<T>(a.container());
}

}

