#ifndef ZOO_VTABLE_POLICY_H
#define ZOO_VTABLE_POLICY_H

#include "zoo/Any/Traits.h"

#include "zoo/AlignedStorage.h"

#include <typeinfo>

#include <functional>

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

        template<typename Manager>
        bool holds() const noexcept {
            auto downcast = static_cast<const Container *>(this);
            return
                Operation<Manager>.dp ==
                    downcast->template vTable<Destroy>()->dp;
        }
    };

    template<typename AnyC>
    struct UserAffordance {
        template<typename T>
        bool holds() const noexcept {
            return
                static_cast<const AnyC *>(this)->
                    container()->
                        template holds<
                            typename AnyC::Policy::template Builder<T>
                        >();
        }

        bool isDefault() const noexcept {
            return
                noOp ==
                    static_cast<const AnyC *>(this)->
                        container()->template vTable<Destroy>()->dp;
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

        void moveAndDestroy(void *to) noexcept {
            auto downcast = static_cast<Container *>(this);
            downcast->template vTable<Move>()->mp(to, downcast);
            downcast->template vTable<Destroy>()->dp(downcast);
        }
    };

    template<typename AnyC>
    struct UserAffordance {};
};

struct Copy {
    struct VTableEntry { void (*cp)(void *, const void *); };

    template<typename Container>
    constexpr static inline VTableEntry Default = { Container::copyVTable };

    template<typename Container>
    constexpr static inline VTableEntry Operation = { Container::copyOp };

    template<typename Container>
    struct Mixin {
        void copy(void *to) const {
            auto downcast = static_cast<const Container *>(this);
            downcast->template vTable<Copy>()->cp(to, downcast);
        }
    };

    template<typename TypeSwitch, typename MemoryLayout>
    struct Raw {
        static void copy(void *to, const void *from) {
            auto downcast = static_cast<const MemoryLayout *>(from);
            auto vtable = downcast->pointer();
            auto entry = static_cast<const TypeSwitch *>(vtable);
            entry->cp(to, downcast);
        }
    };

    template<typename>
    struct UserAffordance {};
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

        bool nonEmpty() const noexcept {
            return typeid(void) != type();
        }
    };

    template<typename TypeSwitch, typename MemoryLayout>
    struct Raw {
        static const std::type_info &typeId(const void *from) {
            auto downcast = static_cast<const MemoryLayout *>(from);
            auto typeSwitch =
                static_cast<const TypeSwitch *>(downcast->pointer());
            auto entry = static_cast<const VTableEntry *>(typeSwitch);
            return *entry->ti;
        }
    };

    template<typename AnyC>
    struct UserAffordance {
        const std::type_info &type() const noexcept {
            return static_cast<const AnyC *>(this)->container()->type();
        }

        const std::type_info &type2() const noexcept {
            auto thy = static_cast<const AnyC *>(this);
            return AnyC::Policy::ExtraAffordances::typeId(thy->container());
        }
    };
};

template<typename>
struct CallableViaVTable;

template<typename R, typename... As>
struct CallableViaVTable<R(As...)> {
    static R throwStdBadFunctionCall(As..., void *) {
        throw std::bad_function_call{};
    }

    template<typename MDC>
    static R invoke(As... arguments, void *pc) {
        return (*static_cast<MDC *>(pc)->value())(std::forward<As>(arguments)...);
    }

    struct VTableEntry {
        R (*executor_)(As..., void *);
    };

    template<typename>
    constexpr static inline VTableEntry Default = { throwStdBadFunctionCall };

    template<typename MostDerivedContainer>
    constexpr static inline VTableEntry Operation = { invoke<MostDerivedContainer> };

    template<typename Container>
    struct Mixin {};

    template<typename AnyC>
    struct UserAffordance {
        R operator()(As... args) {
            auto container = static_cast<AnyC *>(this)->container();
            auto vTP = container->template vTable<CallableViaVTable>();
            return vTP->executor_(std::forward<As>(args)..., container);
        }
    };
};

template<typename VirtualTable>
struct VTableHolder {
    const VirtualTable *pointer_;

