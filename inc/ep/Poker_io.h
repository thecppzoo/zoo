#pragma once

#include "PokerTypes.h"

#include <istream>

namespace ep {

auto g_rankLetters = "23456789TJQKA0#%";
auto g_suitLetters = "shcd";

std::ostream &operator<<(std::ostream &out, SWARRank ranks) {
    while(ranks.value()) {
        auto rank = ranks.top();
        for(auto suits = ranks.at(rank); suits; ) {
            auto suit = __builtin_ctzll(suits);
            out << g_rankLetters[rank] << g_suitLetters[suit];
            suits ^= (1 << suit);
        }
        ranks = ranks.clear(rank);
    }
    return out;
}

}
