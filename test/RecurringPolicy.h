#ifndef ZOO_TEST_RECURRING_POLICY_H
#define ZOO_TEST_RECURRING_POLICY_H

//
//  Created by Eduardo Madrid on 6/27/19.
//

#include "zoo/AlignedStorage.h"

#include <typeinfo>
#include <utility>

/// \brief Operations is a mixin of operation states and functions
template<typename...Opers>
struct Operations: Opers... {};

namespace op {
struct Destroy {
    void (*destroy)(void *who) noexcept;
};

struct Move {
    void (*move)(void *from, void *to) noexcept;
};

struct Copy {
    void (*copy)(const void *from, void *to);
};

struct Type {
    const std::type_info &(*type)() noexcept;
};
}

template<
    typename Storage, template<class> typename... Affordances
>
// An abbreviation to reduce the noise of the variadic CRTP pattern
#define PP_ZOO_AFFORDANCES Affordances<Container<Storage, Affordances...>>
struct Container: Storage, PP_ZOO_AFFORDANCES... {
    using Ops = Operations<typename PP_ZOO_AFFORDANCES::Operation...>;

    const Ops *vTable_ = &Defaults;

    inline static const Ops Defaults = { PP_ZOO_AFFORDANCES::val... };

    template<typename V>
    inline static const Ops Table = {
        &PP_ZOO_AFFORDANCES::template function<V>...
    };

    bool nonEmpty() const noexcept {
        return &Defaults != vTable_;
    }

    void moveAndDestroy(void *to) {
        this->move(to);
        this->destroy();
    }
};
#undef PP_ZOO_AFFORDANCES

template<typename D>
struct Destroy {
    D *d() { return static_cast<D *>(this); }
    const D *derived() const { return const_cast<Destroy *>(this)->derived(); }

    using Operation = op::Destroy;

    static void doNothing(void *) noexcept {}
    inline static const auto val = &doNothing;

    template<typename V>
    static void function(void *thy) noexcept {
        static_cast<V *>(thy)->~V();
    }

    void destroy() noexcept { this->d()->vTable_->destroy(this->d()); }
};

template<typename D>
struct Move {
    static D *d(const void *p) { return static_cast<D *>(const_cast<void *>(p)); }

    using Operation = op::Move;

    static void moveVTable(void *from, void *to) noexcept {
        d(to)->vTable_ = d(from)->vTable_;
    }
    inline static const auto val = &moveVTable;

    template<typename V>
    static void function(void *source, void *sink) noexcept {
        auto from = static_cast<V *>(source);
        auto to = static_cast<V *>(sink);
        from->move_impl(to);
    }

    void move(void *to) noexcept { d(this)->vTable_->move(d(this), to); }
};

template<typename D>
struct Copy {
    static D *d(const void *p) { return static_cast<D *>(const_cast<void *>(p)); }

    using Operation = op::Copy;

    static void copyVTable(const void *from, void *to) {
        d(to)->vTable_ = d(from)->vTable_;
    }
    inline static const auto val = &copyVTable;

    template<typename V>
    static void function(const void *source, void *sink) {
        auto from = static_cast<const V *>(source);
        auto to = static_cast<V *>(sink);
        from->copy_impl(to);
    }

    void copy(void *to) const { d(this)->vTable_->copy(d(this), to); }
};

template<typename D>
struct Type {
    static D *d(const void *p) { return static_cast<D *>(const_cast<void *>(p)); }

    using Operation = op::Type;

    inline static const auto val = []() noexcept -> decltype(auto) { return typeid(void); };

    template<typename V>
    static const std::type_info &function() noexcept {
        return typeid(typename V::type);
    }

    auto &type() const noexcept { return d(this)->vTable_->type(); }
};

using Goldilocks = zoo::AlignedStorage<1 * zoo::VPSize, zoo::VPAlignment>;
using C = Container<Goldilocks, Destroy, Move, Copy, Type>;

template<typename V>
struct Value: C {
    template<typename... Args>
    Value(Args &&...args) {
        this->template build<V>(std::forward<Args>(args)...);
        this->vTable_ = &C::template Table<Value>;
    }

    ~Value() { this->template as<V>()->~V(); }

    void move_impl(Value *to) noexcept {
        to->template build<V>(std::move(*value()));
        to->vTable_ = this->vTable_;
    }

    V *value() const noexcept { return const_cast<Value *>(this)->template as<V>(); }

    void copy_impl(Value *to) const {
        to->template build<V>(*value());
        to->vTable_ = this->vTable_;
    }

    using type = V;

    constexpr static auto IsReference = false;
};

template<typename V>
struct Reference: C {
    template<typename... Args>
    Reference(Args &&...args) {
        *this->template as<V *>() = new V(std::forward<Args>(args)...);
        this->vTable_ = &C::template Table<Reference>;
    }

    ~Reference() { delete value(); }

    void move_impl(Reference *to) noexcept {
        to->value() = value();
        to->vTable_ = this->vTable_;
        this->vTable_ = &C::Defaults;
    }

    V *&value() const {
        return *const_cast<Reference *>(this)->template as<V *>();
    }

    void copy_impl(Reference *to) const {
        to->value() = new V(*value());
        to->vTable_ = this->vTable_;
    }

    using type = V;

    constexpr static auto IsReference = true;
};

struct RecurringPolicy {
    using MemoryLayout = C;

    template<typename T>
    using Builder =
        std::conditional_t<
            std::is_nothrow_move_constructible_v<T> &&
                sizeof(T) <= C::Size &&
                0 == (C::Alignment % alignof(T)),
            Value<T>,
            Reference<T>
        >;

    template<typename C>
    struct Affordances {
        const std::type_info &type() const noexcept {
            return static_cast<const C *>(this)->container()->type();
        }
    };

    constexpr static auto RequireMoveOnly = true;
};

#endif
