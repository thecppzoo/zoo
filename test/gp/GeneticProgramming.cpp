#include <array>
#include <random>
#include <assert.h>

struct ArtificialAnt {
    constexpr static inline std::array Tokens {
        "TR", "TL", "Move", "IFA", "Prog"
    };
    constexpr static inline std::array ArgumentCount {
        0, 0, 0, 1, 2
    };
};

void trace(const char *);

namespace zoo {

using std::size;

template<typename T>
constexpr auto TerminalsCount() {
    for(size_t ndx = 0; ndx < size(T::ArgumentCount); ++ndx) {
        if(T::ArgumentCount[ndx]) { return ndx; }
    }
    return size(T::ArgumentCount);
}

template<typename T>
constexpr auto NonTerminalsCount() {
    return size(T::ArgumentCount) - TerminalsCount<T>();
}

using Language = ArtificialAnt;

#define CSIA constexpr static inline auto

std::string tokens[1000];
int index = 0;

struct Population {
    static_assert(size(Language::Tokens) == size(Language::ArgumentCount));
    static_assert(size(Language::Tokens) < 256);
    enum GenerationStrategies {
        Rampled
    };

    CSIA
        LanguageSize = size(Language::ArgumentCount),
        NonTerminals = NonTerminalsCount<Language>(),
        Terminals = LanguageSize - NonTerminals,
        Size = size_t(1000);
    CSIA MaxArgumentCount = []() constexpr {
        auto rv = 2;
        for(auto ndx = size(Language::ArgumentCount); NonTerminals != ndx--; ) {
            if(rv < Language::ArgumentCount[ndx]) {
                rv = Language::ArgumentCount[ndx];
            }
        }
        return rv;
    }();

    std::array<char *, Size> individuals_;

    std::uniform_int_distribution<char>
        terminals,
        nonTerminals,
        all;

    struct GenerationReturnType {
        int realizedHeight;
        int weight;
    };

    template<typename G>
    GenerationReturnType generate(int h, char *space, const char *tail, G &g) {
        auto spc = space;
        auto setToken = [&](char t) {
            *spc = t;
            auto he = std::to_string(h);
            auto to = ArtificialAnt::Tokens[t];
            assert(index < 1000);
            tokens[index++] = he + ":" + to;
            tokens[index] = "<->";
        };
        auto onSpaceExhausted =
            [&]() -> GenerationReturnType {
                auto t = terminals(g);
                setToken(t);
                return {0, 1};
            };
        if((tail - spc) < MaxArgumentCount || 0 == h) {
            // not enough space to select a non terminal guaranteed to fit.
            return onSpaceExhausted();
        }
        auto choice = nonTerminals(g);
        setToken(choice);
        ++spc;
        auto currentDescendantMaxHeight = h - 1;
        auto choiceDescendants = Language::ArgumentCount[choice];
        auto tailForDescendants = tail - choiceDescendants;
        for(auto ndx = choiceDescendants;;) {
            // this would be a great place for a recursive lambda to
            // improve performance by not capturing all the invariant context.
            // Check that indeed the compilers generate a dumb recursion
            auto recurringResult =
                generate(
                    currentDescendantMaxHeight, spc, tailForDescendants, g
                );
            spc += recurringResult.weight;
            if(!--ndx) { break; }
            currentDescendantMaxHeight =
                currentDescendantMaxHeight * 2 / 3;
        }
        return { h, int(spc - space) };
    }

    char *allocate(const char *source, int weight) {
        auto rv = new char[weight];
        assert(rv);
        memcpy(rv, source, weight);
        return rv;
    }

    template<typename G>
    Population(
        int maxHeight,
        G &randomGenerator,
        GenerationStrategies strategy = Rampled
    ):
        terminals(0, Terminals - 1),
        nonTerminals(Terminals, Terminals + NonTerminals - 1),
        all(0, size(Language::ArgumentCount) - 1)
    {
        char generationBuffer[1000];
        for(auto ndx = Size; ndx--; ) {
            index = 0;
            auto gr = generate(
                maxHeight,
                generationBuffer,
                generationBuffer + sizeof(generationBuffer),
                randomGenerator
            );
            trace(generationBuffer);
            individuals_[ndx] = allocate(generationBuffer, gr.weight);
        }
    }
};

}

#include <catch2/catch.hpp>

#include <sstream>

static_assert(3 == zoo::TerminalsCount<ArtificialAnt>());
static_assert(2 == zoo::NonTerminalsCount<ArtificialAnt>());

template<typename Language>
struct Individual {
    const char *inorderRepresentation;

    struct Conversion {
        std::string str;
        const char *converter(const char *source) {
            auto head = *source++;
            assert(unsigned(head) < size(ArtificialAnt::ArgumentCount));
            str += Language::Tokens[head];
            if(head < zoo::TerminalsCount<Language>()) {
                return source;
            }
            str += "(";
            source = converter(source);
            for(
                auto remainingCount = Language::ArgumentCount[head] - 1;
                remainingCount--;
            ) {
                str += ", ";
                source = converter(source);
            }
            str += ")";
            return source;
        }
    };
};

template<typename Language>
auto to_string(const Individual<Language> &i) {
    typename Individual<Language>::Conversion c;
    c.converter(i.inorderRepresentation);
    return c.str;
};

void trace(const char *ptr) {
    WARN(to_string(Individual<ArtificialAnt>{ptr}));
}

TEST_CASE("Genetic Programming", "[genetic-programming]") {
    std::mt19937_64 gennie(0xEdd1e);
    zoo::Population p(7, gennie);
    trace(p.individuals_[0]);
}


