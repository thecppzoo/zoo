#ifndef ZOO_VAR_H

/// \file VAR.H
/// \brief Simplification of std::Var
/// Differences:
/// 1. There is no fail state, the user should specify a fail state
/// 2. Allows the user to indicate how to do visit using the mechanisms of
///    1. Call table: Very short assembler but the compiler can not optimize
///    across the call
///    2. Compiler optimized logic: Sometimes the optimization in Clang is
///    effective, sometimes it isn't

#include <zoo/meta/in_place_operations.h>
#include <zoo/meta/traits.h>
#include <zoo/AlignedStorage.h>

#include <tuple>
    // provides std::tuple_element to be able to index a pack of types,
    // indirectly includes type traits and utility
#include <new>
#include <algorithm>

namespace zoo {

template<int Index, typename... Ts>
using TypeAtIndex_t =
    typename std::tuple_element<Index, std::tuple<Ts...>>::type;

template<typename... Ts>
constexpr auto MaxSize_v = std::max({sizeof(Ts)...});

template<typename... Ts>
constexpr auto MaxAlignment_v = std::max({alignof(Ts)...});

namespace impl {
    template<typename CRTP>
    struct DefaultConstructible {
        DefaultConstructible() noexcept {
            static_cast<CRTP *>(this)->defaultConstruct();
        }
    };

    struct NonDefaultConstructible {
        NonDefaultConstructible() = delete;
    };

    template<typename CRTP>
    struct Copiable {
        Copiable() = default;
        Copiable(const Copiable &c) {
            static_cast<CRTP *>(this)->copy(static_cast<const Copiable &>(c));
        }
        Copiable(Copiable &&) = default;
    };

    struct NonCopiable {
        NonCopiable() = default;
        NonCopiable(const NonCopiable &) = delete;
        NonCopiable(NonCopiable &&) = default;
    };

    template<typename CRTP>
    struct Movable {
        Movable() = default;
        Movable(const Movable &) = default;
        Movable(Movable &&m) noexcept {
            static_cast<CRTP *>(this)->move(static_cast<CRTP &&>(m));
        }
    };

