#pragma once

#include <stdint.h>
#include <random>

namespace ep {

template<int N, int K, typename Rng>
inline uint64_t floydSample(Rng &&g) {
    std::uniform_int_distribution<> dist;
    using Bounds = std::uniform_int_distribution<>::param_type;
    uint64_t rv = 0;
    for(auto v = N - K; v < N; ++v) {
        auto draw = dist(g, Bounds(0, v));
        auto bit = uint64_t(1) << draw;
        if(bit & rv) {
            draw = v;
            bit = uint64_t(1) << draw;
        }
        rv |= bit;
    }
    return rv;
}

inline uint64_t deposit(uint64_t preselected, uint64_t extra) {
    auto depositSelector = ~preselected;
    auto deposited = __builtin_ia32_pdep_di(extra, depositSelector);
    return preselected | deposited;
}

template<int N, int K, typename Rng>
inline uint64_t floydSample(Rng &&g, uint64_t preselected) {
    auto normal = floydSample<N, K>(std::forward<Rng>(g));
    return deposit(preselected, normal);
}

}
