#ifndef ZOO_STATISTICS_MOMENTS
#define ZOO_STATISTICS_MOMENTS

#include "zoo/StatisticsMoments.h"
#include <vector>
#include <algorithm>
#include <numeric>

namespace zoo {

struct StatisticsMoments {
    std::size_t count;
    double
        sum,
        average,
        standardDeviation;
    template<typename Iterator, typename NumberProjector>
    StatisticsMoments(
        Iterator begin, Iterator end, NumberProjector &&numberProjector
    );
};

template<typename Iterator, typename NumberProjector>
StatisticsMoments::StatisticsMoments(
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
    this->count = count;
    this->sum = sum;
    this->average = average;
    this->standardDeviation = standardDeviation;
}

}

#endif
