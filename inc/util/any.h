#include "meta/NotBasedOn.h"

#ifdef MODERN_COMPILER
#include "meta/InplaceType.h"
#endif

#include <new>
#include <initializer_list>
#include <typeinfo>

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
};

template<int Size, int Alignment>
struct BaseContainer: IAnyContainer<Size, Alignment> {
    bool nonEmpty() const noexcept { return true; }
};

template<int Size, int Alignment, typename ValueType>
struct ValueContainer: BaseContainer<Size, Alignment> {
    using IAC = IAnyContainer<Size, Alignment>;

    ValueType *thy() { return reinterpret_cast<ValueType *>(this->m_space); }

    template<typename... Values>
    ValueContainer(Values &&... values) {
        new(this->m_space) ValueType{std::forward<Values>(values)...};
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
struct ReferentialContainer: IAnyContainer<Size, Alignment> {
    using IAC = IAnyContainer<Size, Alignment>;
    using NONE = void (ReferentialContainer::*)();
    constexpr static NONE None = nullptr;

    ValueType **pThy() {
        return reinterpret_cast<ValueType **>(this->m_space);
    }

    ValueType *thy() { return *pThy(); }

    template<typename... Values>
    ReferentialContainer(Values &&... values) {
        *pThy() = new ValueType{std::forward<Values>(values)...};
    }

    ReferentialContainer(NONE, ValueType *ptr) { *pThy() = ptr; }

    void destroy() override { thy()->~ValueType(); }

    void copy(IAC *to) override { new(to) ReferentialContainer{*thy()}; }

    void move(IAC *to) noexcept override {
        new(to) ReferentialContainer{None, thy()};
        *pThy() = nullptr;
    }

    void *value() noexcept override { return thy(); }

    const std::type_info &type() const noexcept override {
        return typeid(ValueType);
    }
};

template<int Size, int Alignment, typename ValueType, bool Value>
struct PolymorphicImplementationDecider {
    using type = ReferentialContainer<Size, Alignment, ValueType>;
};

template<int Size, int Alignment, typename ValueType>
struct PolymorphicImplementationDecider<Size, Alignment, ValueType, true> {
    using type = ValueContainer<Size, Alignment, ValueType>;
};

template<int Size_, int Alignment_>
struct PolymorphicTypeSwitch {
    constexpr static auto Size = Size_;
    constexpr static auto Alignment = Alignment_;

    using Switch = IAnyContainer<Size, Alignment>;

    template<typename ValueType>
    static constexpr bool useValueSemantics() {
        return
            Alignment % alignof(ValueType) == 0 &&
            sizeof(ValueType) <= Size &&
            std::is_nothrow_move_constructible<ValueType>::value;
    }

    template<typename ValueType>
    using Implementation =
        typename PolymorphicImplementationDecider<
            Size,
            Alignment,
            ValueType,
            useValueSemantics<ValueType>()
        >::type;
};

template<typename TypeSwitch>
struct AnyContainer {
    constexpr static auto Size = TypeSwitch::Size;
    constexpr static auto Alignment = TypeSwitch::Alignment;

    using Container = typename TypeSwitch::Switch;

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
                std::is_copy_constructible<Decayed>::value
                #ifdef MODERN_COMPILER
                && !meta::InplaceType<Initializer>::value
                #endif
            ,
            int
        > = 0
    >
    AnyContainer(Initializer &&initializer) {
        using Implementation =
            typename TypeSwitch::template Implementation<Decayed>;
        new(m_space) Implementation(std::forward<Initializer>(initializer));
    }

    #ifdef MODERN_COMPILER
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
        using Implementation =
            typename
                TypeSwitch::template Implementation<Decayed>;
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
        using Implementation =
            typename
                TypeSwitch::template Implementation<Decayed>;
        new(m_space) Implementation(il, std::forward<Args>(args)...);
    }
    #endif

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
        container()->destroy();
        new(this) AnyContainer(std::forward<Argument>(argument));
        return *this;
    }

    template<typename ValueType, typename... Arguments>
    std::decay_t<ValueType> &emplace(Arguments  &&...);

    template<typename ValueType, typename U, typename... Arguments>
    std::decay_t<ValueType> &
    emplace(std::initializer_list<U>, Arguments &&...);

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

    Container *container() const {
        return reinterpret_cast<Container *>(const_cast<char *>(m_space));
    }
};

template<typename TypeSwitch>
inline void anyContainerSwap(
    AnyContainer<TypeSwitch> &a1, AnyContainer<TypeSwitch> &a2
) noexcept { a1.swap(a2); }

template<typename T, typename TypeSwitch>
inline T *anyContainerCast(const AnyContainer<TypeSwitch> *ptr) {
    return
        reinterpret_cast<T *>(
            const_cast<AnyContainer<TypeSwitch> *>(ptr)->container()->value()
        );
}

using CanonicalTypeSwitch =
    PolymorphicTypeSwitch<sizeof(void *), alignof(void *)>;

using Any = AnyContainer<CanonicalTypeSwitch>;

struct bad_any_cast: std::bad_cast {
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

}
