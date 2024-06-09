#include "zoo/swar/associative_iteration.h"

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "catch2/catch.hpp"

#include <array>
#include <random>

#include <assert.h>

template<int NB, typename G>
auto generateMultiplicationCorpus(G &&g) {
    constexpr auto MultiplierBits = NB/2;
    std::uniform_int_distribution n(0, (1 << MultiplierBits) - 1);
    constexpr auto NLanes = 64/NB;
    std::vector<uint64_t> mms;
    constexpr auto Count = 2043;
    mms.resize(2*Count);
    assert(mms.size() == 2*Count);
    auto b = mms.data();
    auto another = reinterpret_cast<uintptr_t>(b);
    for(auto i = Count; i--; ) {
        auto setLanes = [&]() {
            uint64_t v = 0;
            for(auto l = NLanes; l--; ) {
                v <<= NB;
                v |= n(g);
            }
            auto current = reinterpret_cast<uintptr_t>(b);
            assert((current - another) < (2*Count*8));
            *b++ = v;
        };
        setLanes();
        setLanes();
    }
    return mms;
}

uint64_t g_sideEffect = 0;

template<int NB>
auto SWAR_multiplication(const std::vector<uint64_t> &v) {
    using S = zoo::swar::SWAR<NB>;
    S rv{0};
    for(auto current = v.begin(); v.end() != current; ++current) {
        auto
            multiplicand = S{*current++},
            multiplier = S{*current};
        rv =
            rv ^
            zoo::swar::multiplication_OverflowUnsafe_SpecificBitCount<NB/2>(
                multiplicand, multiplier
            );
    }
    g_sideEffect ^= rv.value();
    return rv;
}

template<int NB>
auto normal_multiplication(const std::vector<uint64_t> &v) {
    uint64_t rv = 0;
    constexpr uint64_t Mask = 1 << (NB - 1);
    for(auto current = v.begin(); v.end() != current; ++current) {
        auto
            multiplicand = *current++,
            multiplier = *current;
        constexpr auto NLanes = 64/NB;
        uint64_t v = 0;
        for(auto i = 0; i < NLanes; ++i) {
            auto  newLane = (Mask & multiplicand) * (Mask & multiplier);
            v |= (newLane << (i*NB));
            multiplicand >>= NB;
            multiplier >>= NB;
        }
        rv = v;
    }
    g_sideEffect ^= rv;
    return rv;
}

template<int NB>
auto compare_multiplications(const std::vector<uint64_t> &v) {
    auto fromNormal = normal_multiplication<NB>(v);
    auto fromSWAR = normal_multiplication<NB>(v);
    if(fromNormal != fromSWAR) { throw 0; }
    return fromNormal;
}

#define SIZES_X_LIST(F) X(4, F)X(8, F)X(16, F)X(32, F)

// This block forces the instantiation of each function outsize of benchmark
// to be inspected with tools like objdump to see what the compiler does
#define X(N, F) auto g_ShowMe##F##N = F<N>;
SIZES_X_LIST(normal_multiplication)
SIZES_X_LIST(SWAR_multiplication)
SIZES_X_LIST(compare_multiplications)
#undef X

TEST_CASE("Swar Multiplication", "[profile][swar][multiplication]") {
    //std::random_device rd;
    std::mt19937 g(148798);
    auto corpus = generateMultiplicationCorpus<8>(g);
    BENCHMARK("normative") {
        return SWAR_multiplication<8>(corpus);
    };
    #define X(N, Fun) \
        BENCHMARK(#Fun #N) { return Fun<N>(corpus); };
    SIZES_X_LIST(compare_multiplications)
    SIZES_X_LIST(SWAR_multiplication)
    SIZES_X_LIST(normal_multiplication)
    #undef X
}
