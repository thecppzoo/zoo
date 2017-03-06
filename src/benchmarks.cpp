#include "detail/ep/mechanisms.h"
#include "ep/nextSubset.h"
#include "ep/timing/benchmark.h"

#include <iostream>

struct ProgressiveGenerator {
    uint64_t m_state = 1;

    constexpr ProgressiveGenerator() = default;
    constexpr ProgressiveGenerator(int v): m_state(v) {}

    constexpr uint64_t next() {
        auto rv = m_state;
        m_state = ep::nextSubset(rv);
        return rv;
    }

    constexpr uint64_t operator()() { return next(); }
};

int isFOAK(uint64_t cards) {
    ep::SWARRank ranks(cards);
    ep::RankCounts counts(ranks);
    auto foaks = ep::core::greaterEqualSWAR<4>(counts.counts());
    return foaks;
}

ep::SWARRank straights(ep::RankCounts rc) {
    auto present = ep::core::greaterEqualSWAR<1>(rc.counts());
    auto acep = 
        present.at(ep::abbreviations::rA) ?
            uint64_t(0xF) << ep::abbreviations::rA :
            uint64_t(0);
    auto ranks = present.value();
    auto sk = ranks << 4;
    auto rak = ranks & sk;
    auto rt = acep | (ranks << 16);
    auto rqj = rak << 8;
    auto rakqj = rak & rqj;
    return ep::SWARRank(rakqj & rt);
}

uint64_t flush(uint64_t ranks) {
    auto ranksMask = ep::core::makeBitmask<4, uint64_t>(1);
    for(auto n = 4; n--; ) {
        auto masked = ranks & ranksMask;
        auto count = __builtin_popcountll(masked);
        if(5 <= count) { return masked; }
        ranksMask <<= 1;
    }
    return 0;
}

/// \return true if the cards have a three of a kind and a pair
int isTOAKplusPAIRv1(uint64_t cards) {
    ep::SWARRank ranks(cards);
    ep::RankCounts counts(ranks);
    auto toaks = ep::core::greaterEqualSWAR<3>(counts.counts());
    if(!toaks) { return false; }
    auto toakIndex = toaks.top();
    auto cleared = counts.clearAt(toakIndex);
    auto pairs = ep::core::greaterEqualSWAR<2>(cleared.counts());
    return pairs;
}

int colorBlindRank(uint64_t cards) {
    ep::SWARRank ranks(cards);
    ep::RankCounts counts(ranks);
    auto toaks = ep::core::greaterEqualSWAR<3>(counts.counts());
    if(toaks) {
        auto foaks = ep::core::greaterEqualSWAR<4>(counts.counts());
        if(foaks) {
            return ep::FOUR_OF_A_KIND;
        }
        auto bestToak = counts.best();
        auto cleared = counts.clearAt(bestToak);
        auto pairs = cleared.greaterEqual<2>();
        if(pairs) { return ep::FULL_HOUSE; }
        auto s = straights(counts);
        if(s) { return ep::STRAIGHT; }
        return ep::THREE_OF_A_KIND;
    }
    auto s = straights(counts);
    if(s) { return ep::STRAIGHT; }
    auto pairs = counts.greaterEqual<2>();
    if(pairs) {
        auto bestPair = pairs.top();
        auto cleared = counts.clearAt(bestPair);
        auto secondPair = cleared.greaterEqual<2>();
        if(secondPair) { return ep::TWO_PAIRS; }
        return ep::PAIR;
    }
    return ep::HIGH_CARDS;
}

inline int encode(int hand, int h, int l) {
    return (hand << 28) | (h << 14) | l;
}

//#include <x86intrin.h>

int rankFlushG(uint64_t cards) {
    auto s = straights(ep::RankCounts(ep::SWARRank(cards)));
    if(s) {
        return encode(ep::STRAIGHT_FLUSH, 0, 1 << s.top());
    }
    for(auto count = __builtin_popcountll(cards); __builtin_expect(5 < count--, 0); ) {
        cards &= cards - 1;
    }
    auto high = ep::toRanks(cards);
    return encode(ep::FLUSH, 0, high);
}

int rankFlushCheat(uint64_t cards) {
    if(straights(ep::RankCounts(ep::SWARRank(cards)))) { return ep::STRAIGHT_FLUSH; }
    return ep::FLUSH;
}

int whole(uint64_t cards) {
    ep::SWARRank ranks(cards);
    ep::RankCounts counts(ranks);
    auto toaks = ep::core::greaterEqualSWAR<3>(counts.counts());
    if(toaks) {
        auto foaks = ep::core::greaterEqualSWAR<4>(counts.counts());
        if(foaks) {
            auto high = foaks.top();
            auto cleared = ranks.clear(high);
            auto kicker = cleared.top();
            return encode(ep::FOUR_OF_A_KIND, high, 1 << kicker);
        }
        auto bestToak = counts.best();
        auto cleared = counts.clearAt(bestToak);
        auto pairs = cleared.greaterEqual<2>();
        if(pairs) {
            return encode(ep::FULL_HOUSE, 1 << bestToak, 1 << pairs.top());
        }

        auto flushCards = flush(cards);
        if(flushCards) { return rankFlushG(flushCards); }
        auto s = straights(counts);
        if(s) { return encode(ep::STRAIGHT, 0, 1 << s.top()); }
        auto best = cleared.best();
        auto low = 1 << best;
        auto recleared = cleared.clearAt(best);
        auto worst = recleared.best();
        low |= 1 << worst;
        return encode(ep::THREE_OF_A_KIND, 0, low);
    }

    auto flushCards = flush(cards);
    if(flushCards) { return rankFlushG(flushCards); }
    auto s = straights(counts);
    if(s) { return encode(ep::STRAIGHT, 0, 1 << s.top()); }

    auto pairs = counts.greaterEqual<2>();
    if(pairs) {
        auto bestPair = pairs.best();
        auto cleared = counts.clearAt(bestPair);
        auto secondPairs = cleared.greaterEqual<2>();
        if(secondPairs) {
            auto worsePair = secondPairs.best();
            auto recleared = cleared.clearAt(worsePair);
            auto high = (1 << bestPair) | (1 << worsePair);
            auto tricleared = recleared.clearAt(worsePair);
            return encode(ep::TWO_PAIRS, high, 1 << tricleared.best());
        }
        auto top = cleared.best();
        auto recleared = cleared.clearAt(top);
        auto middle = recleared.best();
        auto tricleared = recleared.clearAt(middle);
        auto bottom = tricleared.best();
        return encode(ep::PAIR, 1 << bestPair, (1 << top) | (1 << middle) | (1 << bottom));
    }
    cards &= cards - 1;
    cards &= cards - 1;
    return encode(ep::HIGH_CARDS, 0, ep::toRanks(cards));
}

