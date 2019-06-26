//
//  AnyMovable.h
//  Zoo wrap
//
//  Created by Eduardo Madrid on 6/24/19.
//  Copyright © 2019 Eduardo Madrid. All rights reserved.
//

#ifndef AnyMovable_h
#define AnyMovable_h

#include "zoo/AnyContainer.h"

// clang-format off
namespace zoo {

template<typename Policy>
struct AnyMovable: AnyContainer<Policy> {
    using Base = AnyContainer<Policy>;
    using Base::Base;

    template<typename Arg>
    constexpr static auto SuitableForBuilding() {
        return
            meta::NotBasedOn<Arg, AnyMovable>() && // disables self-building
            !std::is_lvalue_reference_v<Arg>
        ;
    }

    AnyMovable(const AnyMovable &) = delete;
    AnyMovable(AnyMovable &&) = default;

    template<
        typename Arg,
        typename = std::enable_if_t<SuitableForBuilding<Arg>()>
    >
    AnyMovable(Arg &&a): Base(std::forward<Arg>(a)) {}

    AnyMovable &operator=(const AnyMovable &) = delete;
    AnyMovable &operator=(AnyMovable &&) = default;

    template<typename Argument>
    std::enable_if_t<
        SuitableForBuilding<Argument>(),
        AnyMovable &
    > operator=(Argument &&argument) noexcept(
        noexcept(std::declval<Base &>() = std::forward<Argument>(argument))
    ) {
        Base::operator=(std::forward<Argument>(argument));
        return *this;
    }
};

}

#endif /* AnyMovable_h */
