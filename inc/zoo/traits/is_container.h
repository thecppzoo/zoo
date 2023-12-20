#ifndef ZOO_CONTAINER_TRAIT
#define ZOO_CONTAINER_TRAIT

#ifndef SIMPLIFY_INCLUDES
#include <type_traits>

#ifdef _MSC_VER
#include <iso646.h>
#endif

#endif

namespace zoo {

namespace detail {

template<typename T, typename = void>
struct is_container: std::false_type {};
template<typename T>
struct is_container<
    T,
    std::void_t<
        decltype(std::declval<T>().cbegin()),
        decltype(std::declval<T>().cend())
    >
>: std::true_type {};

}

template<typename T>
constexpr auto is_container_v = detail::is_container<T>::value;

}

#endif
