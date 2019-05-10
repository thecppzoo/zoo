#ifndef VARIANT_H

#include <tuple>
    // provides std::tuple_element to be able to index a pack of types,
    // indirectly includes type traits and utility
#include <new>
#include <zoo/meta/in_place_operations.h>
#include <zoo/meta/traits.h>

namespace zoo {

struct Maxer {
    int v_;
    constexpr Maxer(int v): v_{v} {}
    constexpr operator int() const { return v_; }
    constexpr Maxer operator*(Maxer o) const { return v_ < o.v_ ? o.v_ : v_; }
};

template<int... Ints>
constexpr auto Largest = int{(Maxer{0} * ... * Maxer(Ints))};

template<typename... Ts>
constexpr auto LargestSize = Largest<sizeof(Ts)...>;

template<typename... Ts>
constexpr auto LargestAlignment = Largest<alignof(Ts)...>;

static_assert(sizeof(void *) == LargestSize<int, float, char, void *>, "");
static_assert(alignof(short) == LargestAlignment<short, char, char[4]>, "");

template<int Index, typename... Ts>
using TypeAtIndex =
    typename std::tuple_element<Index, std::tuple<Ts...>>::type;

template<typename... Ts>
struct Variant;

struct BadVariant {};

template<int, typename>
struct AlternativeType;

template<int Ndx, typename... Ts>
struct AlternativeType<Ndx, Variant<Ts...>> {
    using type = TypeAtIndex<Ndx, Ts..., BadVariant>;
};

template<int Ndx, typename V>
using AlternativeType_t =
    typename AlternativeType<Ndx, meta::remove_cr_t<V>>::type;

template<typename T>
struct IsVariant: std::false_type {};

template<typename... Ts>
struct IsVariant<Variant<Ts...>>: std::true_type {};

template<typename T>
constexpr auto IsVariant_v = IsVariant<meta::remove_cr_t<T>>::value;

template<int Index, typename V>
auto get(V &&v) ->
    std::enable_if_t<
        IsVariant_v<V>,
        meta::copy_cr_t<AlternativeType_t<Index, V>, V &&>
    >
{
    using R = meta::copy_cr_t<AlternativeType_t<Index, V>, V &&>;
    return static_cast<R>(*v.template as<AlternativeType_t<Index, V>>());
}

template<typename Visitor, typename Var>
auto visit(Visitor &&visitor, Var &&var) ->
    decltype(visitor(get<0>(std::forward<Var>(var))));

template<typename... Ts>
struct Variant {
    constexpr static auto Count = sizeof...(Ts);
    constexpr static auto Size = LargestSize<Ts...>;
    constexpr static auto Alignment = LargestAlignment<Ts...>;
    constexpr static auto NTMC =
        std::conjunction<std::is_nothrow_move_constructible<Ts>...>::value;

    alignas(Alignment) char space_[Size];
    int typeSwitch_ = Count;

    Variant() = default;

    Variant(const Variant &v): typeSwitch_{v.typeSwitch_} {
        visit(
            [&](const auto &c) {
                using Source = meta::remove_cr_t<decltype(c)>;
                meta::copy_in_place(as<Source>(), c);
            },
            v
        );
    }

    Variant(Variant &&v) noexcept(NTMC): typeSwitch_{v.typeSwitch_} {
        visit(
            [&](auto &&m) {
                using Source = meta::remove_cr_t<decltype(m)>;
                meta::move_in_place(as<Source>(), std::move(m));
            },
            v
        );
        v.typeSwitch_ = Count;
    }

    template<std::size_t Ndx, typename... Args>
    Variant(std::in_place_index_t<Ndx>, Args &&...args): typeSwitch_{Ndx} {
        using Held = TypeAtIndex<Ndx, Ts...>;
        new(as<Held>()) Held{std::forward<Args>(args)...};
    }

    Variant &operator=(const Variant &model) {
        Variant copy{model};
        std::swap(*this, copy);
        return *this;
    }

    Variant &operator=(Variant &&other) noexcept(NTMC) {
        destroy();
        new(this) Variant{std::move(other)};
        return *this;
    }

    ~Variant() { destroy(); }

    void reset() {
        destroy();
        typeSwitch_ = Count;
    }

    template<typename T>
    T *as() noexcept { return reinterpret_cast<T *>(space_); }

    template<typename T>
    const T *as() const noexcept {
        return const_cast<Variant *>(this)->as<T>();
    }

private:
    void destroy() {
        visit(
            [](auto &who) { meta::destroy_in_place(who); },
            *this
        );
    }

};

template<typename Visitor, typename Var, int Current, int Top>
struct Visit {
    static decltype(auto) execute(Visitor &&vi, Var &&va) {
        switch(va.typeSwitch_) {
            case Current: return vi(get<Current>(std::forward<Var>(va)));
            default:
                return
                    Visit<Visitor, Var, Current + 1, Top>::execute(
                        std::forward<Visitor>(vi),
                        std::forward<Var>(va)
                    );
        }
    }
};

template<typename Visitor, typename Var, int Top>
struct Visit<Visitor, Var, Top, Top> {
    static decltype(auto) execute(Visitor &&vi, Var &&va) {
        return vi(get<Top>(std::forward<Var>(va)));
    }
};

template<typename Visitor, typename Var>
auto visit(Visitor &&visitor, Var &&var) ->
    decltype(visitor(get<0>(std::forward<Var>(var))))
{
    using V = std::remove_reference_t<Var>;
    return Visit<Visitor, Var, 0, V::Count>::execute(
        std::forward<Visitor>(visitor),
        std::forward<Var>(var)
    );
}

/// \bug Value category not preserved

template<typename R, typename Visitor, typename Var, typename... Ts>
R GCC_visit_injected(Visitor &&visitor, Var &&value, const Variant<Ts...> *) {
    using Vi = decltype(visitor);
    using Va = decltype(value);
    constexpr static R (*visits[1 + sizeof...(Ts)])(Vi, Va) = {
        [](Vi vi, Va va) { return vi(*va.template as<Ts>()); }...,
        [](Vi vi, Va va) { return vi(*va.template as<BadVariant>()); }
    };
    return
        visits[value.typeSwitch_](
            std::forward<Visitor>(visitor), std::forward<Var>(value)
        );
}

template<typename Visitor, typename Var>
auto GCC_visit(Visitor &&visitor, Var &&value) ->
    decltype(visitor(get<0>(value)))
{
    using R = decltype(visitor(get<0>(value)));
    return
        GCC_visit_injected<R>(
            std::forward<Visitor>(visitor),
            std::forward<Var>(value),
            &value
        );
}

}

#endif

