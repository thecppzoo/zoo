#include "c_str-functions/c_str.h"
#include "c_str-functions/corpus.h"

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "catch2/catch.hpp"

#include <limits.h>

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
        #if ZOO_CONFIGURED_TO_USE_AVX()
            auto avx1 = zoo::avx2_strlen(TwoStrings);
            REQUIRE(avx1 == strlen1);
            auto avx2 = zoo::avx2_strlen(skipFst);
            REQUIRE(avx2 == strlen2);
        #endif
    }
    auto corpus8D = Corpus8DecimalDigits::makeCorpus(g);
    auto corpusLeadingSpaces = CorpusLeadingSpaces::makeCorpus(g);
    SECTION("Leading Spaces Comparison") {
        auto iterator = corpusLeadingSpaces.commence();
        do {
            auto glibc = spaces_glibc(*iterator);
            auto zspc = zoo::leadingSpacesCount(*iterator);
            if(glibc != zspc) {
                auto v = zoo::leadingSpacesCount(*iterator);
                WARN("<<" << *iterator << ">> " << v );
            }
            REQUIRE(glibc == zspc);
        } while(iterator.next());
    }
    auto corpusStrlen = CorpusStringLength::makeCorpus(g);
    auto corpusAtoi = CorpusAtoi::makeCorpus(g);
    #define X(Type, Fun) \
        auto from##Type = traverse(CORPUS, Invoke##Type{}, 0);

        #define CORPUS corpus8D
            PARSE8BYTES_CORPUS_X_LIST
        #undef CORPUS

        #define CORPUS corpusLeadingSpaces
            LEADING_SPACES_CORPUS_X_LIST
        #undef CORPUS

        #define CORPUS corpusStrlen
            STRLEN_CORPUS_X_LIST
        #undef CORPUS

        #define CORPUS corpusAtoi
            ATOI_CORPUS_X_LIST
        #undef CORPUS
    #undef X
    REQUIRE(fromGLIB_Spaces == fromZooSpaces);
    REQUIRE(fromLemire == fromZoo);
    REQUIRE(fromLIBC == fromZoo);
    REQUIRE(fromZOO_STRLEN == fromLIBC_STRLEN);
    REQUIRE(fromLIBC_STRLEN == fromZOO_NATURAL_STRLEN);
    REQUIRE(fromGENERIC_GLIBC_STRLEN == fromZOO_NATURAL_STRLEN);
    #if ZOO_CONFIGURED_TO_USE_AVX()
        REQUIRE(fromZOO_AVX == fromZOO_STRLEN);
    #endif
    
    REQUIRE(fromZooSpaces == fromGLIB_Spaces);

    REQUIRE(fromGLIBC_atoi == fromZOO_ATOI);

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

        #define CORPUS corpusLeadingSpaces
            LEADING_SPACES_CORPUS_X_LIST
        #undef CORPUS
    #undef X
}

TEST_CASE("Atoi correctness", "[pure-test][swar][atoi]") {
    auto empty = "";
    REQUIRE(0 == zoo::c_strToI(empty));
    alignas(8) constexpr char EmptyMisaligned[8] = { 'Q', '\0', '0', '1', '2', '3', '9', '\0' };
    static_assert(8 == alignof(EmptyMisaligned));
    REQUIRE(0 == zoo::c_strToI(EmptyMisaligned + 1));
    auto justSpaces = "    \t\t\v  ";
    REQUIRE(0 == zoo::c_strToI(justSpaces));
    REQUIRE(1239 == zoo::c_strToI(EmptyMisaligned + 2));
    auto hasPositiveSign = "\t\r\t\v+123456";
    REQUIRE(123456 == zoo::c_strToI(hasPositiveSign));
    auto hasNegativeSign9nines = "\t\r\t\v-999999999";
    REQUIRE(-999'999'999 == zoo::c_strToI(hasNegativeSign9nines));
    auto excessiveZeroesCloseToIntMax = "+00000000000001987654321";
    REQUIRE(1'987'654'321 == zoo::c_strToI(excessiveZeroesCloseToIntMax));
    char buffer[20];
    sprintf(buffer, "%d", INT_MAX);
    REQUIRE(INT_MAX == zoo::c_strToI(buffer));
    sprintf(buffer, "%d", INT_MIN);
    REQUIRE(INT_MIN == zoo::c_strToI(buffer));
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution rnd(INT_MIN, INT_MAX);
    auto randomNumber = rnd(g);
    sprintf(buffer, "    %d", randomNumber);
    auto glibc = atoi(buffer);
    REQUIRE(zoo::c_strToI(buffer) == glibc);
}
