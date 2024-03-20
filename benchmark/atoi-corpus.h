#include "atoi.h"
#include "zoo/pp/platform.h"

#include <vector>
#include <string>
#include <cstring>
#include <array>
#include <random>
#include <math.h>

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
            sizes.push_back(length + 1);
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

#if ZOO_CONFIGURED_TO_USE_NEON()
#define NEON_STRLEN_CORPUS_X_LIST \
    X(ZOO_NEON, zoo::neon_strlen)
#else
#define NEON_STRLEN_CORPUS_X_LIST /* nothing */
#endif


#define STRLEN_CORPUS_X_LIST \
    X(LIBC_STRLEN, strlen) \
    X(ZOO_STRLEN, zoo::c_strLength) \
    X(ZOO_NATURAL_STRLEN, zoo::c_strLength_natural) \
    X(GENERIC_GLIBC_STRLEN, STRLEN_old) \
    AVX2_STRLEN_CORPUS_X_LIST \
    NEON_STRLEN_CORPUS_X_LIST

struct CorpusLeadingSpaces {
    constexpr static auto CountOfSpaceCharactersAvailable = 6;
    constexpr static inline std::array<char, CountOfSpaceCharactersAvailable> Spaces =
        { ' ', '\n', '\t', '\r', '\f', '\v' };
    std::vector<int> skips_;
    std::string characters_;

    CorpusLeadingSpaces(std::vector<int> &&skips, std::string &&cs):
        skips_{std::move(skips)}, characters_{std::move(cs)}
    {}

    template<typename G>
    static auto makeCorpus(G &generator) {
        auto count = 1031; // see Corpus8DecimalDigits for why 1031
        std::vector<int> sizes;
        std::string allCharacters;
        std::geometric_distribution<>
            spacesCount(1.0/29),
            extraCharacters(0.5);
            // unrepresentatively very large, but will cross the 32 boundary
            // to test 32-byte techniques
        std::uniform_int_distribution<>
            spacer(0, CountOfSpaceCharactersAvailable - 1),
            moreCharacters(0, 255);

        while(count--) {
            auto count = spacesCount(generator);
            for(auto i = count; i--; ) {
                allCharacters.append(1, Spaces[spacer(generator)]);
            }
            auto extra = moreCharacters(generator);
            for(auto i = extra; i--; ) {
                allCharacters.append(1, moreCharacters(generator));
            }
            sizes.push_back(count + extra + 1);
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

#define LEADING_SPACES_CORPUS_X_LIST X(GLIB_Spaces, spaces_glibc) X(ZooSpaces, zoo::leadingSpacesCount)

void (*consumeStrPtr)(const char *, unsigned) =
    [](const char *p, unsigned l) {
        return;
    };

struct CorpusAtoi {
    constexpr static auto CountOfSpaceCharactersAvailable = 6;
    constexpr static inline std::array<char, CountOfSpaceCharactersAvailable> Spaces =
        { ' ', '\n', '\t', '\r', '\f', '\v' };
    std::vector<int> skips_;
    std::string characters_;

    CorpusAtoi(std::vector<int> &&skips, std::string &&cs):
        skips_{std::move(skips)}, characters_{std::move(cs)}
    {}

    template<typename G>
    static auto makeCorpus(G &generator) {
        auto count = 1031; // see Corpus8DecimalDigits for why 1031
        std::vector<int> sizes;
        std::string allCharacters;
        std::geometric_distribution
            spacesCount(0.5),
            insignificantZeros(0.9);
        std::uniform_real_distribution numberLogarithmBase10(-2.0, 9.2);
            // a maximum of 10^9.2 is ~1.6 billion, within the range.
            // negative "logarithms" are for indicating negative numbers up to
            // -10^2, or -100
        std::uniform_int_distribution
            postNumber('9' + 1, 255),
            spacer(0, CountOfSpaceCharactersAvailable - 1);
        char conversionBuffer[20];

        while(count--) {
            auto currentLength = allCharacters.size();
            auto count = spacesCount(generator);
            for(auto i = count; i--; ) {
                allCharacters.append(1, Spaces[spacer(generator)]);
            }
            auto logBase10 = numberLogarithmBase10(generator);
            int negativeSign;
            if(0.0 <= logBase10) {
                negativeSign = 0;
            } else {
                allCharacters.append(1, '-');
                logBase10 = -logBase10;
                negativeSign = 1;
            }
            auto iz = insignificantZeros(generator);
            for(auto i = iz; i--; ) {
                allCharacters.append(1, '0');
            }
            int number = exp(logBase10 * M_LN10);
            auto n = sprintf(conversionBuffer, "%d%c", number, postNumber(generator));
            if(n < 0) { throw 0; }
            allCharacters.append(conversionBuffer);
            sizes.push_back(count + negativeSign + iz + n);
            consumeStrPtr(allCharacters.c_str() + currentLength, count + negativeSign + iz + n);
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

#define ATOI_CORPUS_X_LIST \
    X(GLIBC_atoi, atoi) X(ZOO_ATOI, zoo::c_strToI) X(COMPARE_ATOI, zoo::compareAtoi)

#define X(Typename, FunctionToCall) \
    struct Invoke##Typename { int operator()(const char *p) { return FunctionToCall(p); } };

PARSE8BYTES_CORPUS_X_LIST
STRLEN_CORPUS_X_LIST
LEADING_SPACES_CORPUS_X_LIST
ATOI_CORPUS_X_LIST

#undef X
