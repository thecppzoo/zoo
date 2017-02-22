#include "ep/metaBinomial.h"
#include "ep/core/Swar.h"
#include "ep/core/metaLog.h"

#include <iostream>

//#include <boost/math/special_functions/binomial.hpp>

namespace ep {

/// Number of numbers in the card set
constexpr auto NNumbers = 13;
/// Number of suits
constexpr auto NSuits = 4;

constexpr auto SuitBits = 1 << core::metaLogCeiling(NNumbers);
static_assert(16 == SuitBits, "");
constexpr auto NumberBits = 1 << core::metaLogCeiling(NSuits);
static_assert(4 == NumberBits, "");

using SWARSuit = core::SWAR<SuitBits>;
using SWARNumber = core::SWAR<NumberBits>;

constexpr int positiveIndex1Better(int index1, int index2) {
    return index2 - index1;
}

int bestBit(uint64_t val) {
    return __builtin_ctzll(val);
}

template<int Size, typename T>
struct Counted {
    Counted() = default;
    constexpr Counted(core::SWAR<Size, T> bits):
        m_counts(
            core::popcount<core::metaLogCeiling(Size - 1) - 1>(bits.value())
        )
    {}

    constexpr core::SWAR<Size, T> counts() { return m_counts; }

    template<int N>
    constexpr Counted greaterEqual() {
        return core::greaterEqualSWAR<N>(m_counts);
    }
    constexpr operator bool() { return m_counts.value(); }

    // \note Best is smaller number
    constexpr int bestIndex() {
        return bestBit(m_counts.value()) / Size;
    }

    constexpr Counted clearAt(int index) {
        auto rv = m_counts;
        rv.clearAt(index);
        return rv;
    }

    protected:
    core::SWAR<Size, T> m_counts;
};

template<int Size, typename T>
constexpr Counted<Size, T>
makeCounted(core::SWAR<Size, T> bits) { return bits; }

struct MonotoneFlop {
    constexpr static auto equivalents = NSuits;
    constexpr static auto element_count = Choose<NNumbers, 3>::value;
    constexpr static auto max_repetitions = 0;
};

/// \todo There are several subcategories here:
/// The card with the non-repeated suit is highest, pairs high, middle, pairs low, lowest
struct TwoToneFlop {
    constexpr static auto equivalents = PartialPermutations<NSuits, 2>::value;
    constexpr static auto element_count =
        NNumbers * Choose<NNumbers, 2>::value;
    constexpr static auto max_repetitions = 1;
};

struct RainbowFlop {
    constexpr static auto equivalents = Choose<NSuits, 3>::value;
    constexpr static auto element_count = NNumbers*NNumbers*NNumbers;
    constexpr static auto max_repetitions = 2;
};

template<typename...> struct Count {
    constexpr static uint64_t value = 0;
};
template<typename H, typename... Tail>
struct Count<H, Tail...> {
    constexpr static uint64_t value =
        H::equivalents*H::element_count + Count<Tail...>::value;
};

static_assert(4 * Choose<13, 3>::value == Count<MonotoneFlop>::value, "");
static_assert(12*13*Choose<NNumbers, 2>::value == Count<TwoToneFlop>::value, "");
static_assert(4*13*13*13 == Count<RainbowFlop>::value, "");
static_assert(
    Choose<52, 3>::value == Count<MonotoneFlop, TwoToneFlop, RainbowFlop>::value,
    ""
);

}


#include <stdint.h>
#include <array>

constexpr std::array<uint16_t, 4> toArray(uint64_t v) {
    using us = uint16_t;
    return {
        us(v & 0xFFFF), us((v >> 16) & 0xFFFF),
        us((v >> 32) & 0xFFFF), us((v >> 48) & 0xFFFF)
    };
}

namespace ep {

struct CSet {
    SWARSuit m_bySuit;
    SWARNumber m_byNumber;

    constexpr CSet operator|(CSet o) {
        return { m_bySuit | o.m_bySuit, m_byNumber | m_byNumber };
    }

