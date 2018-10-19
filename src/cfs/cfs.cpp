#include <array>

template<typename T, std::size_t S>
T *back_inserter(std::array<T, S> &a) {
    return a.data();
}

#include "cfs.h"

namespace zoo {

template<typename Container>
inline auto reserveIfAvailable(Container &c, unsigned long long s) ->
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

#include <ostream>

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

TEST_CASE("Cache friendly search conversion", "[cfs][conversion]") {
    SECTION("Empty") {
        std::vector<int> empty, output;
        zoo::transformToCFS(back_inserter(output), cbegin(empty), cend(empty));
        REQUIRE(empty == output);
    }
    SECTION("Single element") {
        std::array input{77};
        std::vector<int> output;
        zoo::transformToCFS(back_inserter(output), cbegin(input), cend(input));
        REQUIRE(input == output);
    }
    SECTION("Two elements") {
        const std::array input{77, 88}, expected{88, 77};
        std::vector<int> output;
        zoo::transformToCFS(back_inserter(output), cbegin(input), cend(input));
        REQUIRE(expected == output);
    }
    SECTION("Three elements") {
        const std::array input{77, 88, 99}, expected{88, 77, 99};
        std::vector<int> output;
        zoo::transformToCFS(back_inserter(output), cbegin(input), cend(input));
        REQUIRE(expected == output);
    }
    SECTION("Ten elements (excess of 3)") {
        const std::array
            input{0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
            expected{6, 3, 8, 1, 5, 7, 9, 0, 2, 4};
        std::vector<int> output;
        zoo::transformToCFS(back_inserter(output), cbegin(input), cend(input));
        REQUIRE(expected == output);
    }
    SECTION("Exactly power of 2") {
        const std::array
            input = make_integer_array<8>(),
            expected{4, 2, 6, 1, 3, 5, 7, 0};
        std::vector<int> output;
        zoo::transformToCFS(back_inserter(output), cbegin(input), cend(input));
        REQUIRE(expected == output);
    }
    SECTION("Deficit of 3 to 15 (12 elements, excess of 5 to 7)") {
        const std::array
            input = make_integer_array<12>(),
            expected{7, 3, 10, 1, 5, 9, 11, 0, 2, 4, 6, 8};
            /*  7
                3     10
                1  5  9  11
                02 46 8 */
        std::vector<int> output;
        zoo::transformToCFS(back_inserter(output), cbegin(input), cend(input));
        REQUIRE(expected == output);
        //std::cout << output << std::endl;
    } 
}

TEST_CASE("Cache friendly search lookup", "[cfs][search]") {
    SECTION("Empty search") {
        std::array<int, 0> empty;
        const auto end = cend(empty);
        auto s = zoo::cfsLowerBound(cbegin(empty), end, 1);
        REQUIRE(end == s); 
    }
    SECTION("Search on a single element space") {
        std::array single{1};
        const auto end = cend(single), begin = cbegin(single);
        REQUIRE(zoo::cfsLowerBound(begin, end, 1) == begin);
        REQUIRE(zoo::cfsLowerBound(begin, end, 0) == begin);
        REQUIRE(zoo::cfsLowerBound(begin, end, 2) == end);
    }
    SECTION("General search") {
        //std::array lookup{7, 3, 10, 1, 5, 9, 11, 0, 2, 4, 6, 8};
        std::array lookup{14, 6, 20, 2, 10, 18, 22, 0, 4, 8, 12, 16};
            /*  14
                6         20
                2   10    18 22
                0 4 8  12 16 */
        auto b{cbegin(lookup)}, e{cend(lookup)};
        SECTION("Element present") {
            REQUIRE(18 == *zoo::cfsLowerBound(b, e, 18));
        }
        SECTION("Search for less than minimum") {
            REQUIRE(0 == *zoo::cfsLowerBound(b, e, -1));
        }
        SECTION("Search for higher than maximum") {
            REQUIRE(zoo::cfsLowerBound(b, e, 23) == e);
        }
        SECTION("Search for element higher than high-branch leaf (12)") {
            REQUIRE(14 == *zoo::cfsLowerBound(b, e, 13));
        }
        SECTION("Search for element lower than high-branch leaf (12)") {
            REQUIRE(12 == *zoo::cfsLowerBound(b, e, 11));
        }
        SECTION("Search for element lower than low-branch leaf (16)") {
            REQUIRE(16 == *zoo::cfsLowerBound(b, e, 15));
        }
        SECTION("Search for element higher than low-branch leaf (16)") {
            REQUIRE(18 == *zoo::cfsLowerBound(b, e, 17));
        }
    }
}
