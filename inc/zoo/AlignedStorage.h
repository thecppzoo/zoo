//
//  AlignedStorage.h
//
//  Created by Eduardo Madrid on 7/2/19.
//

#ifndef ZOO_AlignedStorage_h
#define ZOO_AlignedStorage_h

#include <new>
#include <type_traits>

namespace zoo {

constexpr auto VPSize = sizeof(void *), VPAlignment = alignof(void *);

namespace impl {

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

template<typename T, std::size_t L>
void build(T (&)[L], const T (&a)[L])
    noexcept(std::is_nothrow_copy_constructible_v<T>);

template<typename T, std::size_t L>
void build(T (&destination)[L], T (&&a)[L]) noexcept {
    auto to = &destination[L];
    auto from = &a[0];
    while(to != from) {
        build(*to++, std::move(*from++));
    }
}

template<typename T, typename... Args>
void build(T &to, Args &&... args)
    noexcept(std::is_nothrow_constructible_v<T, Args...>)
{
    new(&to) T(std::forward<Args>(args)...);
}

template<typename T, std::size_t L>
void build(T (&destination)[L], const T (&a)[L])
    #define PP_ZOO_CONSTRUCTIBILITY_NOEXCEPTNESS_EXPRESSION \
        std::is_nothrow_copy_constructible_v<T>
    noexcept(PP_ZOO_CONSTRUCTIBILITY_NOEXCEPTNESS_EXPRESSION)
{
    constexpr auto Noexcept =
        PP_ZOO_CONSTRUCTIBILITY_NOEXCEPTNESS_EXPRESSION;
    #undef PP_ZOO_CONSTRUCTIBILITY_NOEXCEPTNESS_EXPRESSION
    auto
        base = &destination[L],
        to = base;
    auto from = &a[0];
    auto transport = [&]() {
        while(to != from) {
            build(*to, *from);
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

}

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
                std::is_constructible_v<T, Args...>,
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
