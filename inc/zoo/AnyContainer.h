#ifndef ZOO_ANYCONTAINER_H
#define ZOO_ANYCONTAINER_H

#include "zoo/pp/platform.h"
#include "zoo/Any/Traits.h"
#include "zoo/utility.h"

#include "zoo/meta/NotBasedOn.h"
#include "zoo/meta/InplaceType.h"
#include "zoo/meta/copy_and_move_abilities.h"

namespace zoo {

namespace detail {

/// \note the following type is needed only for legacy reasons, old policies
/// did not differentiate between the memory layout and the default builder
template<typename P, typename = void>
struct PolicyDefaultBuilder {
    using type = typename P::MemoryLayout;
};
template<typename P>
struct PolicyDefaultBuilder<P, std::void_t<typename P::DefaultImplementation>> {
    using type = typename P::DefaultImplementation;
};

template<typename, typename = void>
struct MemoryLayoutHasCopy: std::false_type {};
template<typename Policy>
struct MemoryLayoutHasCopy<
    Policy,
    std::void_t<decltype(&Policy::MemoryLayout::copy)>
>: std::true_type {};

template<typename, typename = void>
struct ExtraAffordanceOfCopying: std::false_type {};
template<typename Policy>
struct ExtraAffordanceOfCopying<
    Policy,
    std::void_t<decltype(&Policy::ExtraAffordances::copy)>
>: std::true_type {};

/// Copy constructibility and assignability are fundamental operations that
/// can not be enabled/disabled with SFINAE, this trait is to detect copyability
/// in a policy
template<typename Policy>
using AffordsCopying =
    std::disjunction<
        MemoryLayoutHasCopy<Policy>, ExtraAffordanceOfCopying<Policy>
    >;

}

template<typename Policy, typename = void>
struct CompositionChain {
    struct Base {
        using TokenType = void (Base::*)();
        constexpr static TokenType Token = nullptr;

        Base(TokenType) noexcept {}
        Base(TokenType, std::nullptr_t) noexcept {}
        Base(TokenType, Base &&) noexcept {}
        Base(TokenType, const Base &) {}

        // by default, move-only
        Base(const Base &) = delete;

        Base &operator=(Base &&) noexcept { return *this; }
        Base &operator=(const Base &) { return *this; }

        template<typename T>
        void emplaced(T *) noexcept {}

