#pragma once

#include "obsolete/Poker.h"
#include "ep/Poker_io.h"
#include "ep/Floyd.h"

namespace ep { namespace naive {

inline HandRank handRank(CSet hand) {
    auto rankCounts = hand.rankCounts();
    auto suitCounts = hand.suitCounts();
    auto ranks = hand.rankSet();
    auto toaks = noaks<3>(rankCounts);
    HandRank rv;
    RARE(toaks) {
        auto foaks = noaks<4>(rankCounts);
        RARE(foaks) {
            static_assert(TotalHand < 8, "There can be four-of-a-kind and straight-flush");
            return { FOUR_OF_A_KIND, 0, 0 };
        }
        auto toak = toaks.best();
        auto without = rankCounts.clearAt(toak);
        auto pairs = noaks<2>(without);
        RARE(pairs) {
            static_assert(TotalHand < 8, "There can be full-house and straight-flush");
            return { FULL_HOUSE, 0, 0 };
        }
        rv = HandRank(THREE_OF_A_KIND, 0, 0);
    }
    auto flushes = suitCounts.greaterEqual<5>();
    RARE(flushes) {
        static_assert(TotalHand < 2*5, "No two flushes");
        auto suit = flushes.best();
        auto royal = hand.m_bySuit.at(suit);
        auto isIt = straights(royal);
        RARE(isIt) {
            return { STRAIGHT_FLUSH, 0, 0 };
        }
        return { FLUSH, 0, 0 };
    }
    auto str = straights(ranks);
    RARE(str) { return { STRAIGHT, 0, 0 }; }
    RARE(rv) { return rv; }
    auto pairs = rankCounts.greaterEqual<2>();
    if(pairs) {
        auto top = pairs.best();
        auto without = pairs.clearAt(top);
        if(without) {
            return { TWO_PAIRS, 0, 0 };
        }
        return { PAIR, 0, 0 };
    }
    return { HIGH_CARDS, 0, 0 };
}
    
}}