    struct NonMovable {
        NonMovable(NonMovable &&) = delete;
    };
};

template<typename... Ts>
struct Var;

template<typename T>
struct IsVar: std::false_type {};

template<typename... Ts>
struct IsVar<Var<Ts...>>: std::true_type {};

template<typename T>
constexpr auto IsVar_v = IsVar<meta::remove_cr_t<T>>::value;

namespace impl {
template<int, typename>
struct Alternative; // do not define

template<int Index, typename... Ts>
struct Alternative<Index, Var<Ts...>> {
    using type = TypeAtIndex_t<Index, Ts...>;
};
}

template<int Index, typename T>
using Alternative_t = typename impl::Alternative<Index, T>::type;

template<int Index, typename V>
auto get(V &&v) ->
    std::enable_if_t<
        IsVar_v<V>,
        meta::copy_cr_t<Alternative_t<Index, meta::remove_cr_t<V>>, V &&>
    >
{
    using Va = meta::remove_cr_t<V>;
    using R = meta::copy_cr_t<Alternative_t<Index, Va>, V &&>;
    return static_cast<R>(*v.template as<Alternative_t<Index, Va>>());
}

template<typename Visitor, typename Va>
auto visit(Visitor &&visitor, Va &&var) ->
    decltype(visitor(get<0>(std::forward<Va>(var))));

template<typename... Ts>
struct Var:
        #define PP_VAR_TS_CRTP(TEMPLATE1, FALSE, ...) \
            std::conditional_t<__VA_ARGS__, impl::TEMPLATE1<Var<Ts...>>, impl::FALSE>
    PP_VAR_TS_CRTP(
        DefaultConstructible,
        NonDefaultConstructible,
        std::is_nothrow_default_constructible_v<TypeAtIndex_t<0, Ts...>>
    ),
    PP_VAR_TS_CRTP(
        Copiable,
        NonCopiable,
        (std::is_copy_constructible_v<Ts> && ...)
    ),
    PP_VAR_TS_CRTP(
        Movable,
        NonMovable,
        (std::is_nothrow_move_constructible_v<Ts> && ...)
    )
        #undef PP_VAR_TS_CRTP
{
    constexpr static auto Count = sizeof...(Ts);
    constexpr static auto Size = MaxSize_v<Ts...>;
    constexpr static auto Alignment = MaxAlignment_v<Ts...>;

    AlignedStorage<Size, Alignment> space_;
    int typeSwitch_;

    Var() = default;
    auto defaultConstruct() noexcept
    {
        if constexpr(std::is_nothrow_default_constructible_v<Var>) {
            space_.template build<Alternative_t<0, Var>>();
            typeSwitch_ = 0;
        }
    }

    Var(const Var &) = default;
    void copy(const Var &v) {
        if constexpr(std::is_copy_constructible_v<Var>) {
            visit(
                [&](const auto &c) {
                    using Source = meta::remove_cr_t<decltype(c)>;
                    space_->template build<Source>(c);
                },
                v
            );
            typeSwitch_ = v.typeSwitch_;
        }
    }

    Var(Var &&) = default;
    void move(Var &&v) noexcept {
        if constexpr(std::is_nothrow_move_constructible_v<Var>) {
            visit(
                [&](auto &&m) {
                    using Source = meta::remove_cr_t<decltype(m)>;
                    meta::move_in_place(as<Source>(), std::move(m));
                },
                v
            );
            typeSwitch_ = v.typeSwitch_;
        }
    }

    template<std::size_t Ndx, typename... Args>
    Var(std::in_place_index_t<Ndx>, Args &&...args): typeSwitch_{Ndx} {
        using Held = Alternative_t<Ndx, Var>;
        space_.template build<Held>(std::forward<Args>(args)...);
    }

    /// \todo should be improved
    Var &operator=(const Var &model) {
        Var copy{model};
        std::swap(*this, copy);
        return *this;
    }

    /// \bug pending SFINAE and noexcept
    Var &operator=(Var &&other) noexcept {
        destroy();
        new(this) Var{std::move(other)};
        return *this;
    }

    ~Var() { destroy(); }

    template<typename T>
    auto as() noexcept { return space_.template as<T>(); }

    template<typename T>
    const T *as() const noexcept {
        return const_cast<Var *>(this)->as<T>();
    }

private:
    void destroy() {
        visit(
            [](auto &who) { meta::destroy_in_place(who); },
            *this
        );
    }

};

template<typename Visitor, typename Va, int Current, int Top>
struct Visit {
    static decltype(auto) execute(Visitor &&vi, Va &&va) {
        switch(va.typeSwitch_) {
            case Current: return vi(get<Current>(std::forward<Va>(va)));
            default:
                return
                    Visit<Visitor, Va, Current + 1, Top>::execute(
                        std::forward<Visitor>(vi),
                        std::forward<Va>(va)
                    );
        }
    }
};

template<typename Visitor, typename Va, int Top>
struct Visit<Visitor, Va, Top, Top> {
    static decltype(auto) execute(Visitor &&vi, Va &&va) {
        return vi(get<Top>(std::forward<Va>(va)));
    }
};

template<typename Visitor, typename Va>
auto visit(Visitor &&visitor, Va &&var) ->
    decltype(visitor(get<0>(std::forward<Va>(var))))
{
    using V = std::remove_reference_t<Va>;
    return Visit<Visitor, Va, 0, V::Count - 1>::execute(
        std::forward<Visitor>(visitor),
        std::forward<Va>(var)
    );
}

/// \bug Value category not preserved

template<typename R, typename Visitor, typename Va, typename... Ts>
R GCC_visit_injected(Visitor &&visitor, Va &&value, const Var<Ts...> *) {
    using Vi = decltype(visitor);
    using V = decltype(value);
    constexpr static R (*visits[1 + sizeof...(Ts)])(Vi, V) = {
        [](Vi vi, V va) { return vi(*va.template as<Ts>()); }...
    };
    return
        visits[value.typeSwitch_](
            std::forward<Visitor>(visitor), std::forward<Va>(value)
        );
}

template<typename Visitor, typename Va>
auto GCC_visit(Visitor &&visitor, Va &&value) ->
    decltype(visitor(get<0>(value)))
{
    using R = decltype(visitor(get<0>(value)));
    return
        GCC_visit_injected<R>(
            std::forward<Visitor>(visitor),
            std::forward<Va>(value),
            &value
        );
}


template<typename... Ts> struct Overloads : Ts... { using Ts::operator()...; };
template<typename... Ts> Overloads(Ts...) -> Overloads<Ts...>;

template<typename V, typename... Visitors>
auto match(V &&var, Visitors &&... vis) {
    return
        visit(
            Overloads{std::forward<Visitors>(vis)...},
            std::forward<V>(var)
        );
}

}

#endif

