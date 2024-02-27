#include "zoo/pp/platform.h"

#include "zoo/swar/associative_iteration.h"

#include "benchmark/benchmark.h"

#include <random>
#include <vector>

uint64_t sideEffect = 0;

template<int NB>
using S = zoo::swar::SWAR<NB, uint64_t>;

template<int NB, bool UseBuiltin>
S<NB> parallelExtraction(S<NB> i, S<NB> m) {
    if constexpr(!UseBuiltin) {
        return compress(i, m);
    } else {
        constexpr auto LaneCount = 64 / NB;
        uint64_t
            input = i.value(),
            mask = i.value(),
            result = 0;
        for(auto lane = 0;;) {
            auto lowV = input & S<NB>::LeastSignificantLaneMask;
            auto lowM = mask & S<NB>::LeastSignificantLaneMask;
            uint64_t tmp;
            if constexpr(NB < 32) {
                tmp = __builtin_ia32_pext_si(lowV, lowM);
            } else {
                tmp = __builtin_ia32_pext_di(lowV, lowM);
            }
            result |= (tmp << (lane * NB));
            ++lane;
            if(LaneCount <= lane) { break; }
            if constexpr(NB != 8*sizeof(typename S<NB>::type)) {
                input >>= NB;
                mask >>= NB;
            }
        }
        return S<NB>{result};
    }
}

template<int NB, bool UseBuiltin>
void runCompressions(benchmark::State &s) {
    using S = zoo::swar::SWAR<NB, uint64_t>;
    std::random_device rd;
    std::mt19937_64 g(rd());
    std::vector<uint64_t> inputs, masks;
    for(auto count = 1000; --count; ) {
        inputs.push_back(g());
        masks.push_back(g());
    }
    for(auto _: s) {
        auto result = 0;
        for(auto c = 1000; c--; ) {
            S input{inputs[c]}, mask{masks[c]};
            result ^= parallelExtraction<NB, UseBuiltin>(input, mask).value();
        }
        sideEffect = result;
        benchmark::ClobberMemory();
    }
}

#define BIT_SIZE_X_LIST X(4) X(8) X(16) X(32) X(64)

#define X(nb) \
    BENCHMARK(runCompressions<nb, false>); \
    BENCHMARK(runCompressions<nb, true>);

BIT_SIZE_X_LIST
