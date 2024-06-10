#include "zoo/swar/associative_iteration.h"

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "catch2/catch.hpp"

#include <array>
#include <random>

#include <assert.h>

template<int NB, typename G>
auto generateMultiplicationCorpus(G &&g) {
    std::vector<uint64_t> mms;
    constexpr auto Count = 2043;
    mms.resize(2*Count);
    auto b = mms.data();
    for(auto i = Count; i--; ) {
        *b++ = g();
        *b++ = g();
    }
    return mms;
}

uint64_t g_sideEffect = 0;

template<
    int NB,
    zoo::swar::SWAR<NB> (*Fun)(zoo::swar::SWAR<NB>, zoo::swar::SWAR<NB>)
>
auto multiplicationTraverse(const std::vector<uint64_t> &v) {
    using S = zoo::swar::SWAR<NB>;
    S rv{0};
    constexpr auto
        HalfLaneMask = S::LeastSignificantLaneMask >> (NB / 2),
        Mask = zoo::meta::BitmaskMaker<uint64_t, HalfLaneMask, NB>::value;
    
    for(auto current = v.begin(); v.end() != current; ++current) {
        auto
            multiplicand = S{Mask & *current++},
            multiplier = S{Mask & *current};
        rv = rv ^ Fun(multiplicand, multiplier);
    }
    g_sideEffect ^= rv.value();
    return rv;
}

template<int NB>
auto SWAR_multiplication(const std::vector<uint64_t> v) {
    return
    multiplicationTraverse<
        NB,
        zoo::swar::multiplication_OverflowUnsafe_SpecificBitCount<NB / 4>
    >(v);
}

namespace zoo::swar {

template<int NB, typename B>
constexpr auto normalMultiplication_OverflowUnsafe(
    SWAR<NB, B> multiplicand,
    SWAR<NB, B> multiplier
) noexcept {
    uint64_t v = 0;
    constexpr auto NLanes = sizeof(B) * 8 / NB;
    using S = SWAR<NB, B>;
    constexpr S Mask = S{S::LeastSignificantLaneMask};
    for(auto i = 0; i < NLanes; ++i) {
        auto newLane = (Mask & multiplicand).value() * (Mask & multiplier).value();
        v |= (newLane << (i*NB));
        multiplicand.m_v >>= NB;
        multiplier.m_v >>= NB;
    }
    return S{v};
}

}

template<int NB>
auto normal_multiplication(const std::vector<uint64_t> &v) {
    return
        multiplicationTraverse<
            NB, zoo::swar::normalMultiplication_OverflowUnsafe<NB>
        >(v);
}

template<int NB>
auto compare_mul(zoo::swar::SWAR<NB> m1, zoo::swar::SWAR<NB> m2) {
    using namespace zoo::swar;
    auto
        fN = normalMultiplication_OverflowUnsafe(m1, m2),
        fS = multiplication_OverflowUnsafe_SpecificBitCount<NB/2>(m1, m2);
    if(fN.value() != fS.value()) {
        auto
            fN1 = normalMultiplication_OverflowUnsafe(m1, m2),
            fS2 = multiplication_OverflowUnsafe_SpecificBitCount<NB/2>(m1, m2);
        throw 1;
    }
    return fN;
}

template<int NB>
auto compare_multiplications(const std::vector<uint64_t> &v) {
    return multiplicationTraverse<NB, compare_mul<NB>>(v);
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
    g.discard(100);
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
