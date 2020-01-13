#ifndef ZOO_ANYCONTAINER_H
#define ZOO_ANYCONTAINER_H

#include "zoo/Any/Traits.h"
#include "zoo/utility.h"

#include "zoo/meta/NotBasedOn.h"
#include "zoo/meta/InplaceType.h"
#include "zoo/meta/copy_and_move_abilities.h"

namespace zoo {

namespace impl {

template<auto Value, typename... Options>
struct CorrectType:
    std::disjunction<std::is_same<decltype(Value), Options>...>
{};

template<typename T, typename = void>
struct HasCopy: std::false_type {};

template<typename T>
struct HasCopy<
        // copy can be called on a T & with a T *
    T, std::void_t<decltype(std::declval<T &>().copy(std::declval<T *>()))>
>: std::true_type {};

}

template<typename Policy_>
struct AnyContainerBase:
    detail::PolicyAffordances<AnyContainerBase<Policy_>, Policy_>
{
    using Policy = Policy_;
    using Container = typename Policy::MemoryLayout;
    constexpr static auto Copyable = impl::HasCopy<Container>::value;

    alignas(alignof(Container))
    char m_space[sizeof(Container)];

    AnyContainerBase() noexcept { new(m_space) Container; }

    AnyContainerBase(const AnyContainerBase &model) {
        auto source = model.container();
        source->copy(container());
    }

    AnyContainerBase(AnyContainerBase &&moveable) noexcept {
        auto source = moveable.container();
        source->move(container());
    }

    template<
        typename Initializer,
        typename Decayed = std::decay_t<Initializer>,
        std::enable_if_t<
            !detail::IsAnyContainer_impl<Decayed>::value &&
                (!Copyable || std::is_copy_constructible_v<Decayed>) &&
                !meta::InplaceType<Initializer>::value,
            int
        > = 0
    >
    AnyContainerBase(Initializer &&initializer) {
        using Implementation = typename Policy::template Builder<Decayed>;
        new(m_space) Implementation(std::forward<Initializer>(initializer));
    }

    template<
        typename ValueType,
        typename... Initializers,
        typename Decayed = std::decay_t<ValueType>,
        std::enable_if_t<
            (!Copyable || std::is_copy_constructible<Decayed>::value) &&
                std::is_constructible<Decayed, Initializers...>::value,
            int
        > = 0
    >
    AnyContainerBase(std::in_place_type_t<ValueType>, Initializers &&...izers) {
        using Implementation = typename Policy::template Builder<Decayed>;
        new(m_space) Implementation(std::forward<Initializers>(izers)...);
    }

    template<
        typename ValueType,
        typename UL,
        typename... Args,
        typename Decayed = std::decay_t<ValueType>,
        typename = std::enable_if_t<
            (!Copyable || std::is_copy_constructible<Decayed>::value) &&
                std::is_constructible<
                    Decayed, std::initializer_list<UL> &, Args...
                >::value
            >
    >
    AnyContainerBase(
        std::in_place_type_t<ValueType>,
        std::initializer_list<UL> il,
        Args &&... args
    ) {
        using Implementation = typename Policy::template Builder<Decayed>;
        new(m_space) Implementation(il, std::forward<Args>(args)...);
    }

    template<
        typename OtherPolicy,
        int = std::enable_if_t<detail::CompatiblePolicy_v<Policy, OtherPolicy>, int>(0)
    >
    AnyContainerBase(const AnyContainerBase<OtherPolicy> &);

    ~AnyContainerBase() { container()->destroy(); }

    AnyContainerBase &operator=(const AnyContainerBase &model) {
        auto myself = container();
        myself->destroy();
        try {
            model.container()->copy(myself);
        } catch(...) {
            new(m_space) Container;
            throw;
        }
        return *this;
    }

    AnyContainerBase &operator=(AnyContainerBase &&moveable) noexcept {
        auto myself = container();
        myself->destroy();
        moveable.container()->move(myself);
        return *this;
    }

    template<
        typename Argument,
        std::enable_if_t<
            !detail::IsAnyContainer_impl<std::decay_t<Argument>>::value,
            int
        > = 0
    >
    AnyContainerBase &operator=(Argument &&argument) {
        emplace_impl<Argument>(std::forward<Argument>(argument));
        return *this;
    }

    template<typename ValueType, typename... Arguments>
    std::enable_if_t<
        std::is_constructible<std::decay_t<ValueType>, Arguments...>::value &&
            std::is_copy_constructible<std::decay_t<ValueType>>::value,
        ValueType &
    >
    emplace(Arguments  &&... arguments) {
        emplace_impl<ValueType>(std::forward<Arguments>(arguments)...);
        return *state<std::decay_t<ValueType>>();
    }

    template<typename ValueType, typename U, typename... Arguments>
    std::enable_if_t<
        std::is_constructible<
            std::decay_t<ValueType>,
            std::initializer_list<U> &,
            Arguments...
        >::value &&
            std::is_copy_constructible<std::decay_t<ValueType>>::value,
        ValueType &
    >
    emplace(std::initializer_list<U> il, Arguments &&... args) {
        emplace_impl(il, std::forward<Arguments>(args)...);
        return *state<std::decay_t<ValueType>>();
    }

protected:
    template<typename ValueType, typename... Arguments>
    void emplace_impl(Arguments  &&... arguments) {
        container()->destroy();
        new(this) AnyContainerBase(
            std::in_place_type<std::decay_t<ValueType>>,
            std::forward<Arguments>(arguments)...
        );
    }

    template<typename ValueType, typename U, typename... Arguments>
    void emplace_impl(std::initializer_list<U> &il, Arguments &&... args) {
        container()->destroy();
        new(this) AnyContainerBase(
            std::in_place_type<std::decay_t<ValueType>>,
            il,
            std::forward<Arguments>(args)...
        );
    }

public:
    void reset() noexcept {
        container()->destroy();
        new(this) AnyContainerBase;
    }

    void swap(AnyContainerBase &other) noexcept {
        auto oc = other.container();

        alignas(alignof(Container))
        char tmp[sizeof(Container)];

        auto tc = reinterpret_cast<Container *>(tmp);
        oc->moveAndDestroy(tc); // note: invalidates pointer tc
        auto myself = container();
        myself->moveAndDestroy(oc);
        tc = reinterpret_cast<Container *>(tmp); // because it was invalidated
        tc->moveAndDestroy(myself);
    }

    bool has_value() const noexcept { return container()->nonEmpty(); }

    template<typename ValueType>
    auto *state() noexcept {
        using Decayed = std::decay_t<ValueType>;
        using Implementation = typename Policy::template Builder<Decayed>;
        return
            static_cast<Decayed *>(
                static_cast<Implementation *>(container())->
                    Implementation::value()
                    // the full qualification of \c value is used to disable
                    // runtime polymorphism in case it is a virtual override
            );
    }

    template<typename ValueType>
    const ValueType *state() const noexcept {
        return const_cast<AnyContainerBase *>(this)->state<ValueType>();
    }

    Container *container() const {
        return reinterpret_cast<Container *>(const_cast<char *>(m_space));
    }
};

template<typename Policy>
struct AnyContainer:
    AnyContainerBase<Policy>,
    meta::CopyAndMoveAbilities<
        impl::HasCopy<typename Policy::MemoryLayout>::value
    >
{
    using Base = AnyContainerBase<Policy>;
    using Base::AnyContainerBase;
    using Base::operator=;
};

template<typename Policy>
inline
void swap(AnyContainer<Policy> &a1, AnyContainer<Policy> &a2) {
    a1.swap(a2);
}

template<typename T, typename Policy>
inline
T *anyContainerCast(const AnyContainer<Policy> *ptr) noexcept {
    return const_cast<T *>(ptr->template state<T>());
}

}

#endif
