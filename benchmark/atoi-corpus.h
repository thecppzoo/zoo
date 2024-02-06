#include <vector>
#include <string>
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

#define X(Typename, FunctionToCall) \
    struct Invoke##Typename { int operator()(const char *p) { return FunctionToCall(p); } };

PARSE8BYTES_CORPUS_X_LIST
#undef X
