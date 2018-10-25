#include "cfs/cfs_utility.h"

#include <zoo/algorithm/cfs.h>
#include <algorithm>

#include <benchmark/benchmark.h>

void transformationToCFS(benchmark::State &s) {
    auto n = s.range(0);
    auto lin = linear_vector(n - 1);
    std::vector<int> converted;
    for(auto _: s) {
        benchmark::DoNotOptimize(lin.data());
        zoo::transformToCFS(back_inserter(converted), cbegin(lin), cend(lin));
        benchmark::DoNotOptimize(converted.data());
        converted.clear();
    }
    s.SetComplexityN(n);
}

void genLinearVector(benchmark::State &s) {
    auto n = s.range(0);
    for(auto _: s) {
        auto v = linear_vector(n);
        benchmark::DoNotOptimize(v.data());
    }
    s.SetComplexityN(s.range(0));
}

void randomVector(benchmark::State &s) {
    auto n = s.range(0);
    for(auto _: s) {
        auto v = makeRandomVector(n);
        benchmark::DoNotOptimize(v.back());
    }
    s.SetComplexityN(n);
}

void sortRandomVector(benchmark::State &s) {
    auto n = s.range(0);
    auto v = makeRandomVector(n);
    for(auto _: s) {
        benchmark::DoNotOptimize(v.data());
        auto copy = v;
        std::sort(begin(copy), end(copy));
        benchmark::DoNotOptimize(copy.data());
    }
    s.SetComplexityN(n);
}

void justARandomKeyCallingOpaque(benchmark::State &s) {
    for(auto _: s) {
        randomTwo30();
    }
}

void justARandomKey(benchmark::State &s) {
    for(auto _: s) {
        for(int i = 1000; i--; ) {
            dist(gen);
        }
    }
}

void justTraversingRandomVector(benchmark::State &s) {
    auto n = s.range(0);
    auto spc = makeRandomVector(n);
    auto acc = 0;
    auto ndx = 0;
    for(auto _: s) {
        for(auto i = 1000; i--; ) {
            acc ^= spc[ndx++];
            if(ndx == n) { ndx = 0; }
        }
    }
    auto cutDependencies = acc;
    benchmark::DoNotOptimize(cutDependencies);
    s.SetComplexityN(n);
}

template<typename F>
void search(benchmark::State &s) {
    auto n = s.range(0);
    auto space = F::makeSpace(n);
    auto b{cbegin(space)}, e{cend(space)};
    constexpr auto mask = (1 << 20) - 1;
    auto keys = makeRandomVector(mask + 1);
    auto kNdx = 0;
    auto found = 0, searched = 0;
    for(auto _: s) {
        auto k = keys[kNdx++];
        auto r = F::search(b, e, k);
        if(e != r && k == *r) { ++found; }
        ++searched;
        kNdx &= mask;
        benchmark::DoNotOptimize(r);
    }
    s.counters["found"] = found;
    s.counters["searched"] = searched;
    s.SetComplexityN(n);
}

struct UseLinear {
    static auto makeSpace(int q) {
        return makeRandomVector(q);
    }

    template<typename I, typename E>
    static auto search(I b, I e, const E &v) {
        return std::find(b, e, v);
    }
};

struct UseSTL {
    static auto makeSpace(int q) {
        auto rv = makeRandomVector(q);
        std::sort(begin(rv), end(rv));
        return rv;
    };

    template<typename I, typename E>
    static auto search(I b, I e, const E &v) {
        return std::lower_bound(b, e, v);
    }
};

struct UseCFSLowerBound {
    static auto makeSpace(int q) {
        auto raw = makeRandomVector(q);
        auto b{begin(raw)}, e{end(raw)};
        std::sort(b, e);
        std::vector<int> rv;
        rv.reserve(q);
        zoo::transformToCFS(back_inserter(rv), b, e);
        if(not(zoo::validHeap(rv))) {
            throw;
        }
        return rv;
    };

    template<typename I, typename E>
    static auto search(I b, I e, const E &v) {
        return zoo::cfsLowerBound(b, e, v);
    }
};

