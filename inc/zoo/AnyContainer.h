#ifndef ZOO_ANYCONTAINER_H
#define ZOO_ANYCONTAINER_H

#include "zoo/Any/Traits.h"
#include "zoo/utility.h"

#include "meta/NotBasedOn.h"
#include "meta/InplaceType.h"

#include <utility>
#include <new>
#include <initializer_list>
#include <typeinfo>

namespace zoo { namespace detail {

template<typename Policy>
struct AnyContainerBase: PolicyAffordances<AnyContainerBase<Policy>, Policy> {
    using Container = typename Policy::MemoryLayout;

    alignas(alignof(Container))
    char m_space[sizeof(Container)];

    AnyContainerBase() noexcept { new(m_space) Container; }

    constexpr static void (AnyContainerBase::*NoInit)(void *) = nullptr;
    AnyContainerBase(void (AnyContainerBase::*)(void *)) {}

    AnyContainerBase(AnyContainerBase &&moveable) noexcept {
        auto source = moveable.container();
        source->move(container());
    }

    template<
        typename Initializer,
        typename Decayed = std::decay_t<Initializer>,
        std::enable_if_t<
            meta::NotBasedOn<Initializer, AnyContainerBase>() &&
                !meta::InplaceType<Initializer>::value,
        int> = 0
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
        std::enable_if_t<
                std::is_constructible<
                    Decayed, std::initializer_list<UL> &, Args...
                >::value,
            int
        > = 0
    >
    AnyContainerBase(
        std::in_place_type_t<ValueType>,
        std::initializer_list<UL> il,
        Args &&... args
    ) {
        using Implementation = typename Policy::template Builder<Decayed>;
        new(m_space) Implementation(il, std::forward<Args>(args)...);
    }

    ~AnyContainerBase() { container()->destroy(); }

    AnyContainerBase &operator=(const AnyContainerBase &model) {
        auto myself = container();
        myself->destroy();
        model.container()->copy(myself);
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
            meta::NotBasedOn<Argument, AnyContainerBase>(),
            int
        > = 0
    >
    AnyContainerBase &operator=(Argument &&argument) {
        emplace_impl<Argument>(std::forward<Argument>(argument));
        return *this;
    }

    template<typename ValueType, typename... Arguments>
    std::enable_if_t<
        std::is_constructible<std::decay_t<ValueType>, Arguments...>::value,
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
        >::value,
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

}

template<typename Policy>
struct AnyCopyable: detail::AnyContainerBase<Policy> {
    using Base = detail::AnyContainerBase<Policy>;

    using Base::Base;

    AnyCopyable(const AnyCopyable &model): Base{Base::NoInit} {
        auto thy = this->container();
        model.container()->copy(thy);
    }

    AnyCopyable(AnyCopyable &&) = default;

    template<typename Argument>
    std::enable_if_t<
        std::is_assignable_v<Base, Argument> &&
        std::is_copy_constructible_v<std::decay_t<Argument>>,
        AnyCopyable &
    > operator=(Argument &&a) noexcept(noexcept(
        std::declval<Base &>() = std::forward<Argument>(a)
    )) {
        this->Base::operator=(std::forward<Argument>(a));
        return *this;
    }

    #define BASE_EMPLACE_FORWARDING \
    std::declval<Base &>().template emplace<ValueType>(std::forward<Args>(args)...)
    template<typename ValueType, typename... Args>
    auto emplace(Args &&...args) noexcept(noexcept(BASE_EMPLACE_FORWARDING)) ->
    std::enable_if_t<
        std::is_copy_constructible_v<ValueType>,
        decltype(BASE_EMPLACE_FORWARDING)
    > {
        this->template emplace<ValueType>(std::forward<Args>()...);
    }
};

template<typename Policy>
struct AnyMovable: detail::AnyContainerBase<Policy> {
    using Base = detail::AnyContainerBase<Policy>;
    using Base::Base;

    template<typename Arg>
    constexpr static auto SuitableForBuilding() {
        return
            meta::NotBasedOn<Arg, AnyMovable>() && // disables self-building
            !std::is_lvalue_reference_v<Arg>
        ;
    }

    AnyMovable(const AnyMovable &) = delete;
    AnyMovable(AnyMovable &&) = default;

    template<
        typename Arg,
        typename = std::enable_if_t<SuitableForBuilding<Arg>()>
    >
    AnyMovable(Arg &&a): Base(std::forward<Arg>(a)) {}

    AnyMovable &operator=(const AnyMovable &) = delete;
    AnyMovable &operator=(AnyMovable &&) = default;

    template<typename Argument>
    std::enable_if_t<
        SuitableForBuilding<Argument>(),
        AnyMovable &
    > operator=(Argument &&argument) noexcept(
        noexcept(std::declval<Base &>() = std::forward<Argument>(argument))
    ) {
        Base::operator=(std::forward<Argument>(argument));
        return *this;
    }
};

template<typename Policy>
using AnyContainer = AnyCopyable<Policy>;

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
