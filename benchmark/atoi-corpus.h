#include "atoi.h"
#include "zoo/pp/platform.h"

#include "zoo/pp/platform.h"

#include <vector>
#include <string>
#include <cstring>
#include <random>

struct Corpus8DecimalDigits {
    std::vector<int> asNumbers_;
    std::string characters_;

    Corpus8DecimalDigits(std::vector<int> aNs, std::string cs):
        asNumbers_(aNs),
        characters_(cs)
    {}

    template<typename G>
    static auto makeCorpus(G &generator) {
        auto count = 1031; // 1031 is a prime number, this helps to disable in
        // practice the branch predictor, the idea is to measure the performance
        // of the code under measurement, not how the the unrealistic conditions
        // of microbenchmarking help/hurt the code under measurement
        std::string allCharacters;
        allCharacters.resize(count * 9);
        std::vector<int> inputs;
        std::uniform_int_distribution<> range(0, 100*1000*1000 - 1);
        char *base = allCharacters.data();
        for(;;) {
            auto input = range(generator);
            snprintf(base, 9, "%08d", input);
            inputs.push_back(input);
            if(--count) { break; }
            base += 9;
        }
        return Corpus8DecimalDigits(inputs, allCharacters);
    }

    struct Iterator {
        Corpus8DecimalDigits *thy;
        int *ip;
        char *cp;

        Iterator &operator++() {
            ++ip;
            cp += 9;
            return *this;
        }

        char *operator*() {
            return cp;
        }

        auto next() noexcept {
            ++(*this);
            return cp != thy->characters_.data() + thy->characters_.size();
        }
    };

    Iterator commence() {
        return { this, asNumbers_.data(), characters_.data() };
    }
};

#define PARSE8BYTES_CORPUS_X_LIST \
    X(Lemire, parse_eight_digits_swar)\
    X(Zoo, lemire_as_zoo_swar)\
    X(LIBC, atoi)

struct CorpusStringLength {
    std::vector<int> skips_;
    std::string characters_;

    CorpusStringLength(std::vector<int> &&skips, std::string &&cs):
        skips_{std::move(skips)}, characters_{std::move(cs)}
    {}

    template<typename G>
    static auto makeCorpus(G &generator) {
        auto count = 1031; // see Corpus8DecimalDigits for why 1031
        std::vector<int> sizes;
        std::string allCharacters;
        std::uniform_int_distribution<> strSize(0, 101); // again a prime
        std::uniform_int_distribution<> characters(1, 255); // notice 0 excluded

        while(count--) {
            auto length = strSize(generator);
            sizes.push_back(length);
            for(auto i = length; i--; ) {
                allCharacters.append(1, characters(generator));
            }
            allCharacters.append(1, '\0');
        }
        return CorpusStringLength(std::move(sizes), std::move(allCharacters));
    }

    struct Iterator {
        int *skips, *sentinel;
        char *cp;

        Iterator &operator++() {
            cp += *skips++;
            return *this;
        }

        char *operator*() {
            return cp;
        }

        auto next() noexcept {
            ++(*this);
            return sentinel != skips;
        }
    };

    Iterator commence() {
        return {
            skips_.data(), skips_.data() + skips_.size(), characters_.data()
        };
    }
};


#if ZOO_CONFIGURED_TO_USE_AVX()
#define AVX2_STRLEN_CORPUS_X_LIST \
    X(ZOO_AVX, zoo::avx2_strlen)
#else
#define AVX2_STRLEN_CORPUS_X_LIST /* nothing */
#endif


#define STRLEN_CORPUS_X_LIST \
    X(LIBC_STRLEN, strlen) \
    X(ZOO_STRLEN, zoo::c_strLength) \
    X(ZOO_NATURAL_STRLEN, zoo::c_strLength_natural) \
    X(ZOO_MANUAL_STRLEN, zoo::c_strLength_manualComparison) \
    X(GENERIC_GLIBC_STRLEN, STRLEN_old) \
    AVX2_STRLEN_CORPUS_X_LIST


#define X(Typename, FunctionToCall) \
    struct Invoke##Typename { int operator()(const char *p) { return FunctionToCall(p); } };

PARSE8BYTES_CORPUS_X_LIST
STRLEN_CORPUS_X_LIST

#undef X
