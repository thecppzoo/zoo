#include <tuple>
    // provides std::tuple_element to be able to index a pack of types,
    // indirectly includes type traits and utility
#include <new>

namespace zoo {

template<typename T>
void swap(T &t1, T &t2) {
    T temporary{std::move(t1)};
    t1 = std::move(t2);
    t2 = std::move(temporary);
}

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

struct BadVariant {};

template<int Index, typename... Ts>
using TypeAtIndex =
    typename std::tuple_element<Index, std::tuple<Ts..., BadVariant>>::type;

template<typename R, typename Visitor, typename Var>
R visit(Visitor &&visitor, Var &&value);

template<typename... Ts>
struct Variant {
    constexpr static auto Count = sizeof...(Ts);
    constexpr static auto Size = LargestSize<Ts...>;
    constexpr static auto Alignment = LargestAlignment<Ts...>;
    constexpr static auto NTMC =
        std::conjunction<std::is_nothrow_move_constructible<Ts>...>::value;

    alignas(Alignment) char space_[Size];
    int typeSwitch_ = Count;

    Variant() {}

    Variant(const Variant &v): typeSwitch_{v.typeSwitch_} {
        visit<void>(
            [&](const auto &c) {
                using Source = std::decay_t<decltype(c)>;
                new(as<Source>()) Source{c};
            },
            v
        );
    }

    Variant(Variant &&v) noexcept(NTMC): typeSwitch_{v.typeSwitch_} {
        visit<void>(
            [&](auto &&m) {
                using Source = std::decay_t<decltype(m)>;
                new(as<Source>()) Source{std::move(m)};
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
        swap(*this, copy);
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
        visit<void>(
            [](auto &who) {
                using Type = std::decay_t<decltype(who)>;
                (&who)->~Type();
            },
            *this
        );
    }

};

template<int Index, typename... Ts>
auto get(Variant<Ts...> &v) ->
    TypeAtIndex<Index, std::tuple<Ts..., BadVariant>> &
{
    return v.as<TypeAtIndex<Index, std::tuple<Ts..., BadVariant>>();
}

template<
    typename R, typename Visitor, typename V,
    int Current, typename Head, typename... Tail
>
struct Visit_impl {
    static R execute(Visitor &&visitor, V &&v, int typeSwitch) {
        switch(typeSwitch) {
            case Current: return visitor(*v.template as<Head>());
            default:
                return
                    Visit_impl<
                        R, Visitor, V, Current + 1, Tail...
                    >::execute(
                        std::forward<Visitor>(visitor),
                        std::forward<V>(v),
                        typeSwitch
                    );
        }
    }
};

template<typename R, typename Visitor, typename V, int Current>
struct Visit_impl<R, Visitor, V, Current, BadVariant> {
    static R execute(Visitor &&visitor, V &&v, int) {
        return visitor(*v.template as<BadVariant>());
    }
};

template<typename R, typename Visitor, typename Var, typename... Types>
R visit_injected(Visitor &&visitor, Var &&v, const Variant<Types...> *) {
    return Visit_impl<R, Visitor, Var, 0, Types..., BadVariant>::execute(
        std::forward<Visitor>(visitor),
        std::forward<Var>(v),
        v.typeSwitch_
    );
}

template<typename R, typename Visitor, typename Var>
R visit(Visitor &&visitor, Var &&value) {
    return visit_injected<R>(
        std::forward<Visitor>(visitor), std::forward<Var>(value), &value
    );
}

}
