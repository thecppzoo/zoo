#ifndef ZOO_DERIVEDANYCONTAINER_HPP
#define ZOO_DERIVEDANYCONTAINER_HPP

#include "zoo/Any/DerivedVTablePolicy.h"
#include "zoo/AnyContainer.h"

namespace zoo {

template<typename BasePolicy, typename... Affordances>
struct AnyContainerDerived: AnyContainer<BasePolicy> {
    using BaseContainer = AnyContainer<BasePolicy>;
    using P = DerivedPolicy<BasePolicy, Affordances...>;

    AnyContainerDerived() noexcept: BaseContainer(BaseContainer::Token) {
        new(this->m_space) typename P::DefaultImplementation;
    }
};

}

#endif //ZOO_DERIVEDANYCONTAINER_HPP
