#include "zoo/gp/Population.h"
#include "zoo/gp/PopulationGenerator_impl_generate.h"
#include "zoo/gp/ArtificialAntLanguage.h"
#include "zoo/gp/ArtificialAntEnvironment.h"
#include "zoo/gp/Individual.h"
#include "zoo/AliasSampling.h"

namespace zoo {

void add(Strip &s, const char *root, unsigned weight) {
    auto &payload = s.payload;
    auto initialLength = payload.size();
    payload.resize(initialLength + 3*weight);
    zoo::conversionToWeightedElement<ArtificialAnt>(payload.data() + initialLength, root);
}

}

#include <vector>
#include <algorithm>
#include <numeric>

// Struct to hold the statistics
struct StatisticsMoment {
    std::size_t count;
    double
        sum,
        average,
        standard_deviation;
};

// Stand-alone function template
template<typename Iterator, typename NumberProjector>
StatisticsMoment
calculate_statistics(
    Iterator begin, Iterator end, NumberProjector &&numberProjector
) {
    // Prepare variables
    std::vector<double> numbers;
    numbers.reserve(std::distance(begin, end));

    // Transform the range to numeric values
    std::transform(begin, end, std::back_inserter(numbers), numberProjector);

    // Calculate count
    auto count = numbers.size();

    // Calculate average
    auto
        sum = std::accumulate(numbers.begin(), numbers.end(), 0.0),
        average = sum / count,
        sumOfSquaredDeviations =
            std::accumulate(
                numbers.begin(), numbers.end(), 0.0,
                [average](double acc, double value) {
                    double diff = value - average;
                    return acc + diff * diff;
                }
            );
    auto standardDeviation = std::sqrt(sumOfSquaredDeviations / count);

    // Return the result
    return {count, sum, average, standardDeviation};
}


#include <string>
#include <stack>

#include <catch2/catch.hpp>

static_assert(3 == zoo::TerminalsCount<ArtificialAnt>());
static_assert(3 == zoo::NonTerminalsCount<ArtificialAnt>());

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

#include <fstream>
#include <iostream>

TEST_CASE("Genetic Programming", "[genetic-programming]") {
    SECTION("Individual") {
        auto verify = [](char *produced, auto &expected) {
            constexpr auto L = std::extent_v<std::remove_reference_t<decltype(expected)>>;
            static_assert(0 == L % 3);
            std::string p, e;
            for(auto ndx = 0; ndx < L; ndx += 3) {
                p += ArtificialAnt::Tokens[produced[ndx]];
                uint16_t u;
                memcpy(&u, produced + ndx + 1, 2);
                p += std::to_string(u);
                e += ArtificialAnt::Tokens[expected[ndx]];
                memcpy(&u, expected + ndx + 1, 2);
                e += std::to_string(u);
            }
            CHECK(p == e);
        };
        constexpr char
            Terminal[] = { Move },
            ExpectedConversionTerminal[] = { Move, 1, 0 },
            NonTerminal[] = { Prog3, TurnRight, Move, TurnRight },
            ExpectedConversionNonTerminal[] = {
                Prog3, 4, 0, TurnRight, 1, 0, Move, 1, 0, TurnRight, 1, 0
            },
            Recursive[] = { Prog2, IFA, Move, TurnLeft },
            ExpectedConversionRecursive[] = {
                Prog2, 4, 0,
                    IFA, 2, 0,
                        Move, 1, 0,
                    TurnLeft, 1, 0
            }
        ;
        char buffer[1000];
        memset(buffer, '*', sizeof(buffer));
        zoo::conversionToWeightedElement<ArtificialAnt>(buffer, Terminal);
        verify(buffer, ExpectedConversionTerminal);
        memset(buffer, '*', sizeof(buffer));
        zoo::conversionToWeightedElement<ArtificialAnt>(buffer, NonTerminal);
        verify(buffer, ExpectedConversionNonTerminal);
        memset(buffer, '*', sizeof(buffer));
        zoo::conversionToWeightedElement<ArtificialAnt>(buffer, Recursive);
        verify(buffer, ExpectedConversionRecursive);
    }
    std::mt19937_64 gennie(0xEdd1e);
    std::array<std::mt19937_64::result_type, 1000> other;
    for(auto ndx = 0; ndx < other.size(); ++ndx) {
        other[ndx] = gennie();
    }
    std::vector<int> a = { 1, 2, 3, 4, 5 };
    WARN(zoo::StreamableView{a});
    zoo::CyclingEngine engine(other.begin(), other.end());
    zoo::PopulationGenerator<ArtificialAnt> p(7, gennie);
    auto moments =
        calculate_statistics(
            p.individualWeights_.begin(),
            p.individualWeights_.end(),
            [](auto i) { return i; }
        );
    WARN(moments.count << ' ' << moments.sum << ' ' << moments.average << ' ' << moments.standard_deviation);
    //auto wholePopulation = new char[moments.sum * 3];
    SECTION("Save individuals") {
        std::ofstream individuals{"/tmp/genetic-programming-individuals.lst"};
        for(auto i = 0; i < p.individuals_.size(); ++i) {
            individuals <<
                to_string(
                    zoo::IndividualStringifier<ArtificialAnt>{p.individuals_[i]}
                ) <<
            '\n';
        }
    }
    SECTION("Strip") {
        zoo::Strip strip;
        auto totalSize = 0;
        for(auto i = p.individuals_.size(); i--; ) {
            totalSize += p.individualWeights_[i];
            add(strip, p.individuals_[i], p.individualWeights_[i]);
            REQUIRE(strip.payload.size() == totalSize*3);
        }
    }
    
    char
        simpleIndividual[] = { Prog2, IFA, Move, TurnRight },
        conversion[sizeof(simpleIndividual) * 3];
    zoo::conversionToWeightedElement<ArtificialAnt>(
        conversion, simpleIndividual
    );
    zoo::WeightedPreorder<ArtificialAnt> indi(conversion);
    ArtificialAntEnvironment e;
    void (*rec)(ArtificialAntEnvironment &, zoo::WeightedPreorder<ArtificialAnt> &, void *) =
        [](ArtificialAntEnvironment &e, zoo::WeightedPreorder<ArtificialAnt> &i, void *r) {
            auto &a = e.ant_;
            std::cout << a.pos.y << ',' << a.pos.x << ' ' << a.dir.y << ',' <<
                a.dir.x << ' ' << ArtificialAnt::Tokens[i.node()] << ' ' <<
                e.steps_ << std::endl;
            recursiveEvaluationFrontEnd(e, i, r);
        };
    while(e.steps_ < 40) {
        rec(e, indi, reinterpret_cast<void *>(rec));
    }
    
    REQUIRE(true);
}
