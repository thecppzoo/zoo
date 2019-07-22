//
//  AlignedStorage.h
//
//  Created by Eduardo Madrid on 7/2/19.
//

#ifndef ZOO_AlignedStorage_h
#define ZOO_AlignedStorage_h

#include "zoo/meta/traits.h"

#include <new>
#include <type_traits>

namespace zoo {

constexpr auto VPSize = sizeof(void *), VPAlignment = alignof(void *);

namespace impl {

template<typename, typename = void>
struct Arraish_impl: std::false_type {};
template<typename T>
struct Arraish_impl<T, decltype(&std::declval<T>()[0])>: std::true_type {};

template<typename T, typename... Args>
struct Constructible: std::is_constructible<T, Args...> {};

template<typename T, std::size_t L, typename Source>
struct Constructible<T[L], Source>:
    Constructible<
        T,
        meta::copy_cr_t<
            meta::remove_cr_t<decltype(std::declval<Source>()[0]) &&>,
            Source
        >
    >
{};

template<typename T, typename... Args>
constexpr auto Constructible_v = Constructible<T, Args &&...>::value;

template<typename T>
void destroy(T &t) noexcept { t.~T(); }

template<typename T, std::size_t L>
void destroy(T (&a)[L]) noexcept {
    auto
        base = &a[0],
        lifo = base + L;
    while(base != lifo--) { destroy(*lifo); }
}

template<typename T, typename... Args>
void build(T &, Args &&...)
    noexcept(std::is_nothrow_constructible_v<T, Args...>);

template<typename T, std::size_t L, typename ArrayLike>
auto build(T (&destination)[L], ArrayLike &&a)
    noexcept(
        std::is_nothrow_constructible_v<
            T,
            #define PP_ZOO_BUILD_ARRAY_SOURCE_TYPE \
                meta::copy_cr_t<meta::remove_cr_t<decltype(a[0])>, ArrayLike &&> &&
            PP_ZOO_BUILD_ARRAY_SOURCE_TYPE
        >)
{
    using Source = PP_ZOO_BUILD_ARRAY_SOURCE_TYPE;
            #undef PP_ZOO_BUILD_ARRAY_SOURCE_TYPE
    constexpr auto Noexcept = std::is_nothrow_constructible_v<T, Source>;
    auto
        base = &destination[0],
        to = base,
        top = to + L;
    auto from = &a[0];
    auto transport = [&]() {
        while(top != to) {
            build(*to, static_cast<Source>(*from));
            ++to; ++from;
        }
    };
    if constexpr(Noexcept) {
        transport();
    } else {
        try {
            transport();
        } catch(...) {
            while(to-- != base) { destroy(*to); }
            throw;
        }
    }
}

template<typename T, typename... Args>
void build(T &to, Args &&... args)
    noexcept(std::is_nothrow_constructible_v<T, Args...>)
{
    new(&to) T(std::forward<Args>(args)...);
}

}

///! \note What about constructors and destructor?
template<int S = VPSize, int A = VPAlignment>
struct AlignedStorage {
    constexpr static auto Size = S, Alignment = A;

    alignas(A) char space_[Size];

    template<typename T>
    T *as() noexcept { return reinterpret_cast<T *>(&space_); }
    template<typename T>
    const T *as() const noexcept {
        return const_cast<AlignedStorage *>(this)->as<T>();
    }

    template<typename T>
    static constexpr auto SuitableType() {
        return sizeof(T) <= S && 0 == A % alignof(T);
    }

    template<typename T, typename... Args>
        #define PP_ZOO_BUILD_EXPRESSION \
            impl::build(*as<T>(), std::forward<Args>(args)...)
    auto build(Args  &&...args) noexcept(noexcept(PP_ZOO_BUILD_EXPRESSION)) ->
        std::enable_if_t<
            SuitableType<T>() &&
                impl::Constructible_v<T, Args...>,
            T *
        >
    {
        PP_ZOO_BUILD_EXPRESSION;
        #undef PP_ZOO_BUILD_EXPRESSION
        return as<T>();
    }

    template<typename T>
    auto destroy() noexcept ->
        std::enable_if_t<SuitableType<T>() && std::is_nothrow_destructible_v<T>>
    {
        impl::destroy(*as<T>());
    }
};

}

#endif /* AlignedStorage_h */
