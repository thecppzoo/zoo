#pragma once

#include "PokerTypes.h"

#include <istream>

namespace ep { namespace core {

inline std::ostream &operator<<(std::ostream &out, SWARRank ranks) {
    static auto g_rankLetters = "23456789TJQKA0#%";
    static auto g_suitLetters = "shdc";
    if(!ranks.value()) { return out << '-'; }
    for(;;) {
        auto rank = ranks.top();
        for(auto suits = ranks.at(rank); ; ) {
            auto suit = core::msb(suits);
            out << g_suitLetters[suit] << g_rankLetters[rank];
            suits &= ~(1 << suit);
            if(!suits) { break; }
            out << '.';
        }
        ranks = ranks.clear(rank);
        if(!ranks.value()) { return out; }
        out << '_';
    }
}

}}
