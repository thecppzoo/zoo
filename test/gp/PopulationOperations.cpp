#include "zoo/gp/Population.h"
#include "zoo/gp/to_string.h"

#include <string.h>
#include <stack>

namespace zoo {

void cross(
    std::string &strip,
    const char *q, int qNdx, const char *p, int pNdx,
    int (*subTreeSize)(const char *) noexcept
) {

    const auto from_q = q + qNdx, from_p = p + pNdx;
    const auto
        size_q = subTreeSize(q),
        size_p = subTreeSize(p),
        qFragmentSize = subTreeSize(from_q),
        pFragmentSize = subTreeSize(from_p);

    auto totalDelta = unsigned(size_q) + size_p;
    auto originalSize = strip.size();
    strip.resize(originalSize + totalDelta);
    auto destination = strip.data() + originalSize;

    auto splice =
        [](
            auto to,
            auto from, auto fromSize, auto splitIndex, auto skipSize,
            auto alternative, auto fragmentIndex, auto fragmentSize
        ) {
            // transfer up to the splitIndex
            memcpy(to, from, splitIndex);
            // insert the fragment
            memcpy(to + splitIndex, alternative + fragmentIndex, fragmentSize);
            // transfer the remainder
            memcpy(
                to + splitIndex + fragmentSize,
                from + splitIndex + skipSize,
                fromSize - splitIndex - skipSize
            );
        };

    // Captures the symmetry between q and p
    // The use of this macro is the lesser evil to reduce error chances
    #define ZOO_SPLICE_DRIVER(d, b) \
        splice(\
            destination,\
            d, size_##d, d##Ndx, d##FragmentSize,\
            b, b##Ndx, b##FragmentSize\
        );

    ZOO_SPLICE_DRIVER(q, p)
    destination += size_q - qFragmentSize + pFragmentSize;
    ZOO_SPLICE_DRIVER(p, q)
    #undef ZOO_SPLICE_DRIVER
}

template<typename Language>
auto treeSize_impl(const char *&tree) noexcept {
    auto rv = 1;
    for(int count =  Language::ArgumentCount[*tree++]; count--; ) {
        rv += treeSize_impl<Language>(tree);
    }
    return rv;
}

template<typename Language>
auto treeSize2(const char *tree) noexcept {
    return treeSize_impl<Language>(tree);
}

template<typename Language>
std::string treeToMarkdown(const char *tree) {
    // Recursive lambda wrapper (Y-combinator)
    auto treeToMarkdownImpl =
        [&](auto &&self, const std::string &prefix, bool isLast) -> std::string
    {
        auto token = Language::Tokens[*tree];
        auto argumentCount = Language::ArgumentCount[*tree];
        ++tree;
        auto currentNode = prefix + (isLast ? "└── " : "├── ") + token + "\n";
        if(0 < argumentCount) {
            auto childPrefix = prefix + (isLast ? "    " : "│   ");
            for(int i = argumentCount; i--; ) {
                currentNode += (*self)(self, childPrefix, 0 == i);
            }
        }
        return currentNode;
    };

    return treeToMarkdownImpl(&treeToMarkdownImpl, "", true);
}

}

#include "zoo/gp/ArtificialAntLanguage.h"

#if !(defined(ZOO_NO_TESTS) && ZOO_NO_TESTS)
#include "catch2/catch.hpp"

auto transformToTokens(const std::string &input) {
    using Language = ArtificialAnt;
    std::string rv;
    for(auto ch: input) { rv += Language::Tokens[ch]; }
    return rv;
}

