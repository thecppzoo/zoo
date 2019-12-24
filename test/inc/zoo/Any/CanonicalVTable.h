#ifndef ZOOTEST_CANONICALVTABLE_H
#define ZOOTEST_CANONICALVTABLE_H

#include "VTablePolicy.h"

namespace zoo {

struct CanonicalVTable:
    Destroy::VTableEntry, Move::VTableEntry, Copy::VTableEntry {};

struct CanonicalVTableContainer:
    VTableHolder<CanonicalVTable>,
    Destroy::Mixin<CanonicalVTableContainer>,
    Move::Mixin<CanonicalVTableContainer>,
    Copy::Mixin<CanonicalVTableContainer>
{
    AlignedStorageFor<void *> space_;

    static void moveVTable(void *to, void *from) noexcept {
        static_cast<CanonicalVTableContainer *>(to)->ptr_ =
            static_cast<CanonicalVTableContainer *>(from)->ptr_;
    }

    static void copyVTable(void *to, const void *from) {
        static_cast<CanonicalVTableContainer *>(to)->ptr_ =
            static_cast<const CanonicalVTableContainer *>(from)->ptr_;
    }

    constexpr static inline CanonicalVTable Default = {
        Destroy::Default<CanonicalVTableContainer>,
        Move::Default<CanonicalVTableContainer>,
        Copy::Default<CanonicalVTableContainer>
    };

    CanonicalVTableContainer(): VTableHolder(&Default) {}

    using VTableHolder::VTableHolder;
};

template<typename V>
struct ByValue: CanonicalVTableContainer {
    V *value() noexcept { return space_.template as<V>(); }
    const V *value() const noexcept {
        return const_cast<ByValue *>(this)->value();
    }

    static void destructor(void *p) noexcept {
        static_cast<ByValue *>(p)->value()->~V();
    }

    static void move(void *to, void *from) noexcept {
        auto downcast = static_cast<ByValue *>(from);
        new(to) ByValue(std::move(*downcast->value()));
    }

    static void copy(void *to, const void *from) {
        auto downcast = static_cast<const ByValue *>(from);
        new(to) ByValue(*downcast->value());
    }

    constexpr static inline CanonicalVTable Operations = {
        Destroy::Operation<ByValue>,
        Move::Operation<ByValue>,
        Copy::Operation<ByValue>
    };

    template<typename... Args>
    ByValue(Args &&...args): CanonicalVTableContainer(&Operations) {
        this->space_.template build<V>(std::forward<Args>(args)...);
    }
};


template<typename V>
struct ByReference: CanonicalVTableContainer {
    V *&pValue() noexcept { return *space_.template as<V *>(); }
    V *value() noexcept { return pValue(); }
    const V *value() const noexcept {
        return const_cast<ByReference *>(this)->value();
    }

    static void destructor(void *p) noexcept {
        auto ptr = static_cast<ByReference *>(p)->pValue();
        if(ptr) { delete ptr; }
    }

    static void move(void *to, void *from) noexcept {
        auto downcast = static_cast<ByReference *>(from);
        new(to) ByReference(downcast);
        downcast->pValue() = nullptr;
    }

    static void copy(void *to, const void *from) {
        auto downcast = static_cast<const ByReference *>(from);
        new(to) ByReference(*downcast->value());
    }

    constexpr static inline CanonicalVTable Operations = {
        Destroy::Operation<ByReference>,
        Move::Operation<ByReference>,
        Copy::Operation<ByReference>
    };

    ByReference(ByReference *source): CanonicalVTableContainer(&Operations) {
        pValue() = source->pValue();
    }

    template<typename... Args>
    ByReference(Args &&...args): CanonicalVTableContainer(&Operations) {
        pValue() = new V(std::forward<Args>(args)...);
    }
};

struct CanonicalVTablePolicy {
    using MemoryLayout = CanonicalVTableContainer;

    template<typename V>
    using Builder =
        std::conditional_t<
            BuilderDecider<8, 8, V>::SmallBufferSuitable,
            ByValue<V>,
            ByReference<V>
        >;
};

}


#endif //ZOOTEST_CANONICALVTABLE_H
