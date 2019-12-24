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

struct RTTI {
    struct VTableEntry { const std::type_info *ti; };

    template<typename>
    constexpr static inline VTableEntry Default = { &typeid(void) };

    template<typename Container>
    constexpr static inline VTableEntry Operation = {
        &typeid(decltype(*std::declval<Container &>().value()))
    };

    template<typename Container>
    struct Mixin {
        const std::type_info &type() const noexcept {
            auto downcast = static_cast<const Container *>(this);
            return *downcast->template vTable<RTTI>()->ti;
        }
    };

    template<typename AnyC>
    struct UserAffordance {
        const std::type_info &type() const noexcept {
            return static_cast<const AnyC *>(this)->container()->type();
        }
    };
};

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

template<int S, int A, typename V>
struct BuilderDecider {
    constexpr static auto NoThrowMovable =
        std::is_nothrow_move_constructible_v<V>;
    constexpr static auto AlignmentSuitable =
        A % alignof(V) == 0;
    constexpr static auto SmallBufferSuitable =
        sizeof(V) <= S && AlignmentSuitable && NoThrowMovable;
};

template<typename... AffordanceSpecifications>
struct GenericPolicy {
    struct VTable: AffordanceSpecifications::VTableEntry... {};
    using VTHolder = VTableHolder<VTable>;
    using HoldingModel = void *;

    struct Container:
        VTHolder,
        AffordanceSpecifications::template Mixin<Container>...
    {
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
            AffordanceSpecifications::template Default<Container>...
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
            AffordanceSpecifications::template Operation<ByValue>...
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
            AffordanceSpecifications::template Operation<ByReference>...
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

template<typename PlainPolicy, typename... AffordanceSpecifications>
struct ExtendedAffordancePolicy: PlainPolicy {
    template<typename AnyC>
    struct Affordances:
        AffordanceSpecifications::template UserAffordance<AnyC>...
    {};
};

}
