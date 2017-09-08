#pragma once

#include "meta/NotBasedOn.h"

#ifdef MODERN_COMPILER
#include <utility>
#else
namespace std {

template<typename T>
struct in_place_type_t {};

template<typename T>
in_place_type_t<T> in_place_type;

}
#endif
#include "meta/InplaceType.h"

#ifndef NO_STANDARD_INCLUDES
#include <new>
#include <initializer_list>
#include <typeinfo>
#endif

namespace zoo {

template<typename T>
using uncvr_t = std::remove_cv_t<std::remove_reference_t<T>>;

template<int Size, int Alignment>
struct IAnyContainer {
    virtual void destroy() {}
    virtual void copy(IAnyContainer *to) { new(to) IAnyContainer; }
    virtual void move(IAnyContainer *to) noexcept { new(to) IAnyContainer; }
    virtual void *value() noexcept { return nullptr; }
    virtual bool nonEmpty() const noexcept { return false; }
    virtual const std::type_info &type() const noexcept { return typeid(void); }

    alignas(Alignment)
    char m_space[Size];

    using NONE = void (IAnyContainer::*)();
    constexpr static NONE None = nullptr;
};

template<int Size, int Alignment>
struct BaseContainer: IAnyContainer<Size, Alignment> {
    bool nonEmpty() const noexcept { return true; }
};

template<int Size, int Alignment, typename ValueType>
struct ValueContainer: BaseContainer<Size, Alignment> {
    using IAC = IAnyContainer<Size, Alignment>;

    ValueType *thy() { return reinterpret_cast<ValueType *>(this->m_space); }

    ValueContainer(typename IAC::NONE) {}

    template<typename... Args>
    ValueContainer(Args &&... args) {
        new(this->m_space) ValueType{std::forward<Args>(args)...};
    }

    void destroy() override { thy()->~ValueType(); }

    void copy(IAC *to) override { new(to) ValueContainer{*thy()}; }

    void move(IAC *to) noexcept override {
        new(to) ValueContainer{std::move(*thy())};
    }

    void *value() noexcept override { return thy(); }

    const std::type_info &type() const noexcept override {
        return typeid(ValueType);
    }
};

template<int Size, int Alignment, typename ValueType>
struct ReferentialContainer: BaseContainer<Size, Alignment> {
    using IAC = IAnyContainer<Size, Alignment>;

    ValueType **pThy() {
        return reinterpret_cast<ValueType **>(this->m_space);
    }

    ValueType *thy() { return *pThy(); }

    ReferentialContainer(typename IAC::NONE) {}

    template<typename... Values>
    ReferentialContainer(Values &&... values) {
        *pThy() = new ValueType{std::forward<Values>(values)...};
    }

    ReferentialContainer(typename IAC::NONE, ValueType *ptr) { *pThy() = ptr; }

    void destroy() override { delete thy(); }

    void copy(IAC *to) override { new(to) ReferentialContainer{*thy()}; }

    void move(IAC *to) noexcept override {
        new(to) ReferentialContainer{IAC::None, thy()};
        new(this) IAnyContainer<Size, Alignment>;
    }

    void *value() noexcept override { return thy(); }

    const std::type_info &type() const noexcept override {
        return typeid(ValueType);
    }
};

template<int Size, int Alignment, typename ValueType, bool Value>
struct RuntimePolymorphicAnyPolicyDecider {
    using type = ReferentialContainer<Size, Alignment, ValueType>;
};

template<int Size, int Alignment, typename ValueType>
struct RuntimePolymorphicAnyPolicyDecider<Size, Alignment, ValueType, true> {
    using type = ValueContainer<Size, Alignment, ValueType>;
};

template<typename ValueType>
constexpr bool canUseValueSemantics(int size, int alignment) {
    return
        alignment % alignof(ValueType) == 0 &&
        sizeof(ValueType) <= size &&
        std::is_nothrow_move_constructible<ValueType>::value;
}

template<int Size, int Alignment>
struct RuntimePolymorphicAnyPolicy {
    using MemoryLayout = IAnyContainer<Size, Alignment>;

    template<typename ValueType>
    using Builder =
        typename RuntimePolymorphicAnyPolicyDecider<
            Size,
            Alignment,
            ValueType,
            canUseValueSemantics<ValueType>(Size, Alignment)
        >::type;
};

template<typename Policy>
struct AnyContainer {
    using Container = typename Policy::MemoryLayout;

    alignas(alignof(Container))
    char m_space[sizeof(Container)];

    AnyContainer() noexcept { new(m_space) Container; }

    AnyContainer(const AnyContainer &model) {
        auto source = model.container();
        source->copy(container());
    }

    AnyContainer(AnyContainer &&moveable) noexcept {
        auto source = moveable.container();
        source->move(container());
    }

    template<
        typename Initializer,
        typename Decayed = std::decay_t<Initializer>,
        std::enable_if_t<
            meta::NotBasedOn<Initializer, AnyContainer>() &&
                std::is_copy_constructible<Decayed>::value &&
                !meta::InplaceType<Initializer>::value,
        int> = 0
    >
    AnyContainer(Initializer &&initializer) {
        using Implementation = typename Policy::template Builder<Decayed>;
        new(m_space) Implementation(std::forward<Initializer>(initializer));
    }

