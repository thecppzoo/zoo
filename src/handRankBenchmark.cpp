#include "ep/timing/benchmark.h"
#include "ep/Poker.h"
#include "ep/Floyd.h"


#include <iostream>

// 1100
// 1010
// 0110
// 1001
// 0101
// 0011
constexpr uint64_t nextSubset(uint64_t v) {
    // find first available element to include
    auto trailingZero = __builtin_ctzll(v);
    auto withoutTrailingZero = (v >> trailingZero);
    auto inverted = ~withoutTrailingZero;
    auto onesCount = __builtin_ctzll(inverted);
    auto firstAvailable = trailingZero + onesCount;
    // leave onesCount - 1 in the least significant parts of the result
    // and one bit set at firstAvailable, the rest is the same
    auto leastOnes = ~(~uint64_t(0) << (onesCount - 1));
    leastOnes |= (uint64_t(1) << firstAvailable);
    auto mask = ~uint64_t(0) << firstAvailable;
    return (v & mask) | leastOnes;
}

static_assert(2 == nextSubset(1), "");
static_assert(8 == nextSubset(4), "");
static_assert(9 == nextSubset(6), "");
// 0001. 1010 == 0x58
// 1000. 0110 == 0x61
static_assert(0x61 == nextSubset(0x58), "");

struct ProgressiveGen {
    uint64_t m_state = 0x7F;
    uint64_t generate() {
        auto rv = nextSubset(m_state);
        if(7 != __builtin_popcountll(rv)) {
            std::cerr << "Error, " << std::hex << m_state << ' ' << rv << std::endl;
            throw;
        }
        return m_state = rv;
    }
};

int
#ifdef TIME_HAND_RANK
main
#else
time_hand_rank
#endif
(int argc, char **argv) {
    //std::random_device device;
    //std::mt19937 generator(device());
    //std::minstd_rand generator(device());
    ProgressiveGen pg;
    auto count = 1 << 25;
    long base;

    if(1 < argc) {
        count = std::stol(argv[1]);
    }

    auto fraction = [&](const char *n, int t) {
        std::cout << n << ' ' << t/1000 << ' ' << 1.0*(t - base)/base << ' ';
        if(t - base) {
            std::cout << count/(t - base);
        } else {
            std::cout << count/base;
        }
        std::cout << " | ";
    };

    uint64_t sideEffect1 = 0;
    unsigned sideEffect2 = 0;

    auto justHandGeneration = [&]() {
        //auto cards = ep::floydSample<ep::NRanks*ep::NSuits, 7>(generator);
        auto cards = pg.generate();
        sideEffect1 ^= cards;
    };

    auto handGenerationAndRanking = [&]() {
        //auto cards = ep::floydSample<ep::NRanks*ep::NSuits, 7>(generator);
        auto cards = pg.generate();
        sideEffect1 ^= cards;
        auto ranks = ep::SWARRank(cards);
        ep::CSet hand = { ep::convert(ranks), ranks };
        sideEffect2 ^= ep::handRank(hand).code;
    };

    auto suitGeneration = [&]() {
        //auto cards = ep::floydSample<ep::NRanks*ep::NSuits, 7>(generator);
        auto cards = pg.generate();
        auto ranks = ep::SWARRank(cards);
        sideEffect1 ^= ep::convert(ranks).value();
    };

    std::cout << "Elements " << count << ':';
    base = ep::timing::countedBenchmark(justHandGeneration, count);
    fraction("Base", base);

    auto ranking = ep::timing::countedBenchmark(handGenerationAndRanking, count);
    auto conversion = ep::timing::countedBenchmark(suitGeneration, count);

    fraction("Ranked", ranking);
    fraction("Conversion", conversion);
    std::cout << std::hex << sideEffect1 << ' ' << sideEffect2 << std::endl;
    return 0;
}

