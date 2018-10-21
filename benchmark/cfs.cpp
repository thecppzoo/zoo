#include "cfs/cfs_utility.h"

#include <zoo/algorithm/cfs.h>
#include <algorithm>

#include <benchmark/benchmark.h>

void transformation(benchmark::State &s) {
    auto n = s.range(0);
    auto lin = linear_vector(n - 1);
    std::vector<int> converted;
    for(auto _: s) {
        benchmark::DoNotOptimize(lin.data());
        zoo::transformToCFS(back_inserter(converted), cbegin(lin), cend(lin));
        benchmark::DoNotOptimize(converted.data());
        converted.clear();
    }
    s.SetComplexityN(s.range(0));
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
}

void justARandomKey(benchmark::State &s) {
    for(auto _: s) {
        randomTwo30();
    }
}

void justVisitElement(benchmark::State &s) {
    auto n = s.range(0);
    auto spc = makeRandomVector(n);
    auto z = spc.begin();
    for(auto _: s) {
        benchmark::DoNotOptimize(z++);
    }
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
}

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

struct UseCFS {
    static auto makeSpace(int q) {
        auto raw = makeRandomVector(q);
        auto b{begin(raw)}, e{end(raw)};
        std::sort(b, e);
        std::vector<int> rv;
        rv.reserve(q);
        zoo::transformToCFS(back_inserter(rv), b, e);
        return rv;
    };

    template<typename I, typename E>
    static auto search(I b, I e, const E &v) {
        return zoo::cfsLowerBound(b, e, v);
    }
};

void searchCFS(benchmark::State &s) {
    search<UseCFS>(s);
}

void searchSTL(benchmark::State &s) {
    search<UseSTL>(s);
}

BENCHMARK(justARandomKey)->Range(1 << 15, 1 << 27);//->Unit(benchmark::kMicrosecond);
BENCHMARK(justVisitElement)->Range(1 << 15, 1 << 27);//->Unit(benchmark::kMicrosecond);
BENCHMARK(searchSTL)->Range(1 << 15, 1 << 27);//->Unit(benchmark::kMicrosecond);
BENCHMARK(searchCFS)->Range(1 << 15, 1 << 27);//->Unit(benchmark::kMicrosecond);
BENCHMARK(genLinearVector)->Range(1 << 15, 1 << 27)->Unit(benchmark::kMicrosecond);
BENCHMARK(randomVector)->Range(1 << 15, 1 << 27)->Unit(benchmark::kMicrosecond);
BENCHMARK(sortRandomVector)->Range(1 << 15, 1 << 27)->Unit(benchmark::kMicrosecond);
BENCHMARK(transformation)->Range(1 << 15, 1 << 27)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();