    template<
        typename ValueType,
        typename... Initializers,
        typename Decayed = std::decay_t<ValueType>,
        std::enable_if_t<
            std::is_copy_constructible<Decayed>::value &&
                std::is_constructible<Decayed, Initializers...>::value,
            int
        > = 0
    >
    AnyContainer(std::in_place_type_t<ValueType>, Initializers &&...izers) {
        using Implementation = typename Policy::template Builder<Decayed>;
        new(m_space) Implementation(std::forward<Initializers>(izers)...);
    }

    template<
        typename ValueType,
        typename UL,
        typename... Args,
        typename Decayed = std::decay_t<ValueType>,
        std::enable_if_t<
            std::is_copy_constructible<Decayed>::value &&
                std::is_constructible<
                    Decayed, std::initializer_list<UL> &, Args...
                >::value,
            int
        > = 0
    >
    AnyContainer(
        std::in_place_type_t<ValueType>,
        std::initializer_list<UL> il,
        Args &&... args
    ) {
        using Implementation = typename Policy::template Builder<Decayed>;
        new(m_space) Implementation(il, std::forward<Args>(args)...);
    }

    ~AnyContainer() { container()->destroy(); }

    AnyContainer &operator=(const AnyContainer &model) {
        auto myself = container();
        myself->destroy();
        model.container()->copy(myself);
        return *this;
    }

    AnyContainer &operator=(AnyContainer &&moveable) noexcept {
        auto myself = container();
        myself->destroy();
        moveable.container()->move(myself);
        return *this;
    }

    template<
        typename Argument,
        std::enable_if_t<
            meta::NotBasedOn<Argument, AnyContainer>(),
            int
        > = 0
    >
    AnyContainer &operator=(Argument &&argument) {
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
        new(this) AnyContainer(
            std::in_place_type<std::decay_t<ValueType>>,
            std::forward<Arguments>(arguments)...
        );
    }

    template<typename ValueType, typename U, typename... Arguments>
    void emplace_impl(std::initializer_list<U> &il, Arguments &&... args) {
        container()->destroy();
        new(this) AnyContainer(
            std::in_place_type<std::decay_t<ValueType>>,
            il,
            std::forward<Arguments>(args)...
        );
    }

public:
    void reset() noexcept {
        container()->destroy();
        new(this) AnyContainer;
    }

    void swap(AnyContainer &other) noexcept {
        AnyContainer auxiliar{std::move(other)};
        other = std::move(*this);
        *this = std::move(auxiliar);
    }

    bool has_value() const noexcept { return container()->nonEmpty(); }

    const std::type_info &type() const noexcept {
        return container()->type();
    }

    template<typename ValueType>
    ValueType *state() noexcept {
        using Decayed = std::decay_t<ValueType>;
        using Implementation = typename Policy::template Builder<Decayed>;
        return reinterpret_cast<ValueType *>(
            static_cast<Implementation *>(container())->Implementation::value()
        );
    }

    template<typename ValueType>
    const ValueType *state() const noexcept {
        return const_cast<AnyContainer *>(this)->state<ValueType>();
    }

    Container *container() const {
        return reinterpret_cast<Container *>(const_cast<char *>(m_space));
    }
};

template<typename Policy>
inline void anyContainerSwap(
    AnyContainer<Policy> &a1, AnyContainer<Policy> &a2
) noexcept { a1.swap(a2); }

template<typename T, typename Policy>
inline T *anyContainerCast(const AnyContainer<Policy> *ptr) noexcept {
    return const_cast<T *>(ptr->template state<T>());
}

using CanonicalPolicy =
    RuntimePolymorphicAnyPolicy<sizeof(void *), alignof(void *)>;

using Any = AnyContainer<CanonicalPolicy>;

struct bad_any_cast: std::bad_cast {
    using std::bad_cast::bad_cast;

    const char *what() const noexcept override {
        return "Incorrect Any casting";
    }
};

template<class ValueType>
ValueType any_cast(const Any &operand) {
    using U = uncvr_t<ValueType>;
    static_assert(std::is_constructible<ValueType, const U &>::value, "");
    if(typeid(U) != operand.type()) { throw bad_any_cast{}; }
    return *anyContainerCast<U>(&operand);
}

template<class ValueType>
ValueType any_cast(Any &operand) {
    using U = uncvr_t<ValueType>;
    static_assert(std::is_constructible<ValueType, U &>::value, "");
    if(typeid(U) != operand.type()) { throw bad_any_cast{}; }
    return *anyContainerCast<U>(&operand);
}

template<class ValueType>
ValueType any_cast(Any &&operand) {
    using U = uncvr_t<ValueType>;
    static_assert(std::is_constructible<ValueType, U>::value, "");
    if(typeid(U) != operand.type()) { throw bad_any_cast{}; }
    return std::move(*anyContainerCast<U>(&operand));
}

template<class ValueType>
ValueType *any_cast(Any *operand) {
    if(!operand) { return nullptr; }
    using U = uncvr_t<ValueType>;
    if(typeid(U) != operand->type()) { return nullptr; }
    return anyContainerCast<U>(operand);
}

template<typename ValueType>
const ValueType *any_cast(const Any *ptr) {
    return any_cast<ValueType>(const_cast<Any *>(ptr));
}

inline void swap(Any &a1, Any &a2) noexcept { anyContainerSwap(a1, a2); }

template<typename T, typename... Args>
Any make_any(Args &&... args) {
    return Any(std::in_place_type<T>, std::forward<Args>(args)...);
}

}

/// \todo guarantee alignment new, tests
