#include "atoi.h"
#include "atoi-corpus.h"

#include <benchmark/benchmark.h>

int g_SideEffect = 0;

template<typename Corpus, typename Callable>
void goOverCorpus(Corpus &c, Callable &&cc) {
    auto iterator = c.commence();
    auto result = g_SideEffect;
    do {
        result ^= cc(*iterator);
    } while(iterator.next());
    g_SideEffect = result;
}

template<typename CorpusMaker, typename Callable>
void runBenchmark(benchmark::State &s) {
    std::random_device rd;
    std::mt19937 g(rd());
    auto corpus = CorpusMaker::makeCorpus(g);
    Callable function;
    for(auto _: s) {
        goOverCorpus(corpus, function);
    }
}

#define X(Typename, FunctionToCall) \
    BENCHMARK(runBenchmark<Corpus8DecimalDigits, Invoke##Typename>);

PARSE8BYTES_CORPUS_X_LIST
#undef X

#define X(TN, FTC) \
    BENCHMARK(runBenchmark<CorpusStringLength, Invoke##TN>);
STRLEN_CORPUS_X_LIST
#undef X
