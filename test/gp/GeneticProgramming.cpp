#include "zoo/gp/Population_impl_generate.h"
#include "zoo/gp/ArtificialAntEnvironment.h"

/*
#include <cmath>
#include <iterator>

#include <stdexcept>
#include <tuple>
#include <type_traits>
*/
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
template<typename Iterator, typename ToNumberFunc>
StatisticsMoment
calculate_statistics(
    Iterator begin, Iterator end, ToNumberFunc to_number
) {
    // Check if the range is valid
    if (begin == end) {
        throw std::invalid_argument("Range cannot be empty.");
    }

    // Prepare variables
    std::vector<double> numbers;
    numbers.reserve(std::distance(begin, end));

    // Transform the range to numeric values
    std::transform(begin, end, std::back_inserter(numbers), to_number);

    // Calculate count
    auto count = numbers.size();

    // Calculate average
    auto
        sum = std::accumulate(numbers.begin(), numbers.end(), 0.0),
        average = sum / count,
        variance = std::accumulate(numbers.begin(), numbers.end(), 0.0, [average](double acc, double value) {
        double diff = value - average;
        return acc + diff * diff;
    }) / count;
    auto standard_deviation = std::sqrt(variance);

    // Return the result
    return {count, sum, average, standard_deviation};
}


#include <string>
#include <stack>

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

#include <fstream>

TEST_CASE("Genetic Programming", "[genetic-programming]") {
    std::mt19937_64 gennie(0xEdd1e);
    std::array<std::mt19937_64::result_type, 1000> other;
    for(auto ndx = 0; ndx < other.size(); ++ndx) {
        other[ndx] = gennie();
    }
    std::vector<int> a = { 1, 2, 3, 4, 5 };
    WARN(zoo::StreamableView{a});
    zoo::CyclingEngine engine(other.begin(), other.end());
    zoo::Population<ArtificialAnt> p(7, gennie);
    auto moments =
        calculate_statistics(
            p.individualWeights_.begin(),
            p.individualWeights_.end(),
            [](auto i) { return i; }
        );
    WARN(moments.count << ' ' << moments.sum << ' ' << moments.average << ' ' << moments.standard_deviation);
    //auto wholePopulation = new char[moments.sum * 3];
    std::ofstream individuals{"/tmp/genetic-programming-individuals.lst"};
    for(auto i = 0; i < p.individuals_.size(); ++i) {
        individuals <<
            to_string(
                zoo::IndividualStringifier<ArtificialAnt>{p.individuals_[i]}
            ) <<
        '\n';
    }
    individuals.close();
    char
        simpleIndividual[] = { Prog2, IFA, Move, TurnRight },
        conversion[sizeof(simpleIndividual) * 3];
    zoo::conversionToWeightedElement<ArtificialAnt>(
        conversion, simpleIndividual
    );
    zoo::WeightedPreorder<ArtificialAnt> indi(conversion);
    ArtificialAntEnvironment e;
    zoo::evaluate<AAEvaluationFunction>(e, indi, implementationArray);
    REQUIRE(true);
}
