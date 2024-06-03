#include "c_str-functions/c_str.h"
#include "c_str-functions/corpus.h"

#include <benchmark/benchmark.h>

int g_SideEffect = 0;

template<typename Corpus, typename Callable>
void goOverCorpus(Corpus &c, Callable &&cc) {
    auto iterator = c.commence();
    // copy memory to local variable that will be used at the end:
    // the result of the loop depends on the global state, thus the optimizer
    // can not execute once and store the result away
    auto result = g_SideEffect;
    do {
        result ^= cc(*iterator);
    } while(iterator.next());
    g_SideEffect = result;
    // write to memory a result that required each result in the loop
}

template<typename CorpusMaker, typename Callable>
void runBenchmark(benchmark::State &s) {
    std::random_device rd;
    std::mt19937 g(rd());
    auto corpus = CorpusMaker::makeCorpus(g);
    Callable function;
    for(auto _: s) {
        goOverCorpus(corpus, function);
        benchmark::ClobberMemory();
    }
}

#define X(Typename, _) \
    BENCHMARK(runBenchmark<CorpusLeadingSpaces, Invoke##Typename>);
    LEADING_SPACES_CORPUS_X_LIST
#undef X

#define X(Typename, _) \
    BENCHMARK(runBenchmark<CorpusDecimalDigits<8>, Invoke##Typename>);
    PARSE_8_BYTES_CORPUS_X_LIST
#undef X

#define X(Typename, _) \
    BENCHMARK(runBenchmark<CorpusDecimalDigits<16>, Invoke##Typename>);
    PARSE_16_BYTES_CORPUS_X_LIST
#undef X

#define X(Typename, _) \
    BENCHMARK(runBenchmark<CorpusStringLength, Invoke##Typename>);
    STRLEN_CORPUS_X_LIST
#undef X


#define X(Typename, _) \
    BENCHMARK(runBenchmark<CorpusAtoi, Invoke##Typename>);
    ATOI_CORPUS_X_LIST
#undef X

using RepeatZooStrlen = InvokeZOO_STRLEN;
BENCHMARK(runBenchmark<CorpusStringLength, RepeatZooStrlen>);
