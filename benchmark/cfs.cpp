#include <zoo/algorithm/cfs.h>
#include <zoo/algorithm/quicksort.h>

#include <vector>

std::vector<int> linear_vector(int n) {
    std::vector<int> rv;
    rv.reserve(n);
    for(auto i = 0; i < n; ++i) {
        rv.push_back(i);
    }
    return rv;
}


#include <iterator>
#include <random>
#include <algorithm>
#include "benchmark/benchmark.h"

void transformation(benchmark::State &s) {
    auto n = s.range(0);
    auto lin = linear_vector(n - 1);
    std::vector<int> converted;
    for(auto _: s) {
        zoo::transformToCFS(back_inserter(converted), cbegin(lin), cend(lin));
        converted.clear();
    }
    s.SetComplexityN(s.range(0));
}

void genV(benchmark::State &s) {
    auto n = s.range(0);
    for(auto _: s) {
        linear_vector(n);
    }
    s.SetComplexityN(s.range(0));
}

std::random_device rdevice;
std::mt19937 gen(rdevice());
std::uniform_int_distribution<int> dist(0, (1 << 30) - 1); // 16 M

std::vector<int> make_vector(int size) {
    std::vector<int> v;
    v.reserve(size);
    for(auto i = size; i--; ) {
        v.push_back(dist(gen));
    }
    return v;
}

std::vector<int> make_vector_nr(int size) {
    std::vector<int> v;
    for(auto i = size; i--; ) {
        v.push_back(dist(gen));
    }
    return v;
}

void randomVector(benchmark::State &s) {
    auto n = s.range(0);
    for(auto _: s) {
        auto v = make_vector(n);
    }
    s.SetComplexityN(n);
}

void randomVectorNR(benchmark::State &s) {
    auto n = s.range(0);
    for(auto _: s) {
        auto v = make_vector_nr(n);
    }
    s.SetComplexityN(n);   
}

void sortRandomVector(benchmark::State &s) {
    auto n = s.range(0);
    auto v = make_vector(n);
    for(auto _: s) {
        auto copy = v;
        std::sort(begin(copy), end(copy));
        auto cs = 0;
        for(auto e: copy) { cs ^= e; }
        benchmark::DoNotOptimize(cs);
    }
}

#include <stdlib.h>

void useQSort(benchmark::State &s) {
    auto n = s.range(0);
    auto v = make_vector(n);
    for(auto _: s) {
        auto copy = v;
        qsort(
            &v[0], n, sizeof(int),
            [](const void *l, const void *r) {
                return *static_cast<const int *>(l) - *static_cast<const int *>(r);
            }
        );
        auto cs = 0;
        for(auto e: copy) { cs ^= e; }
        benchmark::DoNotOptimize(cs);
    }
}

void useQuick(benchmark::State &s) {
    auto n = s.range(0);
    for(auto _: s) {
        auto copy = make_vector(n);
        quicksort(begin(copy), end(copy));
        auto cs = 0;
        for(auto e: copy) { cs ^= e; }
        benchmark::DoNotOptimize(cs);
    }
}

void useQuickCopyV(benchmark::State &s) {
    auto n = s.range(0);
    auto v = make_vector(n);
    for(auto _: s) {
        auto copy = v;
        quicksort(begin(copy), end(copy));
        auto cs = 0;
        for(auto e: copy) { cs ^= e; }
        benchmark::DoNotOptimize(cs);
    }
}

//BENCHMARK(randomVector)->Range(1 << 20, 1 << 26)->Complexity(benchmark::oN);
//BENCHMARK(randomVectorNR)->Range(1 << 20, 1 << 26)->Complexity(benchmark::oN);
//BENCHMARK(genV)->Range(1 << 20, 1 << 26)->Complexity(benchmark::oN);
//BENCHMARK(transformation)->Range(1 << 15, 1 << 24)->Complexity(benchmark::oN);
BENCHMARK(useQuickCopyV)
    ->Range(1 << 15, 1 << 27)->Complexity(benchmark::oNLogN)->Unit(benchmark::kMicrosecond);

BENCHMARK(useQuick)
    ->Range(1 << 15, 1 << 27)->Complexity(benchmark::oNLogN)->Unit(benchmark::kMicrosecond);

BENCHMARK(useQSort)
    ->Range(1 << 15, 1 << 27)->Complexity(benchmark::oNLogN)->Unit(benchmark::kMicrosecond);

BENCHMARK(sortRandomVector)
    ->Range(1 << 15, 1 << 27)->Complexity(benchmark::oNLogN)
    //->Unit(benchmark::kMillisecond)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();

