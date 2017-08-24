#include "meta/NotBasedOf.h"

#include <new>

namespace zoo {

template<int Size, int Alignment>
struct IAnyContainer {
    virtual void destroy() {}

    alignas(Alignment)
    char m_space[Size];
};

template<int Size, int Alignment, typename ValueType>
struct ValueContainer: IAnyContainer<Size, Alignment> {
    ValueType *thy() { return reinterpret_cast<ValueType *>(this->m_space); }

    void destroy() override { thy()->~ValueType(); }
};

template<int Size, int Alignment, typename ValueType>
struct ReferentialContainer: IAnyContainer<Size, Alignment> {
    ValueType **pThy() { return reinterpret_cast<ValueType **>(this->m_space); }

    ValueType *thy() { return *pThy(); }

    void destroy() override { thy()->~ValueType(); }
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

template<int Size, int Alignment, typename TypeSwitch>
struct AnyContainer {
    using Container = typename TypeSwitch::template empty<Size, Alignment>;

    alignas(alignof(Container))
    char m_space[sizeof(Container)];

    AnyContainer() { new(m_space) Container; }

    AnyContainer(const AnyContainer &model) {
        auto source = model.container();
        source->copy(this);
    }

    AnyContainer(AnyContainer &&moveable);

    template<typename Initializer>
    AnyContainer(Initializer &&initializer);

    ~AnyContainer() { container()->destroy(); }

    Container *container() const {
        return reinterpret_cast<Container *>(const_cast<char *>(m_space));
    }
};

using Any = AnyContainer<8, 8, PolymorphicTypeSwitch>;

}
