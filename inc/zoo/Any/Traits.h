#ifndef ZOO_ANY_Traits_H
#define ZOO_ANY_Traits_H

#include <type_traits>

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

}}

#endif
