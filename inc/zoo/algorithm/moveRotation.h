#ifndef ZOO_N_MOVE_ROTATION
#define ZOO_N_MOVE_ROTATION

#include <utility>
#include <type_traits>

namespace zoo {

template<typename A>
constexpr void
moveChain(A &) noexcept {}

template<template<typename> class TT, typename... Ts>
struct meta_conjunction: std::conjunction<TT<Ts>...> {};

template<typename Doing, typename Next, typename... After>
constexpr void
moveChain(Doing &doing, Next &next, After &...after)
noexcept (
    meta_conjunction<
        std::is_nothrow_move_assignable,
        Doing, Next, After...
    >::value
) {
    doing = std::move(next);
    moveChain(next, after...);
};

template<typename First, typename Second, typename... Rest>
constexpr void
moveRotation(First &f, Second &s, Rest &...r) noexcept(
    std::is_nothrow_move_constructible<First>::value &&
    noexcept(moveChain(f, s, r..., f))
) {
    First wrapAround{std::move(f)};
    moveChain(f, s, r..., wrapAround);
}

}

#endif
