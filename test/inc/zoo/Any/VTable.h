#ifndef Zoo_Any_VTable_h
#define Zoo_Any_VTable_h

#include "zoo/Any/Traits.h"

#include <new>
#include <typeinfo>
#include <utility>

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

template<typename D, typename V>
struct SmallBufferTypeEraserCRT {
    V *value() noexcept { return static_cast<D *>(this)->template as<V>(); }

    inline const static TypeErasureOperations VTable = {
        [](void *who) noexcept { static_cast<D *>(who)->value()->~V(); },
        [](void *from, void *to) noexcept {
            new (to) D(std::move(*static_cast<D *>(from)->value()));
        }
    };

    template<typename... Args>
    void build(Args &&...args) {
        new(value()) V(std::forward<Args>(args)...);
    }
};

template<int Size, int Alignment, typename V>
struct SmallBufferTypeEraser:
    TypeErasedContainer<Size, Alignment>,
    SmallBufferTypeEraserCRT<SmallBufferTypeEraser<Size, Alignment, V>, V>
{
    using B =
        SmallBufferTypeEraserCRT<SmallBufferTypeEraser<Size, Alignment, V>, V>;

    template<typename... Args>
    SmallBufferTypeEraser(Args &&...args) {
        this->vTable_ = &B::VTable;
        this->build(std::forward<Args>(args)...);
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

namespace RTTI {

struct RTTIOperation: TypeErasureOperations {
    const std::type_info &(*type)() noexcept;
};

template<typename D>
struct VTCBase {
    const std::type_info &type() const noexcept {
        auto rtti = static_cast<const RTTIOperation *>(this->vTable_);
        return rtti->type();
    }
};

template<int S, int A>
struct TEC: TypeErasedContainer<S, A>, VTCBase<TEC<S, A>> {
    TEC() { this->vTable_ = &Defaulted; }

    using B = TypeErasedContainer<S, A>;

    inline static const RTTIOperation Defaulted = {
        B::Empty.destroy,
        B::Empty.move,
        []() noexcept -> decltype(auto) { return typeid(void); }
    };
};

template<int S, int A, typename V>
struct SBTE: SmallBufferTypeEraserCRT<SBTE<S, A, V>, V>, TEC<S, A> {
    using Base = SmallBufferTypeEraserCRT<SBTE<S, A, V>, V>;

    template<typename... Args>
    SBTE(Args &&...args): Base(std::forward<Args>(args)...) {
        this->vTable_ = &VTable;
    }

    RTTIOperation VTable = {
        Base::VTable::destroy,
        Base::VTable::move,
        []() noexcept -> decltype(auto) { return typeid(V); }
    };
};

template<int Size, int Alignment>
struct RTTI {
    using MemoryLayout = TEC<Size, Alignment>;

    template<typename T>
    using Builder = SBTE<Size, Alignment, T>;

    template<typename C>
    struct Affordances {
        const std::type_info &type() const noexcept {
            return static_cast<const C *>(this)->container()->type();
        }
    };

    constexpr static auto RequireMoveOnly = true;
};

}}

#endif /* VTable_h */
