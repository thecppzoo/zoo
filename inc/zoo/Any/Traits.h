#ifndef ZOO_ANY_Traits_H
#define ZOO_ANY_Traits_H

#include <type_traits>

template<typename Policy>
struct AnyContainerBase;

namespace zoo { namespace detail {

template<typename, typename = const bool>
struct PRMO_impl: std::false_type {};

template<typename Policy>
struct PRMO_impl<Policy, decltype(Policy::RequireMoveOnly)>:
    std::conditional_t<Policy::RequireMoveOnly, std::true_type, std::false_type>
{};

template<typename Policy>
constexpr auto RequireMoveOnly_v = PRMO_impl<Policy>::value;

struct NoAffordances {};

template<typename Container, typename Policy, typename = void>
struct PolicyAffordances_impl {
    using type = NoAffordances;
};

template<typename Container, typename Policy>
struct PolicyAffordances_impl<
    Container,
    Policy,
    std::void_t<typename Policy::template Affordances<Container>>
> {
    using type = typename Policy::template Affordances<Container>;
};

template<typename Container, typename Policy>
using PolicyAffordances =
    typename PolicyAffordances_impl<Container, Policy>::type;

}

// Forward declaration needed for the IsContainer trait
template<typename>
struct AnyContainerBase;

namespace detail {
template<typename, typename = void>
struct IsAnyContainer_impl: std::false_type {};

template<typename T>
struct IsAnyContainer_impl<T, std::void_t<typename T::Policy>>:
    std::conjunction<std::is_base_of<AnyContainerBase<typename T::Policy>, T>>
{};

template<typename, typename>
struct CompatiblePolicy: std::false_type {};

template<typename P1, typename P2>
constexpr auto CompatiblePolicy_v = CompatiblePolicy<P1, P2>::value;

}}

#endif
