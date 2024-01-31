#include "ep/Compare.h"
#include "ep/Floyd.h"
#include "ep/nextSubset.h"
#include "ep/timing/benchmark.h"

#include "ep/Cases.h"

#include <iostream>

template<typename C> struct Comm {
    template<typename Start>
    static int execute(C *c, ep::CardSet community, long &count, int &rv, Start start) {
        constexpr auto stop = uint64_t(1) << 52;
        for(auto p1 = uint64_t(3);;) {
            auto player1 = ep::core::deposit(community.cards(), p1);
            auto commP1 = community | player1;
            for(auto p2 = uint64_t(3);;) {
                auto player2 = ep::core::deposit(commP1.cards(), p2);
                rv ^= ep::compareByCommunity(c, community, player1, player2);
                //rv ^= ep::compare(community, player1, player2);
                //std::cout << std::hex << player2 << ' ' << player1 << ' ' << community << std::endl;
                ++count;
                if(!(count % 100'000'000)) {
                    auto now = std::chrono::high_resolution_clock::now();
                    auto elapsed = now - start;
                    auto micros =
                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                            elapsed
                        ).count();
                    std::cout << count << ' ' << micros/count << ' ' <<
                        1000*count/micros << ' ' << micros << ' ' << rv << std::endl;
                }
                p2 = ep::nextSubset(p2);
                if(stop <= p2) { break; }
            }
            p1 = ep::nextSubset(p1);
            if(stop <= p1) { break; }
        }
        return rv;
    }
};

int
#ifdef HAND_COMPARISON
main
#else
handComparison
#endif
(int argc, const char *argv[]) {
    auto rv = 0;
    long count = 0;
    constexpr auto stop = uint64_t(1) << 52;
    auto start = std::chrono::high_resolution_clock::now();
    for(auto community = uint64_t(0x1F) << 0;;) {
        ep::classifyCommunity<Comm>(community, count, rv, start);
        community = ep::nextSubset(community);
        if(stop <= community) { break; }
    }
    return rv;
}