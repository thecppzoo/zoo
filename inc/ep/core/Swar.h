#pragma once

/// \file Swar.h SWAR operations

#include <stdint.h>

namespace ep { namespace core {

/// Repeats the given pattern in the whole of the argument
/// \tparam T the desired integral type
/// \tparam Progression how big the pattern is
/// \tparam Remaining how many more times to copy the pattern
template<typename T, int Progression, int Remaining> struct BitmaskMaker {
    constexpr static T repeat(T v) {
        return
            BitmaskMaker<T, Progression, Remaining - 1>::repeat(v | (v << Progression));
    }
};
template<typename T, int P> struct BitmaskMaker<T, P, 0> {
    constexpr static T repeat(T v) { return v; }
};

/// Front end to \c BitmaskMaker with the repeating count set to the whole size
template<int size, typename T>
constexpr T makeBitmask(T v) {
    return BitmaskMaker<T, size, sizeof(T)*8/size>::repeat(v);
}

/// Core implementation details
namespace detail {
    //template<int level> 
    constexpr auto b0 = makeBitmask<2>(1ull);
    constexpr auto b1 = makeBitmask<4>(3ull);
    constexpr auto b2 = makeBitmask<8>(0xFull);
    constexpr auto b3 = makeBitmask<16>(0xFFull);
    constexpr auto multiplierNibble = makeBitmask<4>(1ull);
}

constexpr uint64_t bibitPopCounts(uint64_t v) {
    return v - ((v >> 1) & detail::b0);
}

constexpr uint64_t nibblePopCounts(uint64_t c) {   
    return ((c >> 2) & detail::b1) + (c & detail::b1);
}

constexpr uint64_t bytePopCounts(uint64_t v) {
    return ((v >> 4) & detail::b2) + (v & detail::b2);
}

constexpr uint64_t popCounts16bit(uint64_t v) {
    return ((v >> 8) & detail::b3) + (v & detail::b3);
}

auto what = nibblePopCounts(0xF754);

}}
