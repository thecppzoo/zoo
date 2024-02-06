#include "atoi.h"
#include "atoi-corpus.h"

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "catch2/catch.hpp"

TEST_CASE("Atoi benchmarks", "[atoi][swar]") {
    auto traverse =
        [](auto &&corpus, auto &&function, auto rv) {
            auto iterator = corpus.commence();
            do {
                rv ^= function(*iterator);
            } while(iterator.next());
            return rv;
        };
    std::random_device rd;
    auto seed = rd();
    CAPTURE(seed);
    std::mt19937 g(seed);
    auto corpus = Corpus8DecimalDigits::makeCorpus(g);
    #define X(Type, Fun) \
        auto from##Type = traverse(corpus, Invoke##Type{}, 0);
    PARSE8BYTES_CORPUS_X_LIST
    #undef X
    REQUIRE(fromLemire == fromZoo);
    REQUIRE(fromLIBC == fromZoo);

    auto haveTheRoleOfMemoryBarrier = -1;
    #define X(Type, Fun) \
        BENCHMARK(#Type) { \
            return \
                traverse(corpus, Invoke##Type{}, haveTheRoleOfMemoryBarrier); \
        };
    PARSE8BYTES_CORPUS_X_LIST
    #undef X
}

