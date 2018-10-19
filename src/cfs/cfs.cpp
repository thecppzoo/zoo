#include <iostream>

#include <array>

template<typename T, std::size_t S>
T *back_inserter(std::array<T, S> &a) {
    return a.data();
}

#ifndef ZOO_CFS_CACHE_FRIENDLY_SEARCH
#define ZOO_CFS_CACHE_FRIENDLY_SEARCH

#include <algorithm>

namespace zoo {

constexpr unsigned long long log2Floor(unsigned long long arg) {
    return 63 - __builtin_clzll(arg);
}

constexpr unsigned long long log2Ceiling(unsigned long long arg) {
    return 63 - __builtin_clzll(2*arg - 1);
}

template<typename Output, typename Input>
void transform(Output output, Input base, Input end) {
    auto s = end - base;
    if(2 == s) {
        while(s--) {
            *output++ = *--end;
        }
        return;
    }
    auto logP = log2Floor(s + 1); // n
    auto power2 = 1ul << logP;
    auto fullSubtreeSize = power2 - 1;
    // Full tree has (2^n) - 1 elements
    auto excess = s - fullSubtreeSize;
    auto twiceExcess = excess << 1;
    for(auto straddle = power2; 1 < straddle; ) {
        auto nextStraddle = straddle >> 1;
        auto unshifted = nextStraddle - 1;
        auto shifted = unshifted + excess;
        if(shifted < twiceExcess) { // below the "cut"
            auto i = straddle - 1;
            auto prevStraddle = straddle << 1;
            auto addendum = 0;
            do {
                *output++ = *(base + i + addendum);
                addendum += prevStraddle;
            } while(i + addendum < twiceExcess);
            auto halvedAddendum = addendum >> 1;
            shifted += halvedAddendum;
        }
        while(shifted < s) {
            *output++ = *(base + shifted);
            shifted += straddle;
        }
        straddle = nextStraddle;
    }

    // now just write the excess leaves
    for(auto ndx = 0ul, top = 2*excess; ndx < top; ndx += 2) {
        *output++ = *(base + ndx);
    }
}
// s = 0, n = 0, straddle = 1, nothing more
// s = 1, n = 1, p2 = 2, e = 0, straddle = 2, ns = 1, copy(0)
// s = 2, n = 1, p2 = 2, e = 1, e2 = 2, straddle = 2, ns = 1, us = 1, sh = 2
// 6
// 3    8
// 1  5 7 9
// 02 4
// Example: s = 10, logP = 3, straddle = 8, fs = 7, e = 3, e2 = 6
// Iteration: ns = 4, us = 3, sh = 6.
    // not(sh < e2)
    // copy(6)
    // straddle = 4
    // ns = 2, us = 1, sh = 4
    // sh < e2
        // i = 3, ps = 8, add = 0.  Copy(3), add = 8, exit
        // ha = 4, sh = 8
    // copy(8)
    // straddle = 2
    // ns = 1, us = 0, sh = 3
    // sh < e2
        // i = 1, ps = 4, add = 0
        // copy(1), add = 4, copy(5), add = 8, exit
        // ha = 4, sh = 7
    // copy(7), copy(9)

template<typename Container>
inline auto reserveIfAvailable(Container &c, std::size_t s) ->
decltype(c.reserve(s), (void)0) {
    c.reserve(s);
}
inline void reserveIfAvailable(...) {}

template<typename Sorted>
Sorted sortedToCacheFriendlySearch(const Sorted &s) {
    Sorted rv;
    reserveIfAvailable(rv, s.size());
    sortedToCacheFriendlySearch(back_inserter(rv), 0, s);
    return rv;
}


}

#endif

#include <ostream>

#include <type_traits>

namespace zoo {

template<typename...>
using void_t = void;

template<typename T, typename = void>
struct is_container_impl: std::false_type {};
template<typename T>
struct is_container_impl<
    T,
    void_t<
        decltype(std::declval<T>().cbegin()),
        decltype(std::declval<T>().cend())
    >
>: std::true_type {};

template<typename T>
constexpr auto is_container_v = is_container_impl<T>::value;

}

namespace std {
template<typename C>
auto operator<<(std::ostream &out, const C &a)
-> std::enable_if_t<zoo::is_container_v<C>, std::ostream &>
{
    out << '(';
    auto current{cbegin(a)}, sentry{cend(a)};
    if(current != sentry) {
        for(;;) {
            out << *current++;
            if(sentry == current) { break; }
            out << ", ";
        }
    }
    return out << ')';
}
}

template<typename C1, typename C2>
auto operator==(const C1 &l, const C2 &r)
-> std::enable_if_t<
    zoo::is_container_v<C1> and
        zoo::is_container_v<C2>,
    bool
>
{
    auto lb{cbegin(l)}, le{cend(l)};
    auto rb{cbegin(r)}, re{cend(r)};
    for(;;++lb, ++rb){
        if(lb == le) { return rb == re; } // termination at the same time
        if(rb == re) { return false; } // r has fewer elements
        if(not(*lb == *rb)) { return false; }
    }
    return true;
}

#include <catch.hpp>

#include <iostream>
#include <vector>
#include <utility>

template<int... Pack>
auto indices_to_array(std::integer_sequence<int, Pack...>)
{
    return std::array{Pack...};
}

template<int Size>
std::array<int, Size> make_integer_array() {
    return indices_to_array(std::make_integer_sequence<int, Size>{});
}

TEST_CASE("CacheFriendlySearch", "[array]") {
    SECTION("Empty") {
        std::vector<int> empty, output;
        zoo::transform(back_inserter(output), cbegin(empty), cend(empty));
        REQUIRE(empty == output);
    }
    SECTION("Single element") {
        std::array input{77};
        std::vector<int> output;
        zoo::transform(back_inserter(output), cbegin(input), cend(input));
        REQUIRE(input == output);
    }
    SECTION("Two elements") {
        const std::array input{77, 88}, expected{88, 77};
        std::vector<int> output;
        zoo::transform(back_inserter(output), cbegin(input), cend(input));
        REQUIRE(expected == output);
    }
    SECTION("Three elements") {
        const std::array input{77, 88, 99}, expected{88, 77, 99};
        std::vector<int> output;
        zoo::transform(back_inserter(output), cbegin(input), cend(input));
        REQUIRE(expected == output);
    }
    SECTION("Ten elements (excess of 3)") {
        const std::array
            input{0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
            expected{6, 3, 8, 1, 5, 7, 9, 0, 2, 4};
        std::vector<int> output;
        zoo::transform(back_inserter(output), cbegin(input), cend(input));
        REQUIRE(expected == output);
    }
    SECTION("Exactly power of 2") {
        const std::array
            input = make_integer_array<8>(),
            expected{4, 2, 6, 1, 3, 5, 7, 0};
        std::vector<int> output;
        zoo::transform(back_inserter(output), cbegin(input), cend(input));
        REQUIRE(expected == output);
    }
    SECTION("Deficit of 3 (12 elements)") {
        const std::array
            input = make_integer_array<12>(),
            expected{7, 3, 10, 1, 5, 9, 11, 0, 2, 4, 6, 8};
            /*  7
                3     10
                1  5  9  11
                02 46 8 */
        std::vector<int> output;
        zoo::transform(back_inserter(output), cbegin(input), cend(input));
        REQUIRE(expected == output);
        std::cout << output << std::endl;
    } 
}
