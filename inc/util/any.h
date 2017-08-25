#include "meta/NotBasedOn.h"

#ifdef MODERN_COMPILER
#include "meta/InplaceType.h"
#endif

#include <new>
#include <initializer_list>
#include <typeinfo>

namespace zoo {

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

    template<typename Value>
    ValueContainer(Value &&value) {
        new(this->m_space) ValueType{std::forward<Value>(value)};
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

    ValueType **pThy() {
        return reinterpret_cast<ValueType **>(this->m_space);
    }

    ValueType *thy() { return *pThy(); }

    template<typename Value>
    ReferentialContainer(Value &&value) {
        *pThy() = new ValueType{std::forward<Value>(value)};
    }

    ReferentialContainer(ValueType *ptr) { *pThy() = ptr; }

    void destroy() override { thy()->~ValueType(); }

    void copy(IAC *to) override { new(to) ReferentialContainer{*thy()}; }

    void move(IAC *to) noexcept override {
        new(to) ReferentialContainer{thy()};
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

struct PolymorphicTypeSwitch {
    template<int Size, int Alignment>
    using empty = IAnyContainer<Size, Alignment>;

    template<int Size, int Alignment, typename ValueType>
    static constexpr bool useValueSemantics() {
        return
            Alignment % alignof(ValueType) == 0 &&
            sizeof(ValueType) <= Size;
    }

    template<int Size, int Alignment, typename ValueType>
    using Implementation =
        typename PolymorphicImplementationDecider<
            Size,
            Alignment,
            ValueType,
            useValueSemantics<Size, Alignment, ValueType>()
        >::type;
};

template<int Size, int Alignment, typename TypeSwitch = PolymorphicTypeSwitch>
struct AnyContainer {
    using Container = typename TypeSwitch::template empty<Size, Alignment>;

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
            typename
                TypeSwitch::template Implementation<Size, Alignment, Decayed>;
        new(m_space) Implementation(std::forward<Initializer>(initializer));
    }

    #ifdef MODERN_COMPILER
    template<
        typename ValueType,
        typename... Initializers,
        typename Decayed = std::decay_t<ValueType>,
        std::enable_if_t<
            std::is_copy_constructible<Decayed>::value &&
                std::is_constructible<Decayed, Initializers...>::value
            int
        > = 0
    >
    inline
    AnyContainer(std::in_place_type_t<ValueType>, Initializers &&...izers) {
        using Implementation =
        new(m_space) Implementation(std::forward<Initializers>(izers)...);
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

template<int Size, int Alignment, typename TypeSwitch>
inline void anyContainerSwap(
    AnyContainer<Size, Alignment, TypeSwitch> &a1,
    AnyContainer<Size, Alignment, TypeSwitch> &a2
) noexcept { a1.swap(a2); }

template<typename T, int Size, int Alignment, typename TypeSwitch>
inline T *anyContainerCast(AnyContainer<Size, Alignment, TypeSwitch> *ptr) {
    return reinterpret_cast<T *>(ptr->container()->value());
}

using Any = AnyContainer<sizeof(void *), alignof(void *), PolymorphicTypeSwitch>;

template<typename T>
inline T *any_cast(Any *ptr) { return anyContainerCast<T>(ptr); }

inline void swap(Any &a1, Any &a2) noexcept { anyContainerSwap(a1, a2); }

}
