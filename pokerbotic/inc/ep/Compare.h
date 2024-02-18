#pragma once

#include "ep/Poker.h"
#include "PokerTypes.h"

#include <utility>
#include <array>

namespace ep {

struct Unclassified {};

template<typename Community>
int compare(Community c, CardSet comm, CardSet p1, CardSet p2) {
    return compare(comm, p1, p2);
}

struct Classification {
    uint64_t mask;
    enum type {
        SPADES, FOUR_SPADES, THREE_SPADES_TWO_HEARTS,
        THREE_SPADES_HEART_DIAMOND, TWO_SPADES_TWO_HEARTS, TWO_SPADES
    };
};

inline int classify(uint64_t cards) {
    //static_assert(NCommunity + NPlayer < 10, "Two flushes possible");
    constexpr auto suitMask = core::makeBitmask<4, uint64_t>(1);
    auto maskCopy = suitMask;
    for(auto suit = 0; suit < 4; ++suit) {
        auto filtered = cards & maskCopy;
        auto counts = __builtin_popcountll(filtered);
        if(3 <= counts) { return suit; }
        cards >>= 1;
    }
    return 4;
}

inline uint64_t flush(Colored *c, uint64_t cards) {
    auto shifted = cards >> c->shift;
    constexpr auto suitMask = core::makeBitmask<4, uint64_t>(1);
    auto filtered = shifted & suitMask;
    auto count = __builtin_popcountll(filtered);
    if(5 <= count) { return filtered; }
    return 0;
}

inline uint64_t flush(ColorBlind *, uint64_t) { return 0; }

template<
    template<typename> class Continuation,
    typename... Args
>
inline int classifyCommunity(CardSet comm, Args &&... arguments) {
    auto ccards = comm.cards();
    constexpr auto suitMask = core::makeBitmask<4, uint64_t>(1);
    auto maskCopy = suitMask;
    for(auto suit = 0; suit < 4; ++suit) {
        auto filtered = ccards & maskCopy;
        auto counts = __builtin_popcountll(filtered);
        if(3 <= counts) {
            Colored co = { suit };
            return Continuation<Colored>::execute(&co, comm, std::forward<Args>(arguments)...);
        }
        ccards >>= 1;
    }
    ColorBlind cb;
    return Continuation<ColorBlind>::execute(&cb, comm, std::forward<Args>(arguments)...);
}
    template<typename C> struct Continuation {
        static int execute(C *c, CardSet co, CardSet p1, CardSet p2) {
            return handRank(c, co | p1).code - handRank(c, co | p2).code;
        }
    };
inline int compareByCommunity(CardSet comm, CardSet p1, CardSet p2) {

    return classifyCommunity<Continuation>(comm, p1, p2);
    auto ccards = comm.cards();
    constexpr auto suitMask = core::makeBitmask<4, uint64_t>(1);
    auto maskCopy = suitMask;
    for(auto suit = 0; suit < 4; ++suit) {
        auto filtered = ccards & maskCopy;
        auto counts = __builtin_popcountll(filtered);
        if(3 <= counts) {
            Colored c = { suit };
            return handRank(&c, comm | p1).code - handRank(&c, comm | p2).code;
        }
        ccards >>= 1;
    }
    ColorBlind cb;
    return handRank(&cb, comm | p1).code - handRank(&cb, comm | p2).code;
}

template<typename Context>
inline int compareByCommunity(Context c, CardSet community, CardSet p1, CardSet p2) {
    return handRank(c, community | p1).code - handRank(c, community | p2).code;
}

}