    constexpr auto suitCounts() { return makeCounted(m_bySuit); }
    constexpr auto numberCounts() { return makeCounted(m_byNumber); }

    constexpr static unsigned numberSet(uint64_t orig) {
        auto rv = orig;
        for(auto ndx = NSuits; --ndx;) {
            orig = orig >> SuitBits;
            rv |= orig;
        }
        constexpr auto isolateMask = (uint64_t(1) << NNumbers) - 1;
        return rv & isolateMask;
    }

    constexpr unsigned numberSet() { return numberSet(m_bySuit.value()); }
};

constexpr Counted<SuitBits, uint64_t> flushes(Counted<SuitBits, uint64_t>  ss) {
    return ss.greaterEqual<5>();
}

#define RARE(v) if(__builtin_expect(bool(v), false))

unsigned isStraight(unsigned numberSet) {
    constexpr auto NumberOfHandCards = 7;
    auto rv = numberSet;
    // 2 1 0 9 8 7 6 5 4 3 2 1 0 bit index
    // =========================
    // 2 3 4 5 6 7 8 9 T J Q K A
    // - 2 3 4 5 6 7 8 9 T J Q K &
    // - - 2 3 4 6 5 6 7 8 9 T J &
    // - - - 2 3 4 5 6 7 8 9 T J &
    // - - - A - - - - - - - - - |
    // - - - A 2 3 4 5 6 7 8 9 T &
    
    
    rv &= (rv >> 1);  // two
    if(!rv) { return rv; }
    rv &= (rv >> 1); // three in sequence
    if(!rv) { return rv; }
    rv &= (rv >> 1); // four in sequence
    if(!rv) { return rv; }
    RARE(1 & numberSet) {
        // the argument had an ace, insert it as if for card number 1
        rv |= (1 << (NNumbers - 4));
    }
    rv &= (rv >> 1);
    return rv;
}

std::ostream &pRanks(std::ostream &out, unsigned ranks) {
    auto letters = "AKQJT98765432!@#$%^&*()";
    for(auto ndx = 32; ndx--; ) {
        if(ranks & (1 << ndx)) { out << letters[ndx]; }
        else { out << '-'; }
    }
    return out;
}

inline unsigned uncheckStraight(unsigned numberSet);

inline unsigned straightFrontCheck(unsigned rankSet) {
    constexpr auto mask = (1 << 9) | (1 << 4);
    if(rankSet & mask) { return uncheckStraight(rankSet); }
    /*auto notStraight = isStraight(rankSet);
    if(notStraight) {
        static auto count = 5;
        if(5 == count) { pRanks(std::cerr, mask) << '\n' << std::endl; }
        if(--count < 0) { return 0; }
        pRanks(std::cerr, rankSet) << " | ";
        pRanks(std::cerr, notStraight) << std::endl;
    }*/
    return 0;
}

inline unsigned uncheckStraight(unsigned numberSet) {
    constexpr auto NumberOfHandCards = 7;
    auto rv = numberSet;
    auto hasAce = (1 & numberSet) ? (1 << (NNumbers - 4)) : 0;
    // 2 1 0 9 8 7 6 5 4 3 2 1 0 bit index
    // =========================
    // 2 3 4 5 6 7 8 9 T J Q K A
    // - 2 3 4 5 6 7 8 9 T J Q K &
    // - - 2 3 4 6 5 6 7 8 9 T J &
    // - - - 2 3 4 5 6 7 8 9 T J &
    // - - - A - - - - - - - - - |
    // - - - A 2 3 4 5 6 7 8 9 T &
    
    
    rv &= (rv >> 1);  // two
    rv &= (rv >> 1); // three in sequence
    rv &= (rv >> 1); // four in sequence
    rv |= hasAce;
    rv &= (rv >> 1);
    return rv;
}

struct ComparisonResult {
    int ifDecided;
    bool decided;
};

inline int bestKicker(
    Counted<NSuits, uint64_t> p1s, Counted<NSuits, uint64_t> p2s
) {
    return positiveIndex1Better(p1s.bestIndex(), p2s.bestIndex());
}

inline int bestFourOfAKind(
    Counted<NSuits, uint64_t> p1s, Counted<NSuits, uint64_t> p2s,
    Counted<NSuits, uint64_t> p1foaks, Counted<NSuits, uint64_t> p2foaks
) {
    auto p1BestNdx = p1foaks.bestIndex();
    auto p2BestNdx = p2foaks.bestIndex();
    auto diff = positiveIndex1Better(p1BestNdx, p2BestNdx);
    if(diff) { return diff; }
    return bestKicker(p1s.clearAt(p1BestNdx), p2s.clearAt(p2BestNdx));
}

inline ComparisonResult winnerPotentialFullHouseGivenThreeOfAKind(
    Counted<NSuits, uint64_t> p1s, Counted<NSuits, uint64_t> p2s,
    Counted<NSuits, uint64_t> p1toaks, Counted<NSuits, uint64_t> p2toaks
) {
    auto p1BestThreeOfAKindIndex = p1toaks.bestIndex();
    auto p1WithoutBestThreeOfAKind = p1s.clearAt(p1BestThreeOfAKindIndex);
    auto p1FullPairs = p1WithoutBestThreeOfAKind.greaterEqual<2>();
    RARE(p1FullPairs) { // p1 is full house
        RARE(p2toaks) {
            auto p2BestThreeOfAKindIndex = p2toaks.bestIndex();
            auto bestDominantIndex =
                positiveIndex1Better(
                    p1BestThreeOfAKindIndex, p2BestThreeOfAKindIndex
                );
            if(0 < bestDominantIndex) {
                // p2's best three of a kind is not as good as p1's,
                // even if a full house itself will lose
                return { 1, true };
            }
            auto p2WithoutBestThreeOfAKind =
                p2s.clearAt(p2BestThreeOfAKindIndex);
            auto p2FullPairs = p2WithoutBestThreeOfAKind.greaterEqual<2>();
            RARE(p2FullPairs) {
                if(bestDominantIndex) { return { bestDominantIndex, true }; }
                return {
                    positiveIndex1Better(
                        p1FullPairs.bestIndex(), p2FullPairs.bestIndex()
                    ),
                    true
                };
            }
        }
        return { 1, true };
    }
    // p1 is not a full-house
    auto p2BestThreeOfAKindIndex = p2toaks.bestIndex();
    auto p2WithoutBestThreeOfAKind = p2s.clearAt(p2BestThreeOfAKindIndex);
    auto p2FullPairs = p2WithoutBestThreeOfAKind.greaterEqual<2>();
    RARE(p2FullPairs) { return { -1, true }; }
    // neither is a full house
    return { 0, false };
}

inline ComparisonResult winnerPotentialFullHouseOrFourOfAKind(
    Counted<NSuits, uint64_t> p1s, Counted<NSuits, uint64_t> p2s
) {
    // xoptx How can any be full house or four of a kind?
    // xoptx What is the optimal sequence of checks?
    auto p1ThreeOfAKinds = p1s.greaterEqual<3>();
    auto p2ThreeOfAKinds = p2s.greaterEqual<3>();
    RARE(p1ThreeOfAKinds) { // p1 has a three of a kind
        RARE(p2ThreeOfAKinds) { // p2 also has a three of a kind
            auto p1FourOfAKinds = p1s.greaterEqual<4>();
            auto p2FourOfAKinds = p2s.greaterEqual<4>();
            RARE(p1FourOfAKinds) {
                RARE(p2FourOfAKinds) {
                    return {
                        bestFourOfAKind(p1s, p2s, p1FourOfAKinds, p2FourOfAKinds),
                        true
                    };
                }
                return { 1, true };
            } else RARE(p2FourOfAKinds) { return { -1, true }; }
            // No four of a kind
            return
                winnerPotentialFullHouseGivenThreeOfAKind(
                    p1s, p1ThreeOfAKinds, p2s, p2ThreeOfAKinds
                );
        }
        // p2 lacks three of a kind, can not be full-house nor four-of-a-kind
        auto p1BestThreeOfAKindIndex = p1ThreeOfAKinds.bestIndex();
        auto p1WithoutBestThreeOfAKind = p1s.clearAt(p1BestThreeOfAKindIndex);
        auto p1FullPairs = p1WithoutBestThreeOfAKind.greaterEqual<2>();
        RARE(p1FullPairs) { return { 1, true }; }
        // not a full house, but perhaps four of a kind?
        RARE(p1s.greaterEqual<4>()) { return { 1, true }; }
        // neither is full house or better, undecided
    } // p1 lacks three of a kind, can not be full-house nor four-of-a-kind
    else RARE(p2ThreeOfAKinds) {
        auto p2BestThreeOfAKindIndex = p2ThreeOfAKinds.bestIndex();
        auto p2WithoutBestThreeOfAKind = p2s.clearAt(p2BestThreeOfAKindIndex);
        auto p2FullPairs = p2s.greaterEqual<2>();
        RARE(p2FullPairs) { return { -1, true }; }
        // p2 is not a full house, but perhaps a four of a kind?
        RARE(p2s.greaterEqual<4>()) { return { -1, true }; }
    }
    return { 0, false };
}

inline int bestFlush(unsigned p1, unsigned p2) {
    for(auto count = 5; count--; ) {
        auto
            bestP1 = bestBit(p1),
            bestP2 = bestBit(p2);
        auto diff = positiveIndex1Better(bestP1, bestP2);
        if(diff) { return diff; }
    }
    return 0;
}

inline int bestStraight(unsigned s1, unsigned s2) {
    return positiveIndex1Better(bestBit(s1), bestBit(s2));
}

struct FullHouseResult {
    bool isFullHouse;
    int bestThreeOfAKind, bestPair;
};

inline FullHouseResult isFullHouse(Counted<NSuits, uint64_t> counts) {
    auto toaks = counts.greaterEqual<3>();
    RARE(toaks) {
        auto bestTOAK = toaks.bestIndex();
        auto withoutBestTOAK = counts.clearAt(bestTOAK);
        auto fullPairs = withoutBestTOAK.greaterEqual<2>();
        RARE(fullPairs) { return { true, bestTOAK, fullPairs.bestIndex() }; }
    }
    return { false, 0, 0 };
}

template<int N>
inline int bestKickers(
    Counted<NSuits, uint64_t> p1s, Counted<NSuits, uint64_t> p2s
) {
    for(auto counter = N;;) {
        auto p1Best = p1s.bestIndex();
        auto rv = positiveIndex1Better(p1Best, p2s.bestIndex());
        if(rv) { return rv; }
        if(!--counter) { break; }
        // xoptx
        p1s = p1s.clearAt(p1Best);
        p2s = p2s.clearAt(p1Best);
    }
    return 0;
}

/// \todo Handle two pairs and pairs
/// \todo optimize this.  Look for comments with the word xoptx
int winner(CSet community, CSet p1, CSet p2) {
    // There are three independent criteria
    // n-of-a-kind, straights and flushes
    // since flushes dominate straigts, and have in texas holdem about the
    // same probability than straights, the first inconditional check is
    // for flushes.
    // xoptx all the comparison operations should not be unary but binary
    // in the player cards and community, this would allow calculating the
    // community cards only once.
    p1 = p1 | community;
    auto p1Suits = makeCounted(p1.m_bySuit);
    auto p1Flushes = flushes(p1Suits);
    p2 = p2 | community ;
    auto p2Suits = p2.suitCounts();
    auto p2Flushes = flushes(p2Suits);
    // xoptx in the case of flushes, three of a kind, etc., there
    // should be a constexpr template predicate that indicates whether
    // current classification allows for flush, straight, four of a kind, etc
    // xoptx also the community cards determine whether flush, full house can
    // happen
    RARE(p1Flushes) {
        RARE(p2Flushes) {
            // p2 also has a flush, there is the need to check for either being
            // a straight flush
            auto p1FlushSuit = p1Flushes.bestIndex();
            auto p1Flush = p1.m_bySuit.at(p1FlushSuit);
            auto p1Straights = isStraight(p1Flush);
            auto p2FlushSuit = p2Flushes.bestIndex();
            auto p2Flush = p2.m_bySuit.at(p2FlushSuit);
            auto p2Straights = isStraight(p2Flush);
            RARE(p1Straights) { // p1 is straight flush
                // note: never needed to calculate number repetitions!
                RARE(p2Straights) { // both are
                    return bestStraight(p1Straights, p2Straights);
                }
                return 1;
            }
            RARE(p2Straights) { // p2 is straight flush
                // note: never needed to calculate numbers!
                return -1;
            }
            // Both are flush but none straight flush
            // xoptx How can any be full house or four of a kind?
            // xoptx What is the optimal sequence of checks?
            auto p1NumberCounts = makeCounted(p1.m_byNumber);
            auto p2NumberCounts = makeCounted(p2.m_byNumber);
            auto fullHouseOrFourOfAKind =
                winnerPotentialFullHouseOrFourOfAKind(
                    p1NumberCounts, p2NumberCounts
                );
            RARE(fullHouseOrFourOfAKind.decided) {
                return fullHouseOrFourOfAKind.ifDecided;
            }
            // flushes, but not straight-flush, nor full-house nor four of a kind
            return bestFlush(p1Flush, p2Flush);
        }
        // p2 is not a flush and p1 is a flush
        // xoptx this check seems premature, only if p2 is a full house is justified
        auto p1s = makeCounted(p1.m_byNumber);
        auto p2s = makeCounted(p2.m_byNumber);
        auto fullHouseOrFourOfAKind =
            winnerPotentialFullHouseOrFourOfAKind(p1s, p2s);
        RARE(fullHouseOrFourOfAKind.decided) {
            return fullHouseOrFourOfAKind.ifDecided;
        }
        // p2 is not a full house nor a four of a kind
        return 1;
    }
    // p1 is no flush
    auto p1s = makeCounted(p1.m_byNumber);
    auto p2s = makeCounted(p2.m_byNumber);
    RARE(p2Flushes) {
        auto p2FlushSuit = p2Flushes.bestIndex();
        auto p2Flush = p2.m_bySuit.at(p2FlushSuit);
        auto p2Straights = isStraight(p2Flush);
        RARE(p2Straights) { return -1; } // p2 is straight flush
        auto fullHouseOrFourOfAKind =
            winnerPotentialFullHouseOrFourOfAKind(p1s, p2s);
        RARE(fullHouseOrFourOfAKind.decided) {
            return fullHouseOrFourOfAKind.ifDecided;
        }
        return -1;
    }
    // No flushes
    auto p1Set = p1.numberSet();
    auto p1Straights = isStraight(p1Set);
    auto p2Set = p2.numberSet();
    auto p2Straights = isStraight(p2Set);
    RARE(p1Straights | p2Straights) {
        auto fhofoak = winnerPotentialFullHouseOrFourOfAKind(p1s, p2s);
        RARE(fhofoak.decided) { return fhofoak.ifDecided; }
        if(p1Straights) {
            if(p2Straights) {
                return positiveIndex1Better(p1Straights, p2Straights);
            }
            return 1;
        }
        return -1;
    }
    auto p1Pairs = p1s.greaterEqual<2>();
    auto p2Pairs = p2s.greaterEqual<2>();
    RARE(!p1Pairs) {
        RARE(!p2Pairs) {
            return bestFlush(p1.numberSet(), p2.numberSet());
        }
        return -1;
    }
    RARE(!p2Pairs) { return 1; }
    auto p1toaks = p1s.greaterEqual<3>();
    auto p2toaks = p2s.greaterEqual<3>();
    RARE(p1toaks) {
        RARE(p2toaks) {
            auto potentialFull =
                winnerPotentialFullHouseGivenThreeOfAKind(
                    p1s, p2s, p1toaks, p2toaks
                );
            RARE(potentialFull.decided) { return potentialFull.ifDecided; }
            // no full house or better
            auto
                p1Toak = p1toaks.bestIndex(),
                p2Toak = p2toaks.bestIndex();
            auto diff = positiveIndex1Better(p1Toak, p2Toak);
            RARE(!diff) {
                auto p1WithoutToak = p1s.clearAt(p1Toak);
                auto p2WithoutToak = p2s.clearAt(p2Toak);
                return bestKickers<2>(p1WithoutToak, p2WithoutToak);
            }
            return diff;
        }
        return 1;
    }
    // double pairs and pairs
    return 0;
}

}