    /// \brief from the vtable returns the entry corresponding to the affordance
    template<typename Affordance>
    const typename Affordance::VTableEntry *vTable() const noexcept {
        return static_cast<const typename Affordance::VTableEntry *>(pointer_);
    }

    VTableHolder(const VirtualTable *p): pointer_(p) {}

    auto pointer() const noexcept { return pointer_; }

    void change(const VirtualTable *p) noexcept { pointer_ = p; }
};

template<std::size_t S, std::size_t A, typename V>
struct BuilderDecider {
    constexpr static auto NoThrowMovable =
        std::is_nothrow_move_constructible_v<V>;
    constexpr static auto AlignmentSuitable =
        A % alignof(V) == 0;
    constexpr static auto SmallBufferSuitable =
        sizeof(V) <= S && AlignmentSuitable && NoThrowMovable;
};

// Do not define, this is to capture packs of types
template<typename...>
struct CompatibilityParameters;

template<typename HoldingModel, typename... AffordanceSpecifications>
struct GenericPolicy {
    struct VTable: AffordanceSpecifications::VTableEntry... {};

    using VTHolder = VTableHolder<VTable>;

    struct Container:
        VTHolder,
        AffordanceSpecifications::template Mixin<Container>...
    {
        AlignedStorageFor<HoldingModel> space_;

        static void  moveVTable(void *to, void *from) noexcept {
            static_cast<Container *>(to)->change(
                static_cast<Container *>(from)->pointer()
            );
        }

        static void copyVTable(void *to, const void *from) {
            static_cast<Container *>(to)->change(
                static_cast<const Container *>(from)->pointer()
            );
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

        static void copyOp(void *to, const void *from) {
            auto downcast = static_cast<const ByValue *>(from);
            new(to) ByValue(*downcast->value());
        }

        constexpr static inline VTable Operations = {
            AffordanceSpecifications::template Operation<ByValue>...
        };

        template<typename... Args>
        ByValue(Args &&...args):
            ByValue(&Operations, std::forward<Args>(args)...)
        {}

        template<typename... Args>
        ByValue(const VTable *ops, Args &&...args):
            Container(ops)
        {
            this->space_.template build<V>(std::forward<Args>(args)...);
        }

        constexpr static auto IsReference = std::false_type::value;
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

        static void copyOp(void *to, const void *from) {
            auto downcast = static_cast<const ByReference *>(from);
            new(to) ByReference(*downcast->value());
        }

        constexpr static inline VTable Operations = {
            AffordanceSpecifications::template Operation<ByReference>...
        };

        ByReference(ByReference *source): Container(source->pointer()) {
            pValue() = source->pValue();
        }

        template<typename... Args>
        ByReference(const VTable *ops, Args &&...args): Container(ops) {
            pValue() = new V(std::forward<Args>(args)...);
        }

        template<typename... Args>
        ByReference(Args &&...args):
            ByReference(&Operations, std::forward<Args>(args)...)
        {}

        constexpr static auto IsReference = std::true_type::value;
    };

    struct Policy {
        using MemoryLayout = Container;

        using DefaultImplementation = Container;

        template<typename V>
        using Builder =
            std::conditional_t<
                BuilderDecider<sizeof(HoldingModel), alignof(HoldingModel), V>::
                    SmallBufferSuitable,
                ByValue<V>,
                ByReference<V>
            >;

        constexpr static auto
            Size = sizeof(HoldingModel),
            Alignment = alignof(HoldingModel);

        using VTable = GenericPolicy::VTable;

        template<typename AnyC>
        struct Affordances: AffordanceSpecifications::template UserAffordance<AnyC>... {};
    };
};

template<typename HoldingModel, typename... Affordances>
using Policy = typename GenericPolicy<HoldingModel, Affordances...>::Policy;

template<typename PlainPolicy, typename... AffordanceSpecifications>
struct ExtendedAffordancePolicy: PlainPolicy {
    template<typename AnyC>
    struct Affordances:
        AffordanceSpecifications::template UserAffordance<AnyC>...
    {};
};

}

#endif
