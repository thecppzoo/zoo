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
struct Copiable {
    Copiable() = default;
    Copiable(const Copiable &c) {
        static_cast<CRTP *>(this)->copy(c);
    }
    Copiable(Copiable &&) = default;
};

template<typename CRTP>
struct Movable {
    Movable() = default;
    Movable(const Movable &) = default;
    Movable(Movable &&m) noexcept {
        static_cast<CRTP *>(this)->move(m);
    }
};
}

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
struct Var {
    constexpr static auto Count = sizeof...(Ts);
    constexpr static auto Size = MaxSize_v<Ts...>;
    constexpr static auto Alignment = MaxAlignment_v<Ts...>;
    constexpr static auto NTMC =
        std::conjunction<std::is_nothrow_move_constructible<Ts>...>::value;

    AlignedStorage<Size, Alignment> space_;
    int typeSwitch_;

    /// \todo Force the first alternative to be nothrow_default_constructible
    Var() noexcept: Var(std::in_place_index<0>) {}

    Var(const Var &v): typeSwitch_{v.typeSwitch_} {
        visit(
            [&](const auto &c) {
                using Source = meta::remove_cr_t<decltype(c)>;
                space_->template build<Source>(c);
            },
            v
        );
    }

    Var(Var &&v) noexcept(NTMC): typeSwitch_{v.typeSwitch_} {
        visit(
            [&](auto &&m) {
                using Source = meta::remove_cr_t<decltype(m)>;
                meta::move_in_place(as<Source>(), std::move(m));
            },
            v
        );
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

    Var &operator=(Var &&other) noexcept(NTMC) {
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