struct UseCFSSearch: UseCFSLowerBound {
    template<typename I, typename E>
    static auto search(I b, I e, const E &v) {
        return zoo::cfsSearch(b, e, v);
    }
};

void searchCFSLate(benchmark::State &s) {
    search<UseCFSLowerBound>(s);
}

void searchCFSEarly(benchmark::State &s) {
    search<UseCFSSearch>(s);
}

void searchSTL(benchmark::State &s) {
    search<UseSTL>(s);
}

void searchLinear(benchmark::State &s) {
    search<UseLinear>(s);
}

struct CacheLine {
    union {
        alignas(64) int value;
        char space[64];
    };

    CacheLine() = default;
    CacheLine(int v): value{v} {}

    bool operator<(const CacheLine &other) const {
        return value < other.value;
    }

    CacheLine &operator=(int v) { value = v; return *this; }
};

bool operator==(int v, const CacheLine &o) { return v == o.value; }

auto makeSortedCacheLineSpace(int q) {
    auto values = makeRandomVector(q);
    std::vector<CacheLine> rv;
    rv.reserve(q);
    auto bi{back_inserter(rv)};
    for(auto v: values) {
        *bi++ = v;
    }
    std::sort(begin(rv), end(rv));
    return rv;
}

struct UseCacheLineSTL: UseSTL {
    static auto makeSpace(int q) { return makeSortedCacheLineSpace(q); }
};

struct UseCacheLineCFS: UseCFSLowerBound {
    static auto makeSpace(int q) {
        auto sorted = makeSortedCacheLineSpace(q);
        std::vector<CacheLine> rv;
        rv.reserve(q);
        zoo::transformToCFS(back_inserter(rv), sorted.cbegin(), sorted.cend());
        return rv;
    }
};

void searchCacheLineSTL(benchmark::State &s) {
    search<UseCacheLineSTL>(s);
}

void searchCacheLineCFS(benchmark::State &s) {
    search<UseCacheLineCFS>(s);
}

static_assert(64 == sizeof(CacheLine));
static_assert(64 == alignof(CacheLine));

constexpr auto RangeLow = 1 << 16, RangeHigh = 1 << 28;

BENCHMARK(searchCacheLineSTL)->RangeMultiplier(4)->Range(RangeLow/16, RangeHigh/16)->Complexity();
BENCHMARK(searchCacheLineCFS)->RangeMultiplier(4)->Range(RangeLow/16, RangeHigh/16)->Complexity();

BENCHMARK(justARandomKey)->Unit(benchmark::kMicrosecond);
BENCHMARK(justARandomKeyCallingOpaque);
BENCHMARK(justTraversingRandomVector)->RangeMultiplier(4)->Range(RangeLow, RangeHigh)->Complexity();
BENCHMARK(searchLinear)->RangeMultiplier(4)->Range(RangeLow, RangeHigh)->Complexity();
BENCHMARK(searchSTL)->RangeMultiplier(4)->Range(RangeLow, RangeHigh)->Complexity();
BENCHMARK(searchCFSLate)->RangeMultiplier(4)->Range(RangeLow, RangeHigh)->Complexity();
BENCHMARK(searchCFSEarly)->RangeMultiplier(4)->Range(RangeLow, RangeHigh)->Complexity();
BENCHMARK(genLinearVector)->RangeMultiplier(4)->Range(RangeLow, RangeHigh)->Unit(benchmark::kMicrosecond)->Complexity();
BENCHMARK(randomVector)->RangeMultiplier(4)->Range(RangeLow, RangeHigh)->Unit(benchmark::kMicrosecond)->Complexity();
BENCHMARK(sortRandomVector)->RangeMultiplier(4)->Range(RangeLow, RangeHigh)->Unit(benchmark::kMicrosecond)->Complexity();
BENCHMARK(transformationToCFS)->RangeMultiplier(4)->Range(RangeLow, RangeHigh)->Unit(benchmark::kMicrosecond)->Complexity();

BENCHMARK_MAIN();

