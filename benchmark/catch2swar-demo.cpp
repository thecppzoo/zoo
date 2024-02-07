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
    auto corpus8D = Corpus8DecimalDigits::makeCorpus(g);
    auto corpusStrlen = CorpusStringLength::makeCorpus(g);
    #define X(Type, Fun) \
        auto from##Type = traverse(CORPUS, Invoke##Type{}, 0);

        #define CORPUS corpus8D
            PARSE8BYTES_CORPUS_X_LIST
        #undef CORPUS

        #define CORPUS corpusStrlen
            STRLEN_CORPUS_X_LIST
        #undef CORPUS
    #undef X
    REQUIRE(fromLemire == fromZoo);
    REQUIRE(fromLIBC == fromZoo);
    REQUIRE(fromZOO_STRLEN == fromLIBC_STRLEN);
    REQUIRE(fromLIBC_STRLEN == fromZOO_NATURAL_STRLEN);
    REQUIRE(fromZOO_NATURAL_STRLEN == fromZOO_MANUAL_STRLEN);
    REQUIRE(fromGENERIC_GLIBC_STRLEN == fromZOO_NATURAL_STRLEN);
    REQUIRE(fromZOO_AVX == fromZOO_STRLEN);

    auto haveTheRoleOfMemoryBarrier = -1;
    #define X(Type, Fun) \
        WARN(typeid(CORPUS).name() << ':' << typeid(Fun).name()); \
        BENCHMARK(#Type) { \
            return \
                traverse(CORPUS, Invoke##Type{}, haveTheRoleOfMemoryBarrier); \
        };

        #define CORPUS corpus8D
            PARSE8BYTES_CORPUS_X_LIST
        #undef CORPUS

        #define CORPUS corpusStrlen
            STRLEN_CORPUS_X_LIST
        #undef CORPUS
    #undef X
}

