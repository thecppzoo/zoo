#pragma once

#include "zoo/any.h"

#ifndef ANNOTATE_MAKE_REFERENTIAL
#define ANNOTATE_MAKE_REFERENTIAL(...)
#endif

namespace zoo {

struct ConverterDriver {
    virtual void destroy(void *) {}

    virtual void copy(ConverterDriver *dd, void *spc, const void *source) {
        new(dd) ConverterDriver;
    }

    virtual void move(ConverterDriver *dd, void *spc, void *source) {
        new(dd) ConverterDriver;
    }

    virtual void *value(void *source) { return nullptr; }

    virtual const std::type_info &type() { return typeid(void); }

    virtual void copyConvert(
        ConverterDriver *dd, void *spc, const void *source,
        int destiantionSize, int destinationAlignment
    ) {
        new(dd) ConverterDriver;
    }
};

template<typename ValueType, typename CRTP>
struct ConverterDriverCommon: ConverterDriver {
    ValueType *thy(const void *src) {
        return reinterpret_cast<ValueType *>(
            CRTP::val(const_cast<void *>(src))
        );
    }

    void destroy(void *ptr) override {
        thy(ptr)->~ValueType();
        CRTP::recclaim(ptr);
    }

    void copy(ConverterDriver *dd, void *spc, const void *source) override {
        CRTP::make(spc);
        try {
            new(CRTP::val(spc)) ValueType{*thy(source)};
        } catch(...) {
            CRTP::recclaim(spc);
            throw;
        }
        new(dd) CRTP;
    }

    void *value(void *spc) override { return thy(spc); }

    const std::type_info &type() override { return typeid(ValueType); }

    void copyConvert(
        ConverterDriver *dd, void *spc, const void *source,
        int destinationSize, int destinationAlignment
    ) override;
};

template<typename ValueType>
struct ConverterValueDriver:
    ConverterDriverCommon<ValueType, ConverterValueDriver<ValueType>>
{
    static void make(void *) {}

    static void recclaim(void *) {}

    static void *val(void *arg) { return arg; }

    void move(ConverterDriver *dd, void *spc, void *source) override {
        new(spc) ValueType{std::move(*this->thy(source))};
        new(dd) ConverterValueDriver;
    }
};

template<typename ValueType>
struct ConverterReferentialDriver:
    ConverterDriverCommon<ValueType, ConverterReferentialDriver<ValueType>>
{
    struct alignas(alignof(ValueType)) AlignedSpace {
        char spc[sizeof(ValueType)];
    };

    static void *&valPtr(void *ptr) {
        return *reinterpret_cast<void **>(ptr);
    }

    static void make(void *ptr) { valPtr(ptr) = new AlignedSpace; }

    static void recclaim(void *ptr) {
        delete reinterpret_cast<AlignedSpace *>(valPtr(ptr));
    }

    static void *val(void *arg) { return valPtr(arg); }

    void move(ConverterDriver *dd, void *spc, void *source) override {
        new(dd) ConverterReferentialDriver;
        valPtr(spc) = valPtr(source);
        valPtr(source) = nullptr;
    }
};

template<typename ValueType, typename... Args>
void makeReferential(void *dd, void *space, Args &&... args) {
    using Reference = ConverterReferentialDriver<ValueType>;
    Reference::make(space);
    try {
        auto where = Reference::val(space);
        ANNOTATE_MAKE_REFERENTIAL(
            static_cast<Reference *>(dd), space, *(void **)space
        )
        new(where) ValueType{std::forward<Args>(args)...};
    } catch(...) {
        Reference::recclaim(space);
        throw;
    } 
    new(dd) Reference;
}

template<typename ValueType, typename CRTP>
void ConverterDriverCommon<ValueType, CRTP>::copyConvert(
    ConverterDriver *dd, void *spc, const void *source,
    int destinationSize, int destinationAlignment
) {
    auto modifiable = const_cast<void *>(source);
    auto sourceValue = reinterpret_cast<ValueType *>(CRTP::val(modifiable));
    auto value = const_cast<const ValueType *>(sourceValue);
   
    if(canUseValueSemantics<ValueType>(destinationSize, destinationAlignment)) {
        new(spc) ValueType{*value};
        new(dd) ConverterValueDriver<ValueType>;
    } else {
        makeReferential<ValueType>(dd, spc, *value);
    }
}

template<int S, int A>
struct ConverterContainer {
    alignas(alignof(ConverterDriver))
    char m_driver[sizeof(ConverterDriver)];
    alignas(A)
    char m_space[S];

    ConverterDriver *driver() const {
        auto unconst = const_cast<ConverterContainer *>(this);
        return reinterpret_cast<ConverterDriver *>(unconst->m_driver);
    }

    ConverterContainer() { new(driver()) ConverterDriver; }

    void destroy() { driver()->destroy(m_space); }

    void copy(ConverterContainer *to) {
        driver()->copy(to->driver(), to->m_space, m_space);
    }

    void move(ConverterContainer *to) noexcept {
        driver()->move(to->driver(), to->m_space, m_space);
    }

    void moveAndDestroy(ConverterContainer *to) noexcept {
        move(to);
        destroy();
    }

    void *value() noexcept { return driver()->value(m_space); }

    bool nonEmpty() const noexcept { return const_cast<ConverterContainer *>(this)->value(); }

    const std::type_info &type() const noexcept { return driver()->type(); }
};

template<int S, int A, typename T>
struct TypedConverterContainer: ConverterContainer<S, A> {
    constexpr static auto ValueSemantics = canUseValueSemantics<T>(S, A);

    template<
        typename... Args,
        std::enable_if_t<ValueSemantics && 0 <= sizeof...(Args), int> = 0
    >
    TypedConverterContainer(Args &&...args) {
        new(this->m_space) T{std::forward<Args>(args)...};
        new(this->driver()) ConverterValueDriver<T>;
    }

    template<
        typename... Args,
        std::enable_if_t<!ValueSemantics && 0 <= sizeof...(Args), int> = 0
    >
    TypedConverterContainer(Args &&...args) {
        makeReferential<T>(
            this->driver(),
            this->m_space,
            std::forward<Args>(args)...
        );
        ANNOTATE_MAKE_REFERENTIAL(
            this->driver(), this->m_space, *(void **)this->m_space
        )
    }
};

template<int Size, int Alignment>
struct ConverterPolicy {
    using MemoryLayout = ConverterContainer<Size, Alignment>;

    template<typename ValueType>
    using Builder = TypedConverterContainer<Size, Alignment, ValueType>;

    template<typename V>
    bool isRuntimeValue(MemoryLayout &e) {
        return dynamic_cast<ConverterValueDriver<V> *>(e.driver());
    }

    template<typename V>
    bool isRuntimeReference(MemoryLayout &e) {
        return dynamic_cast<ConverterReferentialDriver<V> *>(e.driver());
    }
};

template<int ToSize, int ToAlignment, int FromSize, int FromAlignment>
AnyContainer<ConverterPolicy<ToSize, ToAlignment>>
convert(const AnyContainer<ConverterPolicy<FromSize, FromAlignment>> &from) {
    AnyContainer<ConverterPolicy<ToSize, ToAlignment>> rv;
    auto tc = rv.container();
    auto fc = from.container();
    fc->driver()->copyConvert(
        tc->driver(), tc->m_space, fc->m_space, ToSize, ToAlignment
    );
    return rv;
}

} // zoo

