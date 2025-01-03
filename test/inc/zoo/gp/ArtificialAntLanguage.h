#ifndef ZOO_GP_ARTIFICIAL_ANT_LANGUAGE_H
#define ZOO_GP_ARTIFICIAL_ANT_LANGUAGE_H

#include<array>
#include<cstddef>

enum ArtificialAntEnum: char {
    TurnRight,
    TurnLeft,
    Move,
    IFA,
    Prog2,
    Prog3
};

struct ArtificialAnt {
    constexpr static inline std::array Tokens {
        "TR", "TL", "Move", "IFA", "Prog", "P3"
    };
    constexpr static inline std::array ArgumentCount {
        0, 0, 0, 1, 2, 3
    };

    constexpr static inline auto TerminalsCount = 3;

    using TokenEnum = ArtificialAntEnum;
};

#endif
