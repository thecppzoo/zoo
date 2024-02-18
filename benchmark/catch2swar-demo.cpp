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
    SECTION("Simple comparison of two strings") {
        auto TwoStrings = "Str1\0Much longer string here, even for AVX2";
        auto zoolength1 = zoo::c_strLength(TwoStrings);
        auto strlen1 = strlen(TwoStrings);
        REQUIRE(zoolength1 == strlen1);
        auto skipFst = TwoStrings + strlen1 + 1;
        auto zl2 = zoo::c_strLength(skipFst);
        auto strlen2 = strlen(skipFst);
        REQUIRE(zl2 == strlen2);
        auto avx1 = zoo::avx2_strlen(TwoStrings);
        REQUIRE(avx1 == strlen1);
        auto avx2 = zoo::avx2_strlen(skipFst);
        REQUIRE(avx2 == strlen2);
    }
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
    REQUIRE(fromGENERIC_GLIBC_STRLEN == fromZOO_NATURAL_STRLEN);
#if ZOO_CONFIGURED_TO_USE_AVX()
    REQUIRE(fromZOO_AVX == fromZOO_STRLEN);
#endif

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

