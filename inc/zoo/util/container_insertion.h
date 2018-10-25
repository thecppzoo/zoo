#ifndef ZOO_CONTAINER_INSERTION
#define ZOO_CONTAINER_INSERTION

#include <zoo/traits/is_container.h>
#include <ostream>

namespace zoo {

template<typename T, typename = void>
struct is_insertable_impl: std::false_type {};
template<typename T>
struct is_insertable_impl<
    T,
    std::void_t<decltype(
        std::declval<std::ostream &>() << std::declval<T>()
    )>
>: std::true_type {};

}

namespace std {

template<typename C>
auto operator<<(std::ostream &out, const C &a)
-> std::enable_if_t<
    not(zoo::is_insertable_impl<C>::value) && zoo::is_container_v<C>,
    std::ostream &
> {
    out << '(';
    auto current{cbegin(a)}, sentry{cend(a)};
    if(current != sentry) {
        for(;;) {
            out << *current++;
            if(sentry == current) { break; }
            out << ", ";
        }
    }
    return out << ')';
}

}

#endif
