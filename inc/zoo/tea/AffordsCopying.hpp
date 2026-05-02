#ifndef ZOO_TEA_AFFORDSCOPYING_H
#define ZOO_TEA_AFFORDSCOPYING_H
#include <type_traits>

namespace zoo::tea {

namespace detail {

template<typename, typename = void>
struct MemoryLayoutHasCopy: std::false_type {};
template<typename Policy>
struct MemoryLayoutHasCopy<
    Policy,
    std::void_t<decltype(&Policy::MemoryLayout::copy)>
>: std::true_type {};

template<typename, typename = void>
struct ExtraAffordanceOfCopying: std::false_type {};
template<typename Policy>
struct ExtraAffordanceOfCopying<
    Policy,
    std::void_t<decltype(&Policy::ExtraAffordances::copy)>
>: std::true_type {};

/// Copy constructibility and assignability are fundamental operations that
/// can not be enabled/disabled with SFINAE, this trait is to detect copyability
/// in a policy
template<typename Policy>
using AffordsCopying =
    std::disjunction<
        detail::MemoryLayoutHasCopy<Policy>, detail::ExtraAffordanceOfCopying<Policy>
    >;

} // detail

} // zoo::tea

#endif
