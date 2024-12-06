#include "zoo/remedies-17/iota.h"

#include <array>
#include <random>
#include <stack>

#include <assert.h>

struct ArtificialAnt {
    constexpr static inline std::array Tokens {
        "TR", "TL", "Move", "IFA", "Prog", "P3"
    };
    constexpr static inline std::array ArgumentCount {
        0, 0, 0, 1, 2, 3
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

/// \brief by ChatGPT 4o
template<std::size_t N, typename RNG>
auto shuffledIndices(RNG &g) {
    std::array<int, N> indices;
    std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., N-1

    std::shuffle(indices.begin(), indices.end(), g); // Shuffle the indices
    return indices;
}

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

        constexpr static auto iota =
            Iota<std::array<unsigned, MaxArgumentCount>, MaxArgumentCount>::value;
        if(1 == choiceDescendants)
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

    struct ConversionFrame {
        unsigned weight, remainingSiblings;
        char *destination;
    };
    auto conversionToWeightedElement(const char *input, char *output) {
        using L = Language;
        std::stack<ConversionFrame> frames;

        auto writeWeight = [&](unsigned w) {
            // assumes little endian
            // memcpy would allow unaligned copying of the bytes
            // on the platforms that support it,
            // a "reinterpret_cast" might not respect necessary
            // alignment in some platforms
            memcpy(frames.top().destination, &w, sizeof(uint16_t));
            output += 2;
        };

        frames.push({0, 1, output});
        assert(0 < L::ArgumentCount[*input]);

        for(;;) {
            auto &top = frames.top();
            if(0 == top.remainingSiblings--) {
                // No more siblings, thus completed the frame
                writeWeight(top.weight);
                frames.pop();
                if(frames.empty()) { break; }
                continue;
            }
            frames.top().weight += 1;
            auto current = *input++;
            auto destination = frames.top().destination;

            *frames.top().destination++ = current;
            auto argumentCount = L::ArgumentCount[current];
            if(0 < argumentCount) {
                // There are argumentCount descendants to convert,
                // create a new frame for them
                frames.push({0, unsigned(argumentCount), destination + 2});
            }
        }
    }
};

}

#include <catch2/catch.hpp>

#include <sstream>

static_assert(3 == zoo::TerminalsCount<ArtificialAnt>());
static_assert(3 == zoo::NonTerminalsCount<ArtificialAnt>());

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


