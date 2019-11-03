#include "zoo/AnyCallable.h"
#include "zoo/function.h"

#if defined(USE_FOLLY) && USE_FOLLY
#include "folly/Function.h"
#else
namespace folly {
template<typename S>
using Function = std::function<S>;
}
#endif

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "catch2/catch.hpp"

#include <random>
#include <vector>

using Signature = long(long, long, int *, int *);

long function1(long arg1, long arg2, int *p1, int *p2) {
    return arg1 ^ arg2 ^ (p1 - p2);
}

long function2(long arg1, long arg2, int *p1, int *p2) {
    return p1 - p2 + arg1 - arg2;
}

constexpr auto
    VectorSize = 1021, // a prime number to minimize branch prediction
    Reiterations = 11;

template<typename F>
auto generateFunctions(int seed, int top) {
    std::vector<F> rv;
    std::minstd_rand rng(seed);
    std::uniform_int_distribution<> dis(0, top);
    for(auto i = VectorSize; i--; ) {
        rv.push_back(dis(rng) ? &function1 : &function2);
    }
    return rv;
}

template<typename V>
long evaluate(V &v) {
    long l1 = 0, l2 = 0;
    int i1 = 0, i2 = 0, *p1 = &i1, *p2 = &i2;
    for(auto n = Reiterations; n--; ) {
        for(auto &e: v) {
            l1 = e(l1, l2, p1, p2);
        }
    }
    return l1;
}

template<typename Seed>
void functionBenchmarks(Seed seed, int top) {
    auto z = generateFunctions<zoo::function<Signature>>(seed, top);
    auto s = generateFunctions<std::function<Signature>>(seed, top);
    auto fo = generateFunctions<folly::Function<Signature>>(seed, top);
    auto fp = generateFunctions<Signature *>(seed, top);

    BENCHMARK("folly") {
        return evaluate(fo);
    };
    BENCHMARK("zoo") {
        return evaluate(z);
    };
    BENCHMARK("std") {
        return evaluate(s);
    };
    BENCHMARK("fp") {
        return evaluate(fp);
    };
}

TEST_CASE("Function Benchmark", "[benchmark][zoo][api][callable][type-erasure]") {
    std::random_device trueRandom;
    auto seed = trueRandom();
    SECTION("Maximum entropy, the probability of each target is 0.5") {
        functionBenchmarks(seed, 1);
    }
    SECTION("Intermediate entropy: avg. once in 32") {
        functionBenchmarks(seed, 31);
    }
    SECTION("Low entropy: avg. once in 256") {
        functionBenchmarks(seed, 255);
    }
}

