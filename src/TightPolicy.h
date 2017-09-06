#pragma once

#ifndef NO_STANDARD_INCLUDES
#include <stdint.h>
#include <string>
#include <algorithm>
#endif

#include "util/ExtendedAny.h"

constexpr auto VoidPtrSize = sizeof(void *);
constexpr auto VoidPtrAlignment = alignof(void *);

using Fallback =
    zoo::AnyContainer<zoo::ConverterPolicy<VoidPtrSize, VoidPtrAlignment>>;

struct Empty {
    bool isInteger:1;
    bool notPointer:1;
    bool isString:1;
        // xx 1: Int63
        // x0 0: Pointer
        // 0 10: Empty
        // 1 10: String7
    long dontcare:61;

    Empty(): isInteger{false}, notPointer{true}, isString{false} {}

    using type = void;
};

struct Int63 {
    bool outcode:1;
    long integer:63;

    Int63(): outcode(1) {}
    Int63(long v): outcode(1), integer(v) {}

    operator long() const { return integer; }

    using type = long;
};

struct Pointer62 {
    unsigned outcode:2;
    uintptr_t pointer:62;

    Pointer62(): outcode{0} {}
    Pointer62(void *ptr):
        outcode(0), pointer(reinterpret_cast<uintptr_t>(ptr) >> 2)
    {}

    operator void *() const {
        return reinterpret_cast<void *>(pointer << 2);
    }

    using type = void *;
};

struct String7 {
    struct {
        uint8_t
            outcode:3,
            count: 5;
    };
    char bytes[7];

    enum ConstructorOverload {
        POINTER_COUNT,
        DEFAULT,
        ARRAY,
        STRING
    };
    static void c(ConstructorOverload)
    #ifdef STRING7_TESTS
    ;
    #else
    {}
    #endif

    String7(const char *ptr, std::size_t size): outcode(6), count(size) {
        std::copy(ptr, count + ptr, bytes);
        c(POINTER_COUNT);
    }
    String7(): String7(nullptr, 0) { c(DEFAULT); }

    template<int L, std::enable_if_t<L < 8, int> = 0>
    String7(const char (&array)[L]): String7(array, L) { c(ARRAY); }

    String7(const std::string &arg):
        String7(arg.data(), arg.size())
    { c(STRING); }

    operator std::string() const {
        return { bytes, bytes + count };
    }

    using type = std::string;
};

struct Tight { // Why can't you inherit from unions?
    union Encodings {
        Empty empty;
        Int63 integer;
        Pointer62 pointer;
        String7 string;

        Encodings(): empty{} {}
    } code;

    static_assert(sizeof(code) == VoidPtrSize, "");

    Tight() { code.empty = Empty{}; }

    inline void destroy();
    void copy(Tight *to) const;
    void move(Tight *to) noexcept;
    bool nonEmpty() const noexcept {
        auto e = code.empty;
        return !e.isInteger && e.notPointer && !e.isString;
    }
    void *value() noexcept {
        if(!code.empty.isInteger && !code.empty.notPointer) {
            return code.pointer;
        }
        throw;
    }
    const std::type_info &type() const noexcept;
};

static_assert(alignof(Tight) == VoidPtrAlignment, "");

auto isPointer(Tight t) {
    auto e = t.code.empty;
    return !e.isInteger && !e.notPointer;
}

Fallback *fallback(Pointer62 p) {
    void *rv = p;
    return static_cast<Fallback *>(rv);
}

void Tight::destroy() {
    if(!isPointer(*this)) { return; }
    delete fallback(code.pointer);
}

void Tight::copy(Tight *to) const {
    if(!isPointer(*this)) {
        *to = *this;
        return;
    }
    to->code.pointer = new Fallback{*fallback(code.pointer)};
}

void Tight::move(Tight *to) noexcept {
    *to = *this;
    if(isPointer(*this)) {
        code.pointer = nullptr;
    }
}

const std::type_info &Tight::type() const noexcept {
    if(code.empty.isInteger) { return typeid(long); }
    if(!code.empty.notPointer) { fallback(code.pointer)->type(); }
    if(code.empty.isString) { return typeid(std::string); }
    return typeid(void);
}

template<typename T, typename = void>
struct Builder: Tight {
    template<typename... Args>
    inline
    Builder(Args &&... args);
};

template<typename>
struct is_char_array_impl: std::false_type {};

template<int L>
struct is_char_array_impl<char[L]>: std::true_type {};

template<typename>
struct array_length_impl { constexpr static auto value = 0; };

template<typename T, int L>
struct array_length_impl<T[L]> { constexpr static auto value = L; };

template<typename T>
constexpr auto array_length() { return array_length_impl<T>::value; }

template<typename T>
struct is_stringy_impl: std::false_type {};

template<>
struct is_stringy_impl<std::string>: std::true_type {};

template<int L>
struct is_stringy_impl<char[L]>: std::true_type {
    constexpr static int Size = L;
};

template<typename T>
using is_stringy_type = is_stringy_impl<zoo::uncvr_t<T>>;

template<typename T, typename Void>
template<typename... Args>
Builder<T, Void>::Builder(Args &&... args) {
    auto value =
        new Fallback{std::in_place_type<T>, std::forward<Args>(args)...};
    code.pointer = value;
}

template<typename T>
struct Builder<T, std::enable_if_t<is_stringy_type<T>::value>>: Tight {
    Builder(const std::string &s) {
        if(s.size() < 8) {
            code.string = s;
        } else {
            code.pointer = new Fallback{s};
        }
    }

    Builder(std::string &&s) {
        if(s.size() < 8) {
            code.string = String7{s.data(), s.size()};
        } else {
            code.pointer = new Fallback{std::move(s)};
        }
    }

    template<int L>
    Builder(const char (&array)[L]) {
        if(L < 8) {
            code.string = String7{array, L};
        } else {
            code.pointer = new Fallback{
                std::in_place_type<std::string>, array, array + L
            };
        }
    }

    template<typename... Args>
    Builder(Args &&... args):
        Builder{std::string{std::forward<Args>(args)...}}
    {}
};

template<typename T>
struct Builder<T, std::enable_if_t<std::is_integral<T>::value>>: Tight {
    Builder(T arg) { code.integer = arg; }
};

struct TightPolicy {
    using MemoryLayout = Tight;

    template<typename T>
    using Builder = ::Builder<T>;
};

using TightAny = zoo::AnyContainer<TightPolicy>;

template<typename T>
std::decay_t<T> &tightCast(TightAny &ta) {
    return *zoo::anyContainerCast<std::decay_t<T>>(fallback(ta.container()->code.pointer));
}

template<typename T>
std::enable_if_t<is_stringy_type<T>::value, std::string>
tightCast(TightAny &ta) {
    if(ta.container()->code.empty.notPointer) {
        return ta.container()->code.string;
    }
    return *zoo::anyContainerCast<std::string>(fallback(ta.container()->code.pointer));
}

template<typename T>
std::enable_if_t<std::is_integral<T>::value, T>
tightCast(TightAny &ta) { return ta.container()->code.integer; }

