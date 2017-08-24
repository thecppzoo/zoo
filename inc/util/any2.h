#include "meta/NotBasedOf.h"

#include <new>

namespace zoo {

template<int Size, int Alignment>
struct IAnyContainer {
    virtual void destroy() {}

    alignas(Alignment)
    char m_space[Size];
};

template<int Size, int Alignment>
struct ValueContainer: IAnyContainer<Size, Alignment> {};

template<int Size, int Alignment>
struct ReferentialContainer: IAnyContainer<Size, Alignment> {};

template<int Size, int Alignment, bool Value>
struct PolymorphicImplementationDecider {
    using type = ReferentialContainer<Size, Alignment>;
};

template<int Size, int Alignment>
struct PolymorphicImplementationDecider<Size, Alignment, true> {
    using type = ValueContainer<Size, Alignment>;
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
            Size, Alignment, useValueSemantics<Size, Alignment, ValueType>()
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

    ~AnyContainer() { container()->destroy(); }

    Container *container() const {
        return reinterpret_cast<Container *>(const_cast<char *>(m_space));
    }
};

using Any = AnyContainer<8, 8, PolymorphicTypeSwitch>;

}