        void copy_assign(const Base &) {}
        alignas(alignof(typename Policy::MemoryLayout))
        char m_space[sizeof(typename Policy::MemoryLayout)];
    };
};

template<typename Policy>
struct CompositionChain<Policy, std::void_t<typename Policy::Base>> {
    using Base = typename Policy::Base;
};

template<typename Policy_>
struct MSVC_EMPTY_BASES AnyContainerBase:
    CompositionChain<Policy_>::Base,
    detail::PolicyAffordances<AnyContainerBase<Policy_>, Policy_>
{
    using Policy = Policy_;
    using SuperContainer = typename CompositionChain<Policy>::Base;
    using Container = typename Policy::MemoryLayout;
    constexpr static auto Copyable = detail::AffordsCopying<Policy>::value;

    AnyContainerBase() noexcept:
        SuperContainer(SuperContainer::Token, nullptr)
    {
        new(this->m_space) typename detail::PolicyDefaultBuilder<Policy>::type;
    }

    AnyContainerBase(AnyContainerBase &&moveable) noexcept:
        SuperContainer(
            SuperContainer::Token,
            static_cast<SuperContainer &&>(moveable)
        )
    {
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
    AnyContainerBase(Initializer &&initializer):
        SuperContainer(SuperContainer::Token)
    {
        using Implementation = typename Policy::template Builder<Decayed>;
        new(this->m_space) Implementation(std::forward<Initializer>(initializer));
        SuperContainer::emplaced(state<Decayed>());
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
    AnyContainerBase(std::in_place_type_t<ValueType>, Initializers &&...izers): SuperContainer(SuperContainer::Token)  {
        using Implementation = typename Policy::template Builder<Decayed>;
        new(this->m_space) Implementation(std::forward<Initializers>(izers)...);
        SuperContainer::emplaced(state<std::decay_t<ValueType>>());
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
    ): SuperContainer(SuperContainer::Token) {
        using Implementation = typename Policy::template Builder<Decayed>;
        new(this->m_space) Implementation(il, std::forward<Args>(args)...);
        SuperContainer::emplaced(state<std::decay_t<ValueType>>());
    }

    ~AnyContainerBase() { container()->destroy(); }

    AnyContainerBase &operator=(AnyContainerBase &&moveable) noexcept {
        static_cast<SuperContainer &>(*this) =
            //static_cast<SuperContainer &&>(moveable);
            std::move(static_cast<SuperContainer &>(moveable));
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
        SuperContainer::emplaced(state<std::decay_t<ValueType>>());
    }

    template<typename ValueType, typename U, typename... Arguments>
    void emplace_impl(std::initializer_list<U> &il, Arguments &&... args) {
        container()->destroy();
        new(this) AnyContainerBase(
            std::in_place_type<std::decay_t<ValueType>>,
            il,
            std::forward<Arguments>(args)...
        );
        SuperContainer::emplaced(state<std::decay_t<ValueType>>());
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
        return reinterpret_cast<Container *>(const_cast<char *>(this->m_space));
    }

protected:
    using TokenType = void (AnyContainerBase::*)();
    constexpr static TokenType Token = nullptr;

    AnyContainerBase(TokenType) noexcept:
        SuperContainer(SuperContainer::Token)
    {}
    AnyContainerBase(TokenType, std::nullptr_t) noexcept:
        SuperContainer(SuperContainer::Token, nullptr)
    {}
    AnyContainerBase(TokenType, AnyContainerBase &&source) noexcept:
        SuperContainer(SuperContainer::Token, std::move(source))
    {}
    AnyContainerBase(TokenType, const AnyContainerBase &model):
        SuperContainer(SuperContainer::Token, model)
    {}

    template<typename ValueType>
    void emplaced(ValueType *ptr) noexcept { SuperContainer::emplaced(ptr); }

    AnyContainerBase &copy_assign(const AnyContainerBase &model) {
        SuperContainer::copy_assign(model);
        return *this;
    }
};

template<typename Policy>
struct AnyCopyable: AnyContainerBase<Policy> {
    using Base = AnyContainerBase<Policy>;

    AnyCopyable(AnyCopyable &&) = default;

    AnyCopyable(const AnyCopyable &model):
        Base(Base::Token, model)
    {
        auto source = model.container();
        if constexpr(detail::MemoryLayoutHasCopy<Policy>::value) {
            source->copy(this->container());
        } else {
            static_assert(detail::ExtraAffordanceOfCopying<Policy>::value);
            Policy::ExtraAffordances::copy(this->container(), source);
        }
    }

    AnyCopyable &operator=(AnyCopyable &&) = default;

    /// \note Does not provide the strong exception safety guarantee
    // Affordance of non-destructive copying?
    AnyCopyable &operator=(const AnyCopyable &model) {
        if constexpr(std::is_copy_constructible_v<Base>)this->copy_assign(model);
        auto myself = this->container();
        myself->destroy();
        try {
            auto source = model.container();
            if constexpr(detail::MemoryLayoutHasCopy<Policy>::value) {
                source->copy(this->container());
            } else {
                static_assert(detail::ExtraAffordanceOfCopying<Policy>::value);
                Policy::ExtraAffordances::copy(this->container(), source);
            }
        } catch(...) {
            new(this->m_space) typename Base::Container;
            throw;
        }
        return *this;
    }

    using Base::Base;
    using Base::operator=;
};

template<typename Policy>
        #define PP_BASE_TYPE \
            std::conditional_t< \
                detail::AffordsCopying<Policy>::value, \
                AnyCopyable<Policy>, \
                AnyContainerBase<Policy> \
            >
struct AnyContainer: PP_BASE_TYPE
{
    using Base = typename PP_BASE_TYPE;
        #undef PP_BASE_TYPE
    using Base::Base;
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
