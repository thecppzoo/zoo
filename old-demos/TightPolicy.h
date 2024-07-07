#pragma once

#ifndef NO_STANDARD_INCLUDES
#include <stdint.h>
#include <string>
#include <algorithm>
#endif

#include <zoo/ConverterAny.h>

constexpr auto VoidPtrSize = sizeof(void *);
constexpr auto VoidPtrAlignment = alignof(void *);

using Fallback =
    zoo::AnyContainer<
        zoo::ConverterPolicy<
            sizeof(std::string), alignof(std::string)
    >>;

struct Empty {
    long dontcare:61;
        // 000: string
        // 001: empty
    bool isNotString:1;
        // 01x
    bool isPointer:1;
        // 1xx
    bool isInteger:1;

    Empty(): isNotString{true}, isPointer{false}, isInteger{false} {}

    using type = void;
};

struct Int63 {
    long integer:63;
    bool outcode:1;

    Int63(long v): outcode(1), integer(v) {}

    operator long() const { return integer; }

    using type = long;
};

struct Pointer62 {
    intptr_t pointer:62;
    unsigned outcode:2;

    Pointer62(void *ptr): outcode{1}, pointer{std::intptr_t(ptr) >> 2} {}

    operator void *() const {
        return reinterpret_cast<void *>(pointer << 2);
    }

    using type = void *;
};

struct String7 {
    char bytes[7];
    struct {
        uint8_t
            remaining:5,
            outcode:3;
    };

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

//private:
    String7(const char *ptr, std::size_t size):
        outcode{0}, remaining{static_cast<uint8_t>(7 - size)}
    {
        std::copy(ptr, size + ptr, bytes);
        bytes[size] = '\0'; // technically UB if 7 == size
        c(POINTER_COUNT);
    }

public:
    template<int L, std::enable_if_t<L < 8, int> = 0>
    String7(const char (&array)[L]): String7(array, L) { c(ARRAY); }

    String7(const std::string &arg):
        String7(arg.data(), arg.size())
    { c(STRING); }

    operator std::string() const {
        return { bytes, bytes + 7 - remaining };
    }

    std::size_t count() const noexcept { return 7 - remaining; }

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

    bool isInteger() const noexcept { return code.empty.isInteger; }
    bool isPointer() const noexcept {
        return !isInteger() && code.empty.isPointer;
    }
    bool isString() const noexcept {
        return !isPointer() && !code.empty.isNotString;
    }

    static_assert(sizeof(code) == VoidPtrSize, "");

    Tight() { code.empty = Empty{}; }

    inline void destroy();
    void copy(Tight *to) const;
    void move(Tight *to) noexcept;
    void moveAndDestroy(Tight *to) noexcept {
        move(to);
    }
    bool nonEmpty() const noexcept {
        return isInteger() || isPointer() || isString();
    }
    void *value();
    const std::type_info &type() const noexcept;
};

static_assert(alignof(Tight) == VoidPtrAlignment, "");

Fallback *fallback(Pointer62 p) {
    void *rv = p;
    return static_cast<Fallback *>(rv);
}

void Tight::destroy() {
    if(!isPointer()) { return; }
    delete fallback(code.pointer);
}

void Tight::copy(Tight *to) const {
    if(!isPointer()) {
        *to = *this;
        return;
    }
    to->code.pointer = new Fallback{*fallback(code.pointer)};
}

void Tight::move(Tight *to) noexcept {
    *to = *this;
    code.empty = Empty{};
}

void *Tight::value() {
    if(!isPointer()) { throw zoo::bad_any_cast{}; }
    return fallback(code.pointer)->container()->value();
}

const std::type_info &Tight::type() const noexcept {
    if(isInteger()) { return typeid(long); }
    if(isPointer()) { return fallback(code.pointer)->type(); }
    if(isString()) { return typeid(String7); }
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
using is_stringy_type =
    typename is_stringy_impl<std::remove_cv_t<std::remove_reference_t<T>>>::type;

template<typename T, typename Void>
template<typename... Args>
Builder<T, Void>::Builder(Args &&... args) {
    auto value =
        new Fallback{std::in_place_type<T>, std::forward<Args>(args)...};
    code.pointer = value;
}

#ifndef TIGHT_CONSTRUCTOR_ANNOTATION
#define TIGHT_CONSTRUCTOR_ANNOTATION(...)
#endif

template<typename T>
struct Builder<T, std::enable_if_t<is_stringy_type<T>::value>>: Tight {
    Builder(const std::string &s) {
        TIGHT_CONSTRUCTOR_ANNOTATION("c str &", &s)
        if(s.size() < 8) {
            code.string = s;
        } else {
            code.pointer = new Fallback{s};
        }
    }

    Builder(std::string &&s) {
        TIGHT_CONSTRUCTOR_ANNOTATION("str &&", &s)
        if(s.size() < 8) {
            code.string = String7{s.data(), s.size()};
        } else {
            code.pointer = new Fallback{std::move(s)};
        }
    }

    template<int L>
    Builder(const char (&array)[L]) {
        TIGHT_CONSTRUCTOR_ANNOTATION("c &[L]", array, L);
        if(L < 8) {
            code.string = String7{array, L};
        } else {
            code.pointer = new Fallback{
                std::in_place_type<std::string>, array, array + L
            };
        }
    }

    Builder(const char *base, std::size_t length) {
        TIGHT_CONSTRUCTOR_ANNOTATION("c *, l", base, length)
        if(length < 8) {
            code.string = String7{base, length};
        } else {
            code.pointer = new Fallback{
                std::in_place_type<std::string>,
                base,
                base + length
            };
        }
    }

    template<typename... Args>
    Builder(Args &&... args):
        Builder{std::string{std::forward<Args>(args)...}}
    {
        TIGHT_CONSTRUCTOR_ANNOTATION("Args &&...", "12345678", sizeof...(Args))
    }
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
std::enable_if_t<
    !is_stringy_type<T>::value && !std::is_integral<T>::value,
    std::decay_t<T> &
> tightCast(TightAny &ta) {
    return
        *zoo::anyContainerCast<std::decay_t<T>>(
            fallback(ta.container()->code.pointer)
        );
}

template<typename T>
std::enable_if_t<is_stringy_type<T>::value, std::string>
tightCast(TightAny &ta) {
    if(ta.container()->isString()) {
        return ta.container()->code.string;
    }
    return *zoo::anyContainerCast<std::string>(fallback(ta.container()->code.pointer));
}

template<typename T>
std::enable_if_t<std::is_integral<T>::value, long>
tightCast(TightAny &ta) { return ta.container()->code.integer; }

template<>
Fallback &tightCast<Fallback>(TightAny &ta) {
    return *fallback(ta.container()->code.pointer);
}

