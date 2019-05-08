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

template<typename R, typename Visitor, typename Var>
R GCC_visit(Visitor &&visitor, Var &&value) {
    return
        GCC_visit_injected<R>(
            std::forward<Visitor>(visitor),
            std::forward<Var>(value),
            &value
        );
}

}

#include <catch2/catch.hpp>

struct HasDestructor {
    int *ip_;
    HasDestructor(int *ip): ip_{ip} { *ip_ = 1; }
    ~HasDestructor() { *ip_ = 0; }
};

struct IntReturns1 {
    template<typename T>
    int operator()(T) { return 0; }
    int operator()(int) { return 1; }
};

struct MoveThrows {
    MoveThrows() = default;
    MoveThrows(const MoveThrows &) = default;
    MoveThrows(MoveThrows &&) noexcept(false);
};

TEST_CASE("Variant", "[variant]") {
    int value = 4;
    static_assert(
        std::is_nothrow_move_constructible<zoo::Variant<int, char>>::value,
        ""
    );
    static_assert(
        !std::is_nothrow_move_constructible<
            zoo::Variant<int, MoveThrows, char>
        >::value,
        ""
    );
    using V = zoo::Variant<int, HasDestructor>;
    SECTION("Proper construction") {
        V var{std::in_place_index_t<0>{}, 77};
        REQUIRE(77 == *var.as<int>());
    }
    SECTION("Destructor Called") {
        {
            V var{std::in_place_index_t<1>{}, &value};
            REQUIRE(1 == value);
        }
        REQUIRE(0 == value);
    }
    SECTION("visit") {
        V instance;
        auto result = zoo::visit<int>(IntReturns1{}, instance);
        REQUIRE(0 == result);
        instance = V{std::in_place_index_t<1>{}, &value};
        result = zoo::visit<int>(IntReturns1{}, instance);
        REQUIRE(0 == result);
        value = 5;
        instance = V{std::in_place_index_t<0>{}, 77};
        SECTION("Internally held object is destroyed on move assignment") {
            REQUIRE(0 == value);
        }
        result = zoo::visit<int>(IntReturns1{}, instance);
        REQUIRE(1 == result);
    }
    SECTION("GCC Visit") {
        V instance;
        REQUIRE(0 == zoo::GCC_visit<int>(IntReturns1{}, instance));
        instance = V{std::in_place_index_t<0>{}, 99};
        REQUIRE(1 == zoo::GCC_visit<int>(IntReturns1{}, instance));
    }
}
