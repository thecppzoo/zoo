#pragma once

#include "util/any.h"

namespace zoo {

struct ConverterDriver {
    virtual void destroy(void *) {}

    virtual void copy(ConverterDriver *dd, void *spc, const void *source) {
        new(dd) ConverterDriver;
    }

    virtual void move(ConverterDriver *dd, void *spc, const void *source) {
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
struct BaseConverter: ConverterDriver {
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
            new(spc) ValueType{*thy(source)};
        } catch(...) {
            CRTP::recclaim(spc);
            throw;
        }
        new(dd) CRTP;
    }

    void move(ConverterDriver *dd, void *spc, const void *source) override {
        new(spc) ValueType{std::move(*thy(source))};
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
struct ConverterValue: BaseConverter<ValueType, ConverterValue<ValueType>> {
    static void make(void *) {}

    static void recclaim(void *) {}

    static void *val(void *arg) { return arg; }
};

template<typename ValueType>
struct ConverterReferential {
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
};

template<typename ValueType, typename... Args>
void makeReferential(ConverterDriver *dd, void *space, Args &&... args) {
    using Reference = ConverterReferential<ValueType>;
    Reference::make(space);
    try {
        new(Reference::val(space)) ValueType{std::forward<Args>(args)...};
    } catch(...) {
        Reference::recclaim(space);
        throw;
    } 
    new(dd) Reference;
}

template<typename ValueType, typename CRTP>
void BaseConverter<ValueType, CRTP>::copyConvert(
    ConverterDriver *dd, void *spc, const void *source,
    int destinationSize, int destinationAlignment
) {
    auto modifiable = const_cast<void *>(source);
    auto sourceValue = reinterpret_cast<ValueType *>(CRTP::val(modifiable));
    auto value = const_cast<const ValueType *>(sourceValue);
   
    if(canUseValueSemantics<ValueType>(destinationSize, destinationAlignment)) {
        new(spc) ValueType{*value};
        new(dd) ConverterValue<ValueType>;
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

    ConverterDriver *driver() {
        return reinterpret_cast<ConverterDriver *>(m_driver);
    }

    ConverterContainer() { new(driver()) ConverterDriver; }

    void destroy() { driver()->destroy(m_space); }

    void copy(ConverterContainer *to) {
        driver()->copy(to->driver(), to->m_space, m_space);
    }

    void move(ConverterContainer *to) noexcept {
        driver()->move(to->driver(), to->m_space, m_space);
    }

    void *value() noexcept { return driver()->value(); }

    bool nonEmpty() const noexcept { return driver()->value(); }

    const std::type_info &type() const noexcept { return driver()->type(); }
};

template<int S, int A, typename T>
struct TypedConverterContainer: ConverterContainer<S, A> {
    template<typename... Args>
    TypedConverterContainer(Args &&...args) {
        if(canUseValueSemantics<T>(S, A)) {
            new(this->m_space) T{std::forward<Args>(args)...};
            new(this->driver()) ConverterValue<T>;
        } else {
            makeReferential<T>(
                this->driver(), this->m_space, std::forward<Args>(args)...
            );
        }
    }
};

template<int Size_, int Alignment_>
struct ConverterPolicy {
    constexpr static auto
        Size = Size_,
        Alignment = Alignment_;

    using Empty = ConverterContainer<Size, Alignment>;

    template<typename ValueType>
    using Implementation = TypedConverterContainer<Size, Alignment, ValueType>;

    template<typename V>
    bool isRuntimeValue(Empty &e) {
        return dynamic_cast<ConverterValue<V> *>(e.driver());
    }

    template<typename V>
    bool isRuntimeReference(Empty &e) {
        return dynamic_cast<ConverterReferential<V> *>(e.driver());
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

