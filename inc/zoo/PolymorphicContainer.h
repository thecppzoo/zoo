//
//  PolymorphicContainer.h
//
//  Created by Eduardo Madrid on 6/7/19.
//

#ifndef PolymorphicContainer_h
#define PolymorphicContainer_h

// clang-format off

#include "ValueContainer.h"

namespace zoo {

template<int Size, int Alignment>
struct IAnyContainer {
    const void *vtable() const noexcept { return *reinterpret_cast<const void *const *>(this); }
    virtual void destroy() noexcept {}
    virtual void copy(IAnyContainer *to) { new(to) IAnyContainer; }
    virtual void move(IAnyContainer *to) noexcept { new(to) IAnyContainer; }
    
    /// Note: it is a fatal error to call the destructor after moveAndDestroy
    virtual void moveAndDestroy(IAnyContainer *to) noexcept {
        new(to) IAnyContainer;
    }
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
struct ValueContainer:
    BaseContainer<Size, Alignment>,
    ValueContainerCRT<
        ValueContainer<Size, Alignment, ValueType>,
        IAnyContainer<Size, Alignment>,
        ValueType
    >
{
    using IAC = IAnyContainer<Size, Alignment>;
    using Base = ValueContainerCRT<
        ValueContainer<Size, Alignment, ValueType>,
        IAC,
        ValueType
    >;

    template<typename... Args>
    ValueContainer(Args &&...args) {
        this->build(std::forward<Args>(args)...);
    }

    void destroy() noexcept override { Base::destroy(); }
    
    void copy(IAC *to) override { Base::copy(to); }
    
    void move(IAC *to) noexcept override { Base::move(to); }
    
    void moveAndDestroy(IAC *to) noexcept override {
        Base::moveAndDestroy(to);
    }

    void *value() noexcept override { return Base::value(); }

    const std::type_info &type() const noexcept override {
        return typeid(ValueType);
    }

    constexpr static auto IsReference = false;
};

template<int Size, int Alignment, typename ValueType>
struct ReferentialContainer:
    BaseContainer<Size, Alignment>,
    ReferentialContainerCRT<
        ReferentialContainer<Size, Alignment, ValueType>,
        IAnyContainer<Size, Alignment>,
        ValueType
    >
{
    using IAC = IAnyContainer<Size, Alignment>;
    using Base =
        ReferentialContainerCRT<
            ReferentialContainer<Size, Alignment, ValueType>,
            IAnyContainer<Size, Alignment>,
            ValueType
        >;

    template<typename... Args>
    ReferentialContainer(Args &&...args) {
        this->build(std::forward<Args>(args)...);
    }

    void destroy() noexcept override { Base::destroy(); }
    
    void copy(IAC *to) override { Base::copy(to); }
    
    void move(IAC *to) noexcept override { Base::move(to); }
    
    void moveAndDestroy(IAC *to) noexcept override { Base::moveAndDestroy(to); }
    
    void *value() noexcept override { return Base::value(); }
    
    const std::type_info &type() const noexcept override {
        return Base::type();
    }

    constexpr static auto IsReference = true;
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

    template<typename C>
    struct Affordances {
        const std::type_info &type() const noexcept {
            return static_cast<const C *>(this)->container()->type();
        }
    };
};

}

#endif /* PolymorphicContainer_h */