int wholeCheat(uint64_t cards) {
    ep::SWARRank ranks(cards);
    ep::RankCounts counts(ranks);
    auto toaks = counts.greaterEqual<3>();
    if(toaks) {
        auto foaks = counts.greaterEqual<4>();
        if(foaks) {
            return ep::FOUR_OF_A_KIND;
        }
        auto bestToak = counts.best();
        auto cleared = counts.clearAt(bestToak);
        auto pairs = cleared.greaterEqual<2>();
        if(pairs) { return ep::FULL_HOUSE; }

        auto flushCards = flush(cards);
        if(flushCards) { return rankFlushCheat(flushCards); }
        auto s = straights(counts);
        if(s) { return ep::STRAIGHT; }

        return ep::THREE_OF_A_KIND;
    }

    auto flushCards = flush(cards);
    if(flushCards) { return rankFlushCheat(flushCards); }
    auto s = straights(counts);
    if(s) { return ep::STRAIGHT; }

    auto pairs = counts.greaterEqual<2>();
    if(pairs) {
        auto bestPair = pairs.best();
        auto cleared = counts.clearAt(bestPair);
        auto secondPair = cleared.greaterEqual<2>();
        if(secondPair) { return ep::TWO_PAIRS; }
        return ep::PAIR;
    }
    return ep::HIGH_CARDS;    
}

uint64_t sideEffect = 0;

template<typename Callable>
long driver(ProgressiveGenerator gen, int count, Callable fun) {
    auto nested = [&]() {
        for(auto c = count; c--; ) {
            auto cards = gen.next();
            sideEffect ^= cards;
            fun(cards);
        }
    };
    return ep::timing::benchmark(nested);
};


int (*operation)(uint64_t) = isTOAKplusPAIRv1;

#include <iostream>

#include <iomanip>

int
#ifdef BENCHMARKS
main
#else
benchmarks
#endif
(int argc, const char *argv[]) {
    std::cout << std::setprecision(4);
    ProgressiveGenerator gen(0x7F);
    auto count = 133784560;
    auto toakPlusPairs = 0;
    auto xorCheck = -1;
    auto empty = [](uint64_t) {};
    auto v1fun = [&](uint64_t cards) {
        auto result = operation(cards);
        if(ep::THREE_OF_A_KIND == result) { ++toakPlusPairs; }
        xorCheck ^= result;
        /*auto st = straights(ep::SWARRank(cards));
        if(0x11111 == (0x11111 & cards)) {
            std::cout << std::endl << ep::SWARRank(cards) << ' ' << st << std::endl;
        }
        if(st) {
            static auto strs = 10;
            if(0 < strs--) {
                std::cout << std::endl << ep::SWARRank(cards);
            }
        }*/
    };
    auto indicators = [=](long time, long base) {
        auto divisor = (time == base ? base : time - base);
        std::cout << time << ' ' << 1.0*divisor/base << ' ' << count/divisor <<
            ' ' << 1000.0*divisor/count;
    };

    auto base = driver(gen, count, empty);
    indicators(base, base);
    std::cout.flush();
    auto v1 = driver(gen, count, v1fun);
    std::cout << " |1 ";
    indicators(v1, base);
    std::cout.flush();
    operation = [](uint64_t c) { return colorBlindRank(c); };
    auto v2 = driver(gen, count, v1fun);
    std::cout << " |cb ";
    indicators(v2, base);
    std::cout.flush();
    operation = [](uint64_t c) -> int { return flush(c); };
    auto v3 = driver(gen, count, v1fun);
    std::cout << " |f ";
    indicators(v3, base);
    std::cout.flush();
    operation = [](uint64_t c) -> int { return whole(c); };
    auto v4 = driver(gen, count, v1fun);
    std::cout << " |w ";
    indicators(v4, base);
    std::cout.flush();

    operation = [](uint64_t c) -> int {
        ep::SWARRank ranks(c); ep::SWARSuit ss = ep::convert(ranks);
        ep::CSet cs = { ss, ranks };
        return ep::handRank(cs);
    };
    auto v5 = driver(gen, count, v1fun);
    std::cout << " |h ";
    indicators(v5, base);
    std::cout.flush();

    operation = [](uint64_t c) -> int {
        /*ep::SWARRank ranks(c); ep::SWARSuit ss = ep::convert(ranks);
        ep::CSet cs = { ss, ranks };
        return ep::naive::handRank(cs);*/
        return wholeCheat(c);
    };
    auto v6 = driver(gen, count, v1fun);
    std::cout << " |c ";
    indicators(v6, base);
  
    std::cout << ' ' << toakPlusPairs << ' ' << std::hex << sideEffect << ' ' <<
        xorCheck << std::endl;
    return 0;
}

