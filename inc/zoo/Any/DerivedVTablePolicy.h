#ifndef ZOO_DERIVEDVTABLEPOLICY_H
#define ZOO_DERIVEDVTABLEPOLICY_H

#include "VTablePolicy.h"

namespace zoo {

namespace impl {

struct EmptyRawAffordances {};

template<typename, typename = void>
struct RawAffordances_impl { using type = EmptyRawAffordances; };
template<typename T>
struct RawAffordances_impl<T, std::void_t<typename T::ExtraAffordances>> {
    using type = typename T::ExtraAffordances;
};

}

template<typename BaseContainer, typename... Extensions>
struct DerivedVTablePolicy {
    using Base = BaseContainer;

    using BasePolicy = typename BaseContainer::Policy;

    using MemoryLayout = typename BasePolicy::MemoryLayout;

    struct VTable: BasePolicy::VTable, Extensions::VTableEntry... {};

    using TypeSwitch = VTable;

    struct ExtraAffordances: impl::RawAffordances_impl<BasePolicy>::type, Extensions::template Raw<VTable, MemoryLayout>... {};

    struct DefaultImplementation: MemoryLayout {
        constexpr static inline VTable Default = {
            BasePolicy::DefaultImplementation::Default,
            Extensions::template Default<DefaultImplementation>...
        };

        DefaultImplementation(): MemoryLayout(&Default) {}
    };

    template<typename V>
    struct Builder: BasePolicy::template Builder<V> {
        using Base = typename BasePolicy::template Builder<V>;

        constexpr static inline VTable Operations = {
            Base::Operations,
            Extensions::template Operation<Builder>...
        };

        template<typename... Args>
        Builder(Args &&...args):
            Builder(&Operations, std::forward<Args>(args)...)
        {}

        template<typename... Args>
        Builder(const VTable *ops, Args &&...args):
            Base(
                static_cast<const typename BasePolicy::VTable *>(ops),
                std::forward<Args>(args)...
            )
        {}
    };

    template<typename AnyC>
    struct Affordances: Extensions::template UserAffordance<AnyC>... {};
};

}

#endif //ZOO_DERIVEDVTABLEPOLICY_H
