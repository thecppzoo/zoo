#include <zoo/algorithm/quicksort.h>
#include <zoo/slist.h>
#include <zoo/util/range_equivalence.h>

using zoo::operator==;

#include <catch2/catch.hpp>

#include <vector>

struct TracesMoves {
    int rank_, data_;
    bool moved = false;

    TracesMoves() = default;
    TracesMoves(const TracesMoves &) = default;
    TracesMoves &operator=(const TracesMoves &) = default;

    TracesMoves(TracesMoves &&other): TracesMoves{other} {
        if(other.moved) {
            throw std::runtime_error{"Constructing from already moved"};
        }
        other.moved = true;
    }

    TracesMoves(int rank, int data): rank_{rank}, data_{data} {}

    TracesMoves &operator=(TracesMoves &&other) {
        if(other.moved) {
            throw std::runtime_error{"Assigned from already moved"};
        }
        *this = other;
        other.moved = true;
        return *this;
    }

    bool operator<(TracesMoves other) const {
        return rank_ < other.rank_;
    }
};

TEST_CASE("quicksort") {
    using VInt = std::vector<int>;
    SECTION("Edge cases") {
        SECTION("Empty") {
            VInt empty;
            zoo::quicksort(begin(empty), end(empty));
        }
        SECTION("Single") {
            VInt single = {7};
            zoo::quicksort(begin(single), end(single));
            REQUIRE(7 == *single.begin());
        }
    }
    SECTION("General") {
        SECTION("Already ordered") {
            VInt ordered = {1, 2};
            zoo::quicksort(begin(ordered), end(ordered));
            CHECK(1 == ordered[0]);
            CHECK(2 == ordered[1]);
        }
        SECTION("Not ordered") {
            VInt unordered = {2, 1};
            zoo::quicksort(begin(unordered), end(unordered));
            CHECK(1 == unordered[0]);
            CHECK(2 == unordered[1]);
        }
        SECTION("Repeated") {
            VInt
                repeated = { 1, 7, 5, 5, 5, 0 },
                expected = { 0, 1, 5, 5, 5, 7 };
            zoo::quicksort(begin(repeated), end(repeated));
            CHECK(repeated == expected);
        }
        SECTION("Repeated 2") {
            VInt
                repeated = { 1, 5, 7, 5, 0, 5, 3 },
                expected = { 0, 1, 3, 5, 5, 5, 7 };
            zoo::quicksort(begin(repeated), end(repeated));
            CHECK(repeated == expected);
        }
        SECTION("Large") {
            VInt
                large = { 7, 3, 1, 0, 5, 8, 2, 4, 9, 6 },
                expected = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
            zoo::quicksort(begin(large), end(large));
            CHECK(large == expected);
        }
    }
    SECTION("Regressions") {
        SECTION("Does not move already moved") {
            std::vector<TracesMoves>
                original{{2, 2}, {1, 1}},
                expected{{1, 0}, {2, 0}};
            zoo::quicksort(begin(original), end(original));
            CHECK(zoo::weaklySame(original, expected));
        }
    }
}