#include <iostream>
#include <chrono>
#include <utility>
#include "ep/Floyd.h"

template<typename Callable, typename... Args>
long benchmark(Callable &&call, Args &&... arguments) {
    auto now = std::chrono::high_resolution_clock::now();
    call(std::forward<Args>(arguments)...);
    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - now);
    return diff.count();
}

uint64_t (*sampleGen)(std::mt19937 &generator) =
    [](std::mt19937 &generator) { return ep::floydSample<ep::NNumbers*ep::NSuits, 7>(generator); };
unsigned (*checkStraight)(uint64_t) = [](uint64_t) -> unsigned { return 0; };
unsigned straights;

void experiment(unsigned count, std::mt19937 &generator) {
    while(count--) {
        auto cards =// ep::floydSample<ep::NNumbers*ep::NSuits, 7>(generator);
            sampleGen(generator);
        if(checkStraight(cards)) { ++straights; }
    }
};

auto names = "AKQJT98765432";

std::ostream &operator<<(std::ostream &out, ep::core::SWAR<4, uint64_t> s) {
    auto suites = "chsd";
    auto set = s.value();
    while(set) {
        auto bit = __builtin_ctzll(set);
        auto mask = uint64_t(1) << bit;
        out << names[bit / 4] << suites[bit % 4];
        set ^= mask;
    }
    return out;
}

