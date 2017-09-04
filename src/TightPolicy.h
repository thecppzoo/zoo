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

    String7(): outcode{6}, count{0} {}
    String7(const char *ptr, int size): outcode(6), count(size) {
        std::copy(ptr, count + ptr, bytes);
    }
    String7(const std::string &arg): String7(arg.data(), arg.size()) {}

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
};

static_assert(alignof(Tight) == VoidPtrAlignment, "");

template<typename T, typename = void>
struct Builder: Tight {
    template<typename... Args>
    inline
    Builder(Args &&... args);
};

template<typename T>
struct is_stringy_type: std::false_type {};

template<>
struct is_stringy_type<std::string>: std::true_type {};

template<int L>
struct is_stringy_type<char[L]>: std::true_type {
    constexpr static int Size = L;
};

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
            code.string = String7{s};
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
