#ifndef SIMPLIFY_PREPROCESSING
#include <zoo/algorithm/quicksort.h>

#include <stdlib.h>

#include "cfs/cfs_utility.h"

#include <benchmark/benchmark.h>
#endif

/// \tparam FI is a forward iterator
/// \pre b != e
template<typename FI, typename Comparison>
auto implicitPivotPartition(FI &begin, FI e, Comparison cmp) ->
    zoo::ImplicitPivotResult<FI> 
{
    using zoo::moveRotation;
    auto bias = 0;
    auto b = begin;
    auto pivot = b++;
    if(e == b) { return {pivot, 0}; }
    auto third = next(b);
    if(e == third) {
        if(cmp(*b, *pivot)) {
            moveRotation(*b, *pivot);
        }
        return {pivot, 0};
    }
    auto fourth = next(third);
    if(e != fourth) { goto resume; }
    if(cmp(*b, *pivot)) { // rearrangement necesary
        // second < first, where does first lie with respect to third?
        if(cmp(*third, *pivot)) { // second < first, third < first
            if(cmp(*b, *third)) { // second < third < first
                moveRotation(*pivot, *b, *third);
            } else { // third <= second < first
                moveRotation(*pivot, *third);
            }
        } else { // second < first <= third
            moveRotation(*pivot, *b);
        }
    } else { // first <= second
        if(cmp(*third, *b)) { // third < second
            if(cmp(*third, *pivot)) { // third < first <= second
                moveRotation(*pivot, *third, *b);
            } else { // first <= third < second
                moveRotation(*b, *third);
            }
        } else { // first <= second <= third
            ;
        }
    }
    begin = third;
    return {third, 0};
    for(; b != e; ++b) {
        resume:
        if(!cmp(*b, *pivot)) { // *pivot <= *b
            ++bias;
            /*auto np = next(pivot);
            if(b == np && 1 < bias) {
                pivot = b;
                --bias;
            }*/
            continue;
        }
        --bias;
        // ..., L1, P == *pivot, G0, G1, ..., Gn, L0 == *b, X0, ...
        // The pivot is greater than the element:
        // insert *b into the lower partition:
        //  1. *b goes into the pivot position
        //  2. the pivot increases by one
        //  3. the element at pivot + 1, the new pivot, must be greater
        // than or equal to any Lx, the value of *pivot satisfies this
        // property.
        // These requirements can be satisfied by rotating the elements
        // at positions (pivot, b, pivot + 1)
        // ..., L1, L0, P == *pivot, G1, ..., Gn, G0 == *b, X0, ...
        auto oldPivot = pivot++;
        if(b == pivot) {
            moveRotation(*oldPivot, *pivot);
        } else {
            moveRotation(*oldPivot, *b, *pivot);
        }
        // tmp = *b, *b = *1, *1 = *0, *0 = tmp
        /*moveRotation(*oldPivot, *b);
        moveRotation(*b, *pivot);*/
    }
    return {pivot, bias};
}

template<typename I, typename Comparison = zoo::Less>
void quicksort(I begin, I end, Comparison cmp = Comparison{}) {
    if(begin == end) { return; }

    constexpr static const auto FrameCount = 64;
    struct Frame {
        I b, e;
    };
    std::array<Frame, FrameCount> stack;
    auto index = 0;

    for(;;) {
        auto result = ::implicitPivotPartition(begin, end, cmp);
        auto pivot = result.pivot_;
        auto higherBegin = next(pivot);
        if(higherBegin == end) { // no higher-recursion needed
            if(begin != pivot) {
                end = pivot; // then just do lower recursion
                continue; // without leaving a frame
            }
            // there is no lower-recursion either
            if(!index) { return; }
            auto &frame = stack[--index];
            begin = frame.b;
            end = frame.e;
            continue;
        }
        // higher recursion needed
        if(begin == pivot) { // no lower recursion needed
            begin = higherBegin; // becomes the higher recursion
            continue;
        }
        // both lower and higher recursions needed, make frame for the larger
        // partition:
        // The smaller partition is less than or equal to half the elements:
        // size(smaller) <= size/2 => depth of recursion <= log2(N)
        if(0 < result.bias_) { // lower partition is smaller
            stack[index] = { higherBegin, end };
            end = pivot;            
        } else { // higher partition is smaller
            stack[index] = { begin, pivot };
            begin = higherBegin;
        }
        if(FrameCount <= ++index) {
            throw std::runtime_error("quicksort stack exhausted");
        }
    }
}

enum Generation {
    PROGRESSIVE, REGRESSIVE, RANDOM
};

template<Generation>
std::vector<int> makeVector(std::size_t);

template<>
std::vector<int> makeVector<PROGRESSIVE>(std::size_t n) {
    std::vector<int> rv;
    rv.resize(n);
    for(int i = 0; i < n; ++i) {
        rv[i] = i;
    }
    return rv;
}

template<>
std::vector<int> makeVector<REGRESSIVE>(std::size_t n) {
    std::vector<int> rv;
    rv.resize(n);
    auto position = 0;
    for(int i = n; i--; ) { rv[position++] = i; }
    return rv;
}

template<>
std::vector<int> makeVector<RANDOM>(std::size_t n) {
    return makeRandomVector(n);
}

template<typename Sorter>
void sort(benchmark::State &s) {
    auto n = s.range(0);
    auto v = makeVector<Sorter::data>(n);
    for(auto _: s) {
        benchmark::DoNotOptimize(v.data());
        auto copy = v;
        Sorter::execute(copy);
        if(
            !zoo::is_sorted(
                cbegin(copy), cend(copy), [](auto a, auto b) { return a <= b; }
        )) {
            throw std::runtime_error{"Not sorted"};
        }
        benchmark::DoNotOptimize(copy.data());
    }
    s.SetComplexityN(n);
}

template<typename FI>
void execQsort(FI b, FI e) {
    qsort(
        &*b, e - b, sizeof(*b),
        [](auto l, auto r) {
            return *static_cast<const int *>(l) - *static_cast<const int *>(r);
        }
    );
}

#define Sorters \
    X(Local, quicksort)\
    X(Canonical,zoo::quicksort)\
    X(STL, std::sort)\
    X(Qsort, execQsort)

#define X(name, function)\
template<Generation G>\
struct Sorter##name {\
    constexpr static auto data = G;\
    template<typename T>\
    static void execute(T &c) {\
        function(begin(c), end(c));\
    }\
};

Sorters
#undef X

#define BMARK_GEN(name, generation, mult, start, end) \
    BENCHMARK_TEMPLATE(sort, Sorter##name<generation>)\
        ->RangeMultiplier(mult)\
        ->Range(start, end)\
        ->Unit(benchmark::kMicrosecond)\
        ->Complexity();

#define GENERATION_STYLES(x) Y(x, PROGRESSIVE)Y(x, REGRESSIVE)Y(x, RANDOM)

#define Y(name, style) BMARK_GEN(name, style, 4, 1 << 4, 1 << 18)

#define X(name, _) GENERATION_STYLES(name)

Sorters
#undef Y
#undef X