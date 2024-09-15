#include "junk/compressionWithOldParallelSuffix.h"

#include "zoo/pp/platform.h"

#include "zoo/swar/associative_iteration.h"

#include "benchmark/benchmark.h"

#include <random>
#include <vector>

#include <iostream>
#include <bitset>

uint64_t sideEffect = 0;

template<int NB>
using S = zoo::swar::SWAR<NB, uint64_t>;

enum ExtractionPrimitive {
    UseBuiltin,
    UseSWAR,
    UseOldParallelSuffix,
    CompareBuiltinAndSWAR,
    CompareNewAndOldParallelSuffix
};

template<int NB, ExtractionPrimitive P>
S<NB> parallelExtraction(S<NB> i, S<NB> m) {
    if constexpr(UseSWAR == P) {
        return compress(i, m);
    } else if constexpr(UseOldParallelSuffix == P) {
        return zoo::swar::junk::compressWithOldParallelSuffix(i, m);
    } else if constexpr(CompareNewAndOldParallelSuffix == P) {
        auto newOne = compress(i, m);
        auto oldOne = zoo::swar::junk::compressWithOldParallelSuffix(i, m);
        if (newOne.value() != oldOne.value()) {
            using B = std::bitset<64>;
            auto toBinary = [](S<NB> what) { return B(what.value()); };
            std::cout << NB << '\n' <<
                toBinary(i) << '\n' <<
                toBinary(m) << "\n---------\n" <<
                toBinary(newOne) << '\n' <<
                toBinary(oldOne) << '\n' << std::endl;
        }
        return newOne;
    } else {
        constexpr auto LaneCount = 64 / NB;
        uint64_t
            input = i.value(),
            mask = m.value(),
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
        if constexpr(CompareBuiltinAndSWAR == P) {
            auto fromSWAR = compress(i, m);
            using B = std::bitset<64>;
            auto toBinary = [](S<NB> what) { return B(what.value()); };
            if(fromSWAR.value() != result) {
                std::cout << NB << '\n' <<
                    toBinary(i) << '\n' <<
                    toBinary(m) << "\n---------\n" <<
                    toBinary(S<NB>(result)) << '\n' <<
                    toBinary(fromSWAR) << '\n' << std::endl;
            }
        }
        return S<NB>{result};
    }
}

template<int NB, ExtractionPrimitive EP>
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
            result ^= parallelExtraction<NB, EP>(input, mask).value();
        }
        sideEffect = result;
        benchmark::ClobberMemory();
    }
}

#define BIT_SIZE_X_LIST X(4) X(8) X(16) X(32) X(64)

#if ZOO_CONFIGURED_TO_USE_BMI()
    #define EXTENSION_LIST(nb) \
        BENCHMARK(runCompressions<nb, UseBuiltin>); \
        BENCHMARK(runCompressions<nb, CompareBuiltinAndSWAR>);
#else
    #define EXTENSION_LIST(_)
#endif
#define X(nb) \
    BENCHMARK(runCompressions<nb, UseSWAR>); \
    BENCHMARK(runCompressions<nb, UseOldParallelSuffix>); \
    BENCHMARK(runCompressions<nb, CompareNewAndOldParallelSuffix>); \
    EXTENSION_LIST(nb)

BIT_SIZE_X_LIST

