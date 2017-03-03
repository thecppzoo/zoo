#include "ep/timing/benchmark.h"
#include "obsolete/Poker.h"
#include "ep/Poker_io.h"
#include "ep/Floyd.h"
#include "ep/nextSubset.h"


#include <iostream>

template<int K> struct ProgressiveGen {
    uint64_t m_state = ~(~uint64_t(0) << K);

    uint64_t generate() {
        auto rv = m_state;
        m_state = ep::nextSubset(m_state);
        /*if(K != __builtin_popcountll(rv) || rv <= m_state) {
            std::cerr << "Error, " << std::hex << m_state << ' ' << rv << std::endl;
            throw;
        }*/
        return rv;
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
    ProgressiveGen<7> pg;
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

    auto straightFlushes = 0;
    auto handGenerationAndRanking = [&]() {
        //auto cards = ep::floydSample<ep::NRanks*ep::NSuits, 7>(generator);
        auto cards = pg.generate();
        sideEffect1 ^= cards;
        auto ranks = ep::SWARRank(cards);
        using namespace ep;
        ep::CSet hand = { ep::convert(ranks), ranks };
        auto hr = ep::handRank(hand);
        sideEffect2 ^= hr.code;
    };

    auto suitGeneration = [&]() {
        //auto cards = ep::floydSample<ep::NRanks*ep::NSuits, 7>(generator);
        auto cards = pg.generate();
        auto ranks = ep::SWARRank(cards);
        sideEffect1 ^= ep::convert(ranks).value();
    };

    std::cout << "Elements " << count << ':';
    base = ep::timing::countedBenchmark(justHandGeneration, count);
    fraction("Subset generation", base);

    pg = ProgressiveGen<7>();
    auto ranking = ep::timing::countedBenchmark(handGenerationAndRanking, count);
    auto conversion = ep::timing::countedBenchmark(suitGeneration, count);

    fraction("Ranked", ranking);
    fraction("Conversion to suit", conversion);
    std::cout << std::hex << sideEffect1 << ' ' << sideEffect2 << std::endl;
    return 0;
}

