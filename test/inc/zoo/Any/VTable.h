#ifndef Zoo_Any_VTable_h
#define Zoo_Any_VTable_h

#include "zoo/Any/Traits.h"

#include <new>
#include <typeinfo>

namespace zoo {

struct TypeErasureOperations {
    void (*destroy)(void *) noexcept;
    void (*move)(void *from, void *to) noexcept;
};

template<int Size, int Alignment>
struct TypeErasedContainer {
    alignas(Alignment) char space_[Size];
    const TypeErasureOperations *vTable_ = &Empty;

    template<typename T>
    T *as() noexcept { return reinterpret_cast<T *>(space_); }

    static void copyVTable(const void *from, void *to) noexcept {
        static_cast<TypeErasedContainer *>(to)->vTable_ =
            static_cast<const TypeErasedContainer *>(from)->vTable_;
    }

    // note: the policy operations do not assume an "Empty" to allow inheritance
    // contravariance of the AnyContainer
    inline const static TypeErasureOperations Empty = {
        [](void *) noexcept {},
        reinterpret_cast<void (*)(void *, void *) noexcept>(copyVTable)
    };

    void destroy() noexcept { vTable_->destroy(this); }
    void move(void *to) noexcept { vTable_->move(this, to); }
};

template<int Size, int Alignment, typename V>
struct SmallBufferTypeEraser: TypeErasedContainer<Size, Alignment> {
    using Me = SmallBufferTypeEraser;

    V *value() noexcept { return this->template as<V>(); }

    inline const static TypeErasureOperations VTable = {
        [](void *who) noexcept { static_cast<Me *>(who)->value()->~V(); },
        [](void *from, void *to) noexcept {
            new (to) Me(std::move(*static_cast<Me *>(from)->value()));
        }
    };

    template<typename... Args>
    SmallBufferTypeEraser(Args &&...args) {
        this->vTable_ = &VTable;
        new(value()) V(std::forward<Args>(args)...);
    }
};

template<int Size, int Alignment, typename V>
struct ReferentialTypeEraser: TypeErasedContainer<Size, Alignment> {
    using Me = ReferentialTypeEraser;
    
    V *&value() noexcept { return *this->template as<V *>(); }
    
    inline const static TypeErasureOperations VTable = {
        [](void *who) noexcept { delete static_cast<Me *>(who)->value(); },
        [](void *from, void *to) noexcept {
            auto recipient = static_cast<Me *>(to);
            auto source = static_cast<Me *>(from);
            recipient->vTable_ = source->vTable_;
            recipient->value() = source->value();
            source->vTable = &TypeErasedContainer<Size, Alignment>::Empty;
        }
    };
    
    template<typename... Args>
    ReferentialTypeEraser(Args &&...args) {
        this->vTable_ = &VTable;
        value() = new V(std::forward<Args>(args)...);
    }
};

template<int Size, int Alignment>
struct VTablePolicy {
    using MemoryLayout = TypeErasedContainer<Size, Alignment>;

    template<typename T>
    using Builder = std::conditional_t<
        sizeof(T) <= Size && (0 == Alignment % alignof(T)) &&
            std::is_nothrow_move_constructible_v<T>,
        SmallBufferTypeEraser<Size, Alignment, T>,
        ReferentialTypeEraser<Size, Alignment, T>
    >;

    constexpr static auto RequireMoveOnly = true;
};

}

#endif /* VTable_h */
