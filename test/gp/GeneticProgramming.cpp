#include "zoo/gp/ArtificialAntLanguage.h"

#include "zoo/CyclingEngine.h"
#include "zoo/remedies-17/iota.h"

#include <string>
#include <stack>

namespace zoo {
using Language = ArtificialAnt;

#define CSIA constexpr static inline auto

/// \brief by ChatGPT 4o
template<std::size_t N, typename RNG>
auto shuffledIndices(RNG &g) {
    using Base = std::array<int, N>;

    auto indices = zoo::Iota<Base, N>::value;

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
    ) {
        char generationBuffer[1000];
        using D = decltype(Distributions::terminals);
        Distributions dists{
            D{0, Terminals - 1},
            D{Terminals, Terminals + NonTerminals - 1},
            D{0, LanguageSize - 1}
        };
        for(auto ndx = Size; ndx--; ) {
            auto gr = generate(
                dists,
                maxHeight,
                generationBuffer,
                generationBuffer + sizeof(generationBuffer),
                randomGenerator
            );
            individuals_[ndx] = allocate(generationBuffer, gr.weight);
        }
    }
};

}

#include <catch2/catch.hpp>

static_assert(3 == TerminalsCount<ArtificialAnt>());
static_assert(3 == NonTerminalsCount<ArtificialAnt>());

#include "zoo/gp/Individual.h"

#include "zoo/StreamableView.h"

static_assert(zoo::Streamable_v<int>);
static_assert(!zoo::Streamable_v<std::vector<int>>);
static_assert(zoo::Range_v<std::vector<int>>);
static_assert(zoo::RangeWithStreamableElements_v<std::vector<int>>);

struct NotStreamable {};
// Test the testing artifact itself
static_assert(!zoo::Streamable_v<NotStreamable>);
static_assert(!zoo::RangeWithStreamableElements_v<std::vector<NotStreamable>>);

#include "zoo/CyclingEngine.h"

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
}
