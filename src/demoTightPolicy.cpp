#include "util/ExtendedAny.h"

#include <stdint.h>
#include <string>

union Tight {
    struct Empty {
        unsigned outcode:3;
            // xx 1: Int63
            // x0 0: Pointer
            // 0 10: Empty
            // 1 10: String7
        long dontcare:61;

        Empty(): outcode{4} {}

        using type = void;
    } empty;

    struct Int63 {
        bool outcode:1;
        long integer:63;

        Int63(long v): outcode(1), integer(v << 1) {}
        operator long() const { return integer >> 1; }

        using type = long;
    } integer;

    struct Pointer62 {
        unsigned outcode:2;
        intptr_t pointer:62;

        Pointer62(void *ptr):
            outcode(0), pointer(~3 & reinterpret_cast<intptr_t>(ptr))
        {}

        operator void *() const {
            return reinterpret_cast<void *>(pointer << 2);
        }

        using type = void *;
    } pointer;

    struct String7 {
        struct {
            unsigned outcode:3;
            unsigned count: 5;
        };
        char bytes[7];

        String7(std::string arg): outcode(6), count(arg.size()) {
            arg.copy(bytes, count);
        }

        operator std::string() const {
            return { bytes, bytes + count };
        }

        using type = std::string;
    } string;

    Tight(): empty{} {}

    template<typename T>
    struct Builder { using type = zoo::ConverterReferential<T>; };
};

template<>
struct Tight::Builder<Tight::Int63> { using type = Int63; };

template<>
struct Tight::Builder<Tight::Pointer62> { using type = Pointer62; };

template<>
struct Tight::Builder<Tight::String7> { using type = String7; };

struct TightPolicy {
    template<typename T>
    using Builder = typename Tight::template Builder<T>::type;
};

auto q(void *p) { return typename TightPolicy::template Builder<Tight::Pointer62>{p}; }
