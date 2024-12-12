#include "zoo/CyclingEngine.h"
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

namespace zoo {
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
        Rampled,
        Debug_RampledWithoutDescendantReordering
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

    struct Distributions {
        std::uniform_int_distribution<char>
            terminals,
            nonTerminals,
            all;
    };

    struct GenerationReturnType {
        int realizedHeight;
        int weight;
    };

    static inline auto argsIotaBase_ =
        makeModifiable(
            Iota_v<std::array<unsigned, MaxArgumentCount>, MaxArgumentCount>
        );

    template<typename G>
    static GenerationReturnType generateFrontEnd(
        Distributions &dists,
        int h, char *space, const char *tail, G &g, void *recursionPtr
    ) {
        auto recursion =
            reinterpret_cast<
                GenerationReturnType
                (*)(Distributions &, int, char *, const char *, G &, void *)
            >(recursionPtr);
        auto spc = space;
        auto setToken = [&](char t) {
            *spc = t;
            auto he = std::to_string(h);
            auto to = ArtificialAnt::Tokens[t];
            assert(index < 1000);
            //tokens[index++] = he + ":" + to;
            //tokens[index] = "<->";
        };
        auto onSpaceExhausted =
            [&]() -> GenerationReturnType {
                auto t = dists.terminals(g);
                setToken(t);
                return {0, 1};
            };
        if((tail - spc) < MaxArgumentCount || 0 == h) {
            // not enough space to select a non terminal guaranteed to fit.
            return onSpaceExhausted();
        }
        auto choice = dists.nonTerminals(g);
        setToken(choice);
        ++spc;
        auto currentDescendantMaxHeight = h - 1;
        auto choiceDescendants = Language::ArgumentCount[choice];
        auto tailForDescendants = tail - choiceDescendants;

        std::array<unsigned, MaxArgumentCount> indexOrder = argsIotaBase_;
        if(1 < choiceDescendants) {
            std::shuffle(
                indexOrder.begin(), indexOrder.begin() + choiceDescendants, g
            );
        }
        std::array<int, MaxArgumentCount> heights = {};
        for(auto ndx = choiceDescendants - 1;;) {
            heights[indexOrder[ndx]] = currentDescendantMaxHeight;
            if(0 == ndx) { break; }
            currentDescendantMaxHeight =
                currentDescendantMaxHeight * 2 / 3;
        }
        for(auto ndx = choiceDescendants;;) {
            // this would be a great place for a recursive lambda to
            // improve performance by not capturing all the invariant context.
            // Check that indeed the compilers generate a dumb recursion
            auto recurringResult =
                recursion(
                    dists, heights[ndx], spc, tailForDescendants, g,
                    recursionPtr
                );
            spc += recurringResult.weight;
            if(!--ndx) { break; }
        }
        return { h, int(spc - space) };
    }

    template<typename G>
    auto generate(
        Distributions &dists, int h, char *space, const char *tail, G &g
    ) {
        return
            generateFrontEnd(
                dists, h, space, tail, g, (void *)&generateFrontEnd<G>
            );
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
    )
    #if 0
        terminals(0, Terminals - 1),
        nonTerminals(Terminals, Terminals + NonTerminals - 1),
        all(0, size(Language::ArgumentCount) - 1)
        #endif
    {
        char generationBuffer[1000];
        using D = decltype(Distributions::terminals);
        Distributions dists{
            D{0, Terminals - 1},
            D{Terminals, Terminals + NonTerminals - 1},
            D{0, LanguageSize - 1}
        };
        for(auto ndx = Size; ndx--; ) {
            index = 0;
            auto gr = generate(
                dists,
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

static_assert(3 == TerminalsCount<ArtificialAnt>());
static_assert(3 == NonTerminalsCount<ArtificialAnt>());

template<typename Language>
struct Individual {
    const char *inorderRepresentation;

    struct Conversion {
        std::string str;

        const char *convert(const char *source) {
            return converter(this, source, (void *)&converter);
        }

        static const char *converter(
            Conversion *thy,
            const char *source,
            void *ptrToConverter
        ) {
            auto recursion =
                reinterpret_cast<
                    const char *(*)(Conversion *, const char *, void *)
                >(ptrToConverter);
            auto head = *source++;
            assert(unsigned(head) < size(ArtificialAnt::ArgumentCount));
            thy->str += Language::Tokens[head];
            if(head < TerminalsCount<Language>()) {
                return source;
            }
            thy->str += "(";
            source = recursion(thy, source, ptrToConverter);
            for(
                auto remainingCount = Language::ArgumentCount[head] - 1;
                remainingCount--;
            ) {
                thy->str += ", ";
                source = recursion(thy, source, ptrToConverter);
            }
            thy->str += ")";
            return source;
        }
    };
};

template<typename Language>
auto to_string(const Individual<Language> &i) {
    typename Individual<Language>::Conversion c;
    c.convert(i.inorderRepresentation);
    return c.str;
};

void trace(const char *ptr) {
    WARN(to_string(Individual<ArtificialAnt>{ptr}));
}

#include "zoo/StreamableView.h"

static_assert(zoo::Streamable_v<int>);
static_assert(!zoo::Streamable_v<std::vector<int>>);
static_assert(zoo::Range_v<std::vector<int>>);
static_assert(zoo::RangeWithStreamableElements_v<std::vector<int>>);

struct NotStreamable {};
// Test the testing artifact itself
static_assert(!zoo::Streamable_v<NotStreamable>);
static_assert(!zoo::RangeWithStreamableElements_v<std::vector<NotStreamable>>);

TEST_CASE("Genetic Programming", "[genetic-programming]") {
    std::mt19937_64 gennie(0xEdd1e);
    std::array<std::mt19937_64::result_type, 1000> other;
    for(auto ndx = 0; ndx < other.size(); ++ndx) {
        other[ndx] = gennie();
    }
    std::vector<int> a = { 1, 2, 3, 4, 5 };
    WARN(zoo::StreamableView{a});
    zoo::CyclingEngine engine(other.begin(), other.end());
    zoo::Population p(7, engine);
    trace(p.individuals_[0]);
}