TEST_CASE("Cross operation correctness", "[cross]") {
    using Language = ArtificialAnt;
    int (*subtreeSize)(const char *) noexcept =
        [](const char *ptr) noexcept {
            return int(zoo::treeSize<Language>(ptr));
        };
    constexpr char
        // Prog3(IFA(Move), Prog2(TL, Move), IFA(Move))
        tree_q[] = { Prog3, IFA, Move, Prog2, TurnLeft, Move, IFA, Move },
        // Prog3(Prog2(Move, TR), IFA(Move), TL)
        tree_p[] = { Prog3, Prog2, Move, TurnRight, IFA, Move, TurnLeft };
    auto qFragmentNdx = 3, pFragmentNdx = 1;
    SECTION("Once") {
        WARN('\n' << zoo::treeToMarkdown<Language>(tree_q));
    }

    SECTION("Tree size") {
        REQUIRE(1 == subtreeSize(tree_q + 2));
        REQUIRE(2 == subtreeSize(tree_q + 1));
        REQUIRE(3 == subtreeSize(tree_q + 3));
        REQUIRE(sizeof(tree_q) == subtreeSize(tree_q));
        REQUIRE(sizeof(tree_p) == subtreeSize(tree_p));
    }

    SECTION("Resulting addendum size is correct") {
        std::string result;

        zoo::cross(
            result, tree_q, qFragmentNdx, tree_p, pFragmentNdx, subtreeSize
        );

        auto
            size_q = subtreeSize(tree_q),
            size_p = subtreeSize(tree_p);

        REQUIRE(
            // Proof:
            // - Each tree contributes its full size minus the removed fragment
            //   size, replaced by the corresponding fragment from the other
            //   tree.
            // - The contributions are:
            //   (size_q - fragment_q + fragment_p) +
            //   (size_p - fragment_p + fragment_q) ==
            //   size_q + size_p

            result.size() == size_q + size_p
        );
    }

    SECTION("Full cross verification example") {
        // Prog3(IFA(Move), Prog2(TL, Move), IFA(Move))
        constexpr char
            tree_q[] = { Prog3, IFA, Move, Prog2, TurnLeft, Move, IFA, Move },
            tree_p[] = { Prog3, Prog2, Move, TurnRight, IFA, Move, TurnLeft };
        auto qFragmentNdx = 3, pFragmentNdx = 1;
        std::string result;
        zoo::cross(
            result, tree_q, qFragmentNdx, tree_p, pFragmentNdx, subtreeSize
        );

        auto validateCrossing = [&](
            const char *d, int dIndex,
            const char *b, int bIndex,
            int resultOffset
        ) {
            auto dWholeSize = subtreeSize(d);
            auto bFragmentSize = subtreeSize(b + bIndex);

            // Extract components of d
            auto dBeforeSplice = std::string(d, dIndex);
            auto dAfterSplice = std::string(
                d + dIndex + bFragmentSize,
                dWholeSize - dIndex - bFragmentSize
            );

            // Extract the fragment from b
            auto bFragment = std::string(b + bIndex, bFragmentSize);

            // Construct the expected result for d with b's fragment
            auto expectedResult = dBeforeSplice + bFragment + dAfterSplice;

            // Validate that the result matches the expected transformation
            auto ttt = transformToTokens;
            REQUIRE(
                ttt(result.substr(resultOffset, expectedResult.size())) ==
                ttt(expectedResult)
            );
        };

        validateCrossing(tree_q, qFragmentNdx, tree_p, pFragmentNdx, 0);

        auto firstCrossedTreeSize = subtreeSize(result.data());

        validateCrossing(
            tree_p, pFragmentNdx, tree_q, qFragmentNdx,
            firstCrossedTreeSize
        );
    }
}
#endif

#include "zoo/gp/PopulationGenerator.h"
#include "zoo/gp/ArtificialAntEnvironment.h"

namespace zoo {

//template<typename PG>
using PG = PopulationGenerator<ArtificialAnt>;
using E = ArtificialAntEnvironment;
std::array<float, PG::Size>
populationEvaluation(PG &pg, void (*notify)(int, E &, void *), void *ctx) {
    using L = ArtificialAnt;
    std::array<float, PG::Size> rv;
    for(auto ndx = PG::Size; ndx--; ) {
        auto genome = pg.individuals_[ndx];
        auto flattened = zoo::flat<ArtificialAnt>(genome, pg.individualWeights_[ndx]);
        WARN(flattened);
        WARN(ndx << ' ' << to_string(zoo::IndividualStringifier<ArtificialAnt>{genome}));
        E env;
        std::string forConversion('*', pg.individualWeights_[ndx] * 3);
        conversionToWeightedElement<L>(
            forConversion.data(), pg.individuals_[ndx]
        );
        WeightedPreorder<L> ind(forConversion.data());
        do {
            artificialAntEvaluation(env, ind);
        } while(!env.atEnd());
        notify(ndx, env, ctx);
        rv[ndx] = env.eaten_;
    }
    return rv;
}

}

