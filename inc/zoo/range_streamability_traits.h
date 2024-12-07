#ifndef ZOO_RANGE_STREAMABILITY_TRAITS_H
#define ZOO_RANGE_STREAMABILITY_TRAITS_H

#include "zoo/pp/traits_support.h"
#include <iterator>
#include <type_traits>
#include <utility>
#include <ostream>

namespace zoo {

// Trait to check if a type is directly streamable
ZOO_PP_MAKE_TRAIT(
    IsStreamable,
    std::declval<std::ostream &>() << std::declval<TypeToTriggerSFINAE &>()
)

// Trait to check if a type is a range (has begin and end)
ZOO_PP_MAKE_TRAIT(
    IsRange,
    (std::begin(std::declval<TypeToTriggerSFINAE &>()),
     std::end(std::declval<TypeToTriggerSFINAE &>()))
)

// Trait to check if a range has a streamable value type
ZOO_PP_MAKE_TRAIT(
    RangeWithStreamableValueType,
    IsRange<TypeToTriggerSFINAE>::value &&
    IsStreamable<
        typename std::iterator_traits<
            decltype(std::begin(std::declval<TypeToTriggerSFINAE &>()))
        >::value_type
    >::value
)

}

#endif // ZOO_RANGE_STREAMABILITY_TRAITS_H

