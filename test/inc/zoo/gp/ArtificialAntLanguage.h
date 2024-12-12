#ifndef ZOO_GP_ARTIFICIAL_ANT_LANGUAGE_H
#define ZOO_GP_ARTIFICIAL_ANT_LANGUAGE_H

#include<array>
#include <cstddef>

struct ArtificialAnt {
    constexpr static inline std::array Tokens {
        "TR", "TL", "Move", "IFA", "Prog", "P3"
    };
    constexpr static inline std::array ArgumentCount {
        0, 0, 0, 1, 2, 3
    };
};

enum TokenEnum: char {
    TurnRight,
    TurnLeft,
    Move,
    IFA,
    Prog2,
    Prog3
};

template<typename T>
constexpr auto TerminalsCount() {
    for(size_t ndx = 0; ndx < size(T::ArgumentCount); ++ndx) {
        if(T::ArgumentCount[ndx]) { return ndx; }
    }
    return size(T::ArgumentCount);
}

template<typename T>
constexpr auto NonTerminalsCount() {
    return size(T::ArgumentCount) - TerminalsCount<T>();
}


#endif
