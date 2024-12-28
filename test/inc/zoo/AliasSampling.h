#ifndef ZOO_ALIAS_SAMPLING_H
#define ZOO_ALIAS_SAMPLING_H

#include <vector>
#include <random>

namespace zoo {

struct AliasSampling {
    std::vector<float> probabilities_;
    std::vector<int> aliases_;
    std::uniform_real_distribution<> uniform_;
    float summation_;

    template<typename It, typename Valuer>
    AliasSampling(It beg, It end, Valuer &&ver) {
        using Value = decltype(ver(*beg));
        std::vector<Value> values;
        auto summation = Value(0);

        // Calculate values and total summation
        for(auto current = beg; current != end; ++current) {
            auto v = ver(*current);
            values.push_back(v);
            summation += v;
        }

        this->summation_ = static_cast<float>(summation);
        size_t n = values.size();
        probabilities_.resize(n);
        aliases_.resize(n);

        // Normalize probabilities
        std::vector<double> normalized;
        for(const auto &v: values) {
            normalized.push_back(static_cast<float>(v) / summation * n);
        }

        std::vector<size_t> small, large;
        for(auto i = n; i--; ) {
            if (normalized[i] < 1.0) {
                small.push_back(i);
            } else {
                large.push_back(i);
            }
        }

        while(!small.empty() && !large.empty()) {
            auto
                s = small.back(),
                l = large.back();
            small.pop_back();
            large.pop_back();

            probabilities_[s] = normalized[s];
            aliases_[s] = l;

            normalized[l] = normalized[l] - (1.0 - normalized[s]);

            if (normalized[l] < 1.0) {
                small.push_back(l);
            } else {
                large.push_back(l);
            }
        }

        for(const auto &idx: large) {
            probabilities_[idx] = 1.0;
        }
        for(const auto &idx: small) {
            probabilities_[idx] = 1.0;
        }
    }

    template<typename Rng>
    auto sample(Rng &rng) const {
        auto rnd = uniform_(rng);
        auto
            n = probabilities_.size(),
            ndx = static_cast<size_t>(rnd *  n / summation_);
        auto local = rnd - ndx;
        return this->probabilities_[ndx] < local ? ndx : aliases_[ndx];
    }
};



}

#endif