int main(int argc, char** argv) {
    std::random_device device;
    std::mt19937 generator(device());
    auto count = 1 << 24;
    straights = 0;
    auto reallyCheck =
        [](uint64_t cards) -> unsigned {
            return ep::uncheckStraight(ep::CSet::numberSet(cards));
        };
    auto toakCheck = [](uint64_t cards) {
        ep::core::SWAR<4, uint64_t> nibbles(cards);
        auto counts = ep::makeCounted(nibbles);
        auto toaks = counts.greaterEqual<3>();
        if(toaks) {
            static auto count = 5;
            if(0 < --count) {
                std::cout << nibbles << ' ' << names[toaks.bestIndex()] << std::endl;
            }
        }
        return unsigned(bool(toaks));
    };
    auto empty = benchmark(experiment, count, generator);
    //checkStraight = reallyCheck;
    checkStraight = toakCheck;
    auto nonEmpty = benchmark(experiment, count, generator);
    auto normal = straights;
    auto diff = 1.0*(nonEmpty - empty);

    /*checkStraight = [](uint64_t cards) {
        return ep::straightFrontCheck(ep::CSet::numberSet(cards));
    };
    straights = 0;
    auto frontCheck = benchmark(experiment, count, generator);
    auto fronted = straights;
    straights = 0;
    checkStraight = [](uint64_t cards) {
        return ep::uncheckStraight(ep::CSet::numberSet(cards));
    };
    auto unchecked = benchmark(experiment, count, generator);
    auto uncheckCount = straights;*/
    std::cout << empty << std::endl;
    std::cout << normal << ' ' << nonEmpty << ' ' << diff/empty << ' ' << count/diff << std::endl;
    /*auto diff2 = 1.0*(frontCheck - empty);
    std::cout << fronted << ' ' << frontCheck << ' ' << diff2/diff << std::endl;
    auto diff3 = 1.0*(unchecked - empty);
    std::cout << uncheckCount << ' ' << unchecked << ' ' << diff3/diff << std::endl;*/
    return 0;
}

