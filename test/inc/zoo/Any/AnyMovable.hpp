#ifndef ZOO_ANYMOVABLE_HPP
#define ZOO_ANYMOVABLE_HPP

#include "zoo/Any/VTablePolicy.h"

#include "zoo/AnyContainer.h"

namespace zoo {

template<typename HoldingModel>
struct AnyMovable: AnyContainer<typename GenericPolicy<HoldingModel, Destroy, Move>::Policy> {
    using Policy = typename GenericPolicy<HoldingModel, Destroy, Move>::Policy;
};

}

#endif //ZOOTEST_ANYMOVABLE_HPP

