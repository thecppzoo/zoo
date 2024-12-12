#ifndef ZOO_CYCLING_ENGINE_H
#define ZOO_CYCLING_ENGINE_H

#include <iterator>
#include <stdexcept>

namespace zoo {

template <typename Iter>
class CyclingEngine {
public:
    using result_type = typename std::iterator_traits<Iter>::value_type;

    CyclingEngine(Iter first, Iter last):
        valuesBegin(first), valuesEnd(last), current(first)
    {
        if (valuesBegin == valuesEnd) {
            throw std::invalid_argument(
                "CyclingEngine: Range cannot be empty."
            );
        }
    }

    result_type operator()() {
        result_type value = *current;
        ++current;
        if (current == valuesEnd) {
            current = valuesBegin; // Wrap around
        }
        return value;
    }

    void seed(std::size_t newSeed) {
        std::advance(
            current = valuesBegin,
            newSeed % std::distance(valuesBegin, valuesEnd)
        );
    }

    static constexpr result_type min() {
        return std::numeric_limits<result_type>::lowest();
    }

    static constexpr result_type max() {
        return std::numeric_limits<result_type>::max();
    }

private:
    Iter
        valuesBegin,
        valuesEnd,
        current;
};

}

#endif
