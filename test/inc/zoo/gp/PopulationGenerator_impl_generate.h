#ifndef ZOO_GP_POPULATION_GENERATOR_IMPL_GENERATE_H
#define ZOO_GP_POPULATION_GENERATOR_IMPL_GENERATE_H

#include "zoo/gp/PopulationGenerator.h"

#include <assert.h>

namespace zoo {

template<typename Language, size_t Size>
char *
PopulationGenerator<Language, Size>::allocate(const char *source, int weight) {
    auto rv = new char[weight];
    assert(rv);
    memcpy(rv, source, weight);
    return rv;
}

template<typename Language, size_t Size>
template<typename G>
typename PopulationGenerator<Language, Size>::GenerationReturnType
PopulationGenerator<Language, Size>::generateBackEnd(
    Distributions &dists,
    int h, char *space, const char *tail, G &g, void *recursionPtr
) {
    auto recursion =
        reinterpret_cast<
            GenerationReturnType // return type
            (*) // indicator this is a function pointer
            // finally, the list of arguments
            (Distributions &, int, char *, const char *, G &, void *)
        >(recursionPtr);
    auto spc = space;
    auto setToken = [&](char t) {
        *spc = t;
    };
    auto onSpaceExhausted = [&]() -> GenerationReturnType {
        auto t = dists.terminals(g);
        setToken(t);
        return {0, 1};
    };
    if((tail - spc) < MaxArgumentCount || 0 == h) {
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
        if (0 == ndx--) { break; }
        currentDescendantMaxHeight =
            currentDescendantMaxHeight * 2 / 3;
    }
    for(auto ndx = choiceDescendants - 1;;) {
        auto recurringResult =
            recursion(
                dists, heights[ndx], spc, tailForDescendants, g, recursionPtr
            );
        spc += recurringResult.weight;
        if(!ndx--) { break; }
    }
    return { h, int(spc - space) };
}

template<typename Language, size_t Size>
template<typename G>
PopulationGenerator<Language, Size>::PopulationGenerator(
    int maxHeight,
    G &randomGenerator,
    typename PopulationGenerator<Language, Size>::GenerationStrategies strategy
) {
    char generationBuffer[1000];
    using D = decltype(Distributions::terminals);
    Distributions dists{
        D{0, Terminals - 1},
        D{Terminals, Terminals + NonTerminals - 1},
        D{0, LanguageSize - 1}
    };
    for(auto ndx = Size; ndx--; ) {
        auto originalRandomState = randomGenerator;
        auto gr = generate(
            dists,
            maxHeight,
            generationBuffer,
            generationBuffer + sizeof(generationBuffer),
            randomGenerator
        );
        individuals_[ndx] = allocate(generationBuffer, gr.weight);
        individualWeights_[ndx] = gr.weight;
        auto [h, w] = height(generationBuffer);
        if(h != maxHeight) {
            auto regen = generate(
                dists,
                maxHeight,
                generationBuffer,
                generationBuffer + sizeof(generationBuffer),
                originalRandomState
            );
            assert(regen.realizedHeight == maxHeight);
        }
    }
}

} // namespace zoo

#endif // ZOO_GP_POPULATION_GENERATOR_IMPL_GENERATE_H