#if !(defined(ZOO_NO_TESTS) && ZOO_NO_TESTS)
#include "zoo/StatisticsMoments.h"

#include <chrono>
#include <random>

template<typename Container, typename Range>
Container appendEveryThird(Container &container, const Range &range) {
    auto ndx = 0;
    std::copy_if(
        begin(container),
        end(container),
        std::back_inserter(container),
        [ndx](const auto &) mutable { return ndx++ % 3 == 0; }
    );
}

TEST_CASE("Whole Population", "[genetic-programming][benchmark]") {
    SECTION("Clocks") {
        namespace chrono = std::chrono;
        using namespace std::chrono_literals;
        using Clock = chrono::steady_clock;
        using Period = Clock::period;
        auto pre = Clock::now();
        WARN(Period::num << ' ' << log(Period::den)/log(10));
        auto after = Clock::now();
        auto elapsed = after - pre;
        WARN(elapsed.count() << ' ' << chrono::duration_cast<chrono::nanoseconds>(elapsed).count());
    }
    //auto seed = std::random_device{}();
    auto seed = 329421841;
    WARN("Seed used " << seed);
    std::mt19937_64 gennie(seed);
    zoo::PopulationGenerator<ArtificialAnt> p(6, gennie);

    for(auto ndx = decltype(p)::Size; ndx--; ) {
        auto individualGenes = p.individuals_[ndx];
        auto weight = zoo::treeSize<ArtificialAnt>(individualGenes);
        REQUIRE(weight == p.individualWeights_[ndx]);
        std::string forConversion(weight, '*');
        zoo::conversionToWeightedElement<ArtificialAnt>(
            forConversion.data(), individualGenes
        );
        auto conversionCursor = forConversion.data();
        auto geneCursor = individualGenes;
        for(auto ndx = weight; ndx; --ndx) {
            zoo::WeightedPreorder<ArtificialAnt> n(conversionCursor);
            REQUIRE(*geneCursor == n.node());
            REQUIRE(zoo::treeSize<ArtificialAnt>(geneCursor) == n.weight());
        }
    }
    zoo::StatisticsMoments
        moments {
            p.individualWeights_.begin(),
            p.individualWeights_.end(),
            [](auto i) { return i; }
        };
    WARN(moments.count << ' ' << moments.sum << ' ' << moments.average << ' ' << moments.standardDeviation);
    struct Sampler {
        decltype(std::chrono::steady_clock::now()) sampleStart_;
        std::vector<long> durations_;
        Sampler(): sampleStart_(std::chrono::steady_clock::now()) {}
    };
    Sampler smplr;
    auto sampleCollector = [](int ndx, ArtificialAntEnvironment &e, void *ctx) {
        Sampler *sp = static_cast<Sampler *>(ctx);
        auto d = std::chrono::steady_clock::now() - sp->sampleStart_;
        sp->durations_.push_back(d.count());
        if(ndx % 100 == 99) {
            auto micros = std::chrono::duration_cast<std::chrono::microseconds>(d).count();
            WARN(ndx << ' ' << e.eaten_ << ' ' << e.steps_ << ' ' << micros);
        }
        sp->sampleStart_ = std::chrono::steady_clock::now();
    };
    auto results = populationEvaluation(p, sampleCollector, &smplr);
    zoo::StatisticsMoments times{begin(smplr.durations_), end(smplr.durations_), [](auto i) { return i; }};
    WARN(times.count << ' ' << times.sum << ' ' << times.average << ' ' << times.standardDeviation);
}
#endif
