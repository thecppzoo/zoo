#ifndef ZOO_GP_POPULATION_H
#define ZOO_GP_POPULATION_H

#include "zoo/gp/ArtificialAntLanguage.h"
#include "zoo/remedies-17/iota.h"

#include <algorithm>

namespace zoo {

template<typename Language, size_t Size = 1000>
struct Population {
    static_assert(size(Language::Tokens) == size(Language::ArgumentCount));
    static_assert(size(Language::Tokens) < 256);

    enum GenerationStrategies {
        Rampled,
        Debug_RampledWithoutDescendantReordering
    };

    constexpr static inline auto
        LanguageSize = size(Language::ArgumentCount),
        NonTerminals = NonTerminalsCount<Language>(),
        Terminals = LanguageSize - NonTerminals;

    constexpr static inline auto MaxArgumentCount = []() constexpr {
        auto rv = 2;
        for(auto ndx = size(Language::ArgumentCount); NonTerminals != ndx--; ) {
            if (rv < Language::ArgumentCount[ndx]) {
                rv = Language::ArgumentCount[ndx];
            }
        }
        return rv;
    }();

    std::array<char *, Size> individuals_;
    std::array<int, Size> individualWeights_;

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

    /// @brief
    ///     Generates a random program tree for the genetic programming
    ///     population.
    ///
    /// Constructs a tree of tokens representing a program for the genetic
    /// programming system. Recursion is implemented using an idiom where
    /// the function is passed to itself as an argument, enabling control of
    /// the recursion, as is useful in unit tests, for example.
    ///
    /// @tparam G
    ///     The type of the random number generator (e.g., `std::mt19937`).
    ///
    /// @param dists
    ///     A set of distributions for selecting terminals and non-terminals.
    ///
    /// @param h The current height limit for the tree being generated.
    ///
    /// @param space
    ///     Pointer to the memory where the tree structure will be stored.
    ///
    /// @param tail
    ///     Pointer to the end of the allocated memory region for bounds
    ///     checking.
    ///
    /// @param g The random number generator used for stochastic decisions.
    ///
    /// @param recursionPtr
    ///     Pointer to the recursive function, enabling dynamic recursion.
    ///
    /// @return
    ///     A structure containing the realized height of the tree and its total
    ///     weight (number of nodes).
    ///
    /// @note
    ///     This function uses a variation of the Y-combinator idiom, enabling
    ///     recursion without direct self-references. This improves modularity
    ///     and supports recursive logic in generic programming contexts.
    template<typename G>
    static GenerationReturnType generateBackEnd(
        Distributions &dists,
        int h, char *space, const char *tail, G &g, void *recursionPtr
    );

    /// Generation front end, \sa generationBackEnd
    template<typename G>
    static auto generate(
        Distributions &dists, int h, char *space, const char *tail, G &g
    ) {
        return
            generateBackEnd(
                dists, h, space, tail, g,
                reinterpret_cast<void *>(generateBackEnd<G>)
            );
    }

    char *allocate(const char *source, int weight);

    template<typename G>
    Population(
        int maxHeight,
        G &randomGenerator,
        GenerationStrategies strategy = Rampled
    );

    static GenerationReturnType height(const char *root) {
        auto p = root;
        auto tag = *p++;
        if(tag < Terminals) { return {0, 1}; }
        auto max = 0;
        for(auto i = Language::ArgumentCount[tag]; i--; ) {
            auto [h, w] = height(p);
            if(max < h) { max = h; }
            p += w;
        }
        return {max + 1, int(p - root)};
    }
};

}

#endif // ZOO_GP_POPULATION_H

