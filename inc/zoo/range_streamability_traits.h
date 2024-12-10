#ifndef ZOO_RANGE_STREAMABILITY_TRAITS_H
#define ZOO_RANGE_STREAMABILITY_TRAITS_H

#include "zoo/pp/traits_support.h"
#include "zoo/range_adl_overloads.h"

#include <iterator>
#include <type_traits>
#include <utility>
#include <ostream>

namespace zoo {

// Trait to check if a type is directly streamable
ZOO_PP_MAKE_TRAIT(
    Streamable,
    std::declval<std::ostream &>() << std::declval<TypeToTriggerSFINAE &>()
)

// Trait to check if a type is a range (has begin and end)
ZOO_PP_MAKE_TRAIT(
    Range,
    (
        std::begin(std::declval<TypeToTriggerSFINAE &>()),
        std::end(std::declval<TypeToTriggerSFINAE &>())
    )
)

template<typename T>
struct RangeWithStreamableElements:
    std::conditional_t<
        Range<T>::value &&
        Streamable<
            decltype(*begin(std::declval<T &>()))
        >::value,
        std::true_type,
        std::false_type
    >
{};

ZOO_PP_V_VARIABLE_OUT_OF_TRAIT(RangeWithStreamableElements)

}

#endif // ZOO_RANGE_STREAMABILITY_TRAITS_H

