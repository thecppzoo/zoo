#include "zoo/AlignedStorage.h"

#include <typeinfo>

namespace zoo {

struct Destroy {
    struct VTableEntry { void (*dp)(void *) noexcept; };

    static void noOp(void *) noexcept {}

    template<typename>
    constexpr static inline VTableEntry Default = { noOp };

    template<typename Container>
    constexpr static inline VTableEntry Operation = { Container::destructor };

    template<typename Container>
    struct Mixin {
        void destroy() noexcept {
            auto downcast = static_cast<Container *>(this);
            downcast->template vTable<Destroy>()->dp(downcast);
        }
    };
};

struct Move {
    struct VTableEntry { void (*mp)(void *, void *) noexcept; };

    template<typename Container>
    constexpr static inline VTableEntry Default = { Container::moveVTable };

    template<typename Container>
    constexpr static inline VTableEntry Operation = { Container::move };

    template<typename Container>
    struct Mixin {
        void move(void *to) noexcept {
            auto downcast = static_cast<Container *>(this);
            downcast->template vTable<Move>()->mp(to, downcast);
        }
    };
};

struct Copy {
    struct VTableEntry { void (*cp)(void *, const void *); };

    template<typename Container>
    constexpr static inline VTableEntry Default = { Container::copyVTable };

    template<typename Container>
    constexpr static inline VTableEntry Operation = { Container::copy };

    template<typename Container>
    struct Mixin {
        void copy(void *to) const {
            auto downcast = static_cast<const Container *>(this);
            downcast->template vTable<Copy>()->cp(to, downcast);
        }
    };
};

struct CanonicalVTable:
    Destroy::VTableEntry, Move::VTableEntry, Copy::VTableEntry {};

template<typename VirtualTable>
struct VTableHolder {
    const VirtualTable *ptr_;

    /// \brief from the vtable returns the entry corresponding to the affordance
    template<typename Affordance>
    const typename Affordance::VTableEntry *vTable() const noexcept {
        return static_cast<const typename Affordance::VTableEntry *>(ptr_);
    }

    VTableHolder(const VirtualTable *p): ptr_(p) {}
};

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

template<int S, int A, typename V>
struct BuilderDecider {
    constexpr static auto NoThrowMovable =
        std::is_nothrow_move_constructible_v<V>;
    constexpr static auto AlignmentSuitable =
        A % alignof(V) == 0;
    constexpr static auto SmallBufferSuitable =
        sizeof(V) <= S && AlignmentSuitable && NoThrowMovable;
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

template<typename... Affordances>
struct GenericPolicy {
    struct VTable: Affordances::VTableEntry... {};
    using VTHolder = VTableHolder<VTable>;
    using HoldingModel = void *;

    struct Container: VTHolder, Affordances::template Mixin<Container>... {
        AlignedStorageFor<HoldingModel> space_;

        static void  moveVTable(void *to, void *from) noexcept {
            static_cast<Container *>(to)->ptr_ =
                static_cast<Container *>(from)->ptr_;
        }

        static void copyVTable(void *to, const void *from) {
            static_cast<Container *>(to)->ptr_ =
                static_cast<const Container *>(from)->ptr_;
        }

        constexpr static inline VTable Default = {
            Affordances::template Default<Container>...
        };

        Container(): VTHolder(&Default) {}
        using VTHolder::VTHolder;
    };

    template<typename V>
    struct ByValue: Container {
        V *value() noexcept { return this->space_.template as<V>(); }
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

        constexpr static inline VTable Operations = {
            Affordances::template Operation<ByValue>...
        };

        template<typename... Args>
        ByValue(Args &&...args): Container(&Operations) {
            this->space_.template build<V>(std::forward<Args>(args)...);
        }
    };

    template<typename V>
    struct ByReference: Container {
        V *&pValue() noexcept { return *this->space_.template as<V *>(); }
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

        constexpr static inline VTable Operations = {
            Affordances::template Operation<ByReference>...
        };

        ByReference(ByReference *source): Container(&Operations) {
            pValue() = source->pValue();
        }

        template<typename... Args>
        ByReference(Args &&...args): Container(&Operations) {
            pValue() = new V(std::forward<Args>(args)...);
        }
    };

    struct Policy {
        using MemoryLayout = Container;

        template<typename V>
        using Builder =
        std::conditional_t<
            BuilderDecider<sizeof(HoldingModel), alignof(HoldingModel), V>::
                SmallBufferSuitable,
            ByValue<V>,
            ByReference<V>
        >;
    };
};

template<typename... Affordances>
using Policy = typename GenericPolicy<Affordances...>::Policy;

}
