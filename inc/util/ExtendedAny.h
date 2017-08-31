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

template<typename ValueType>
struct ConverterReferential;

template<typename ValueType>
struct ConverterValue: ConverterDriver {
    ValueType *thy(void *ptr) { return reinterpret_cast<ValueType *>(ptr); }

    void destroy(void *ptr) override {
        thy(ptr)->~ValueType();
    }

    void copy(ConverterDriver *dd, void *spc, int size, int alignment) override;
};

template<int Size_, int Alignment_>
struct ConverterPolicy {
    constexpr static auto
        Size = Size_,
        Alignment = Alignment_;

    using Empty = ConverterContainer<Size, Alignment>;
    template<typename>
    using Implementation = ConverterContainer<Size, Alignment>;
};

template<int ToSize, int ToAlignment, int FromSize, int FromAlignment>
AnyContainer<ConverterPolicy<ToSize, ToAlignment>>
convert(const AnyContainer<ConverterPolicy<FromSize, FromAlignment>> &from) {
    AnyContainer<ConverterPolicy<ToSize, ToAlignment>> rv;
    auto tc = rv.container();
    auto fc = from.container();
    fc->driver()->copyConvert(tc->driver(), tc->m_space, fc->m_space, ToSize, ToAlignment);
    return rv;
}

} // zoo

