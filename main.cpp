#include "ep/Poker.h"
#include "ep/Poker_io.h"
#include "ep/CascadeComparisons.h"

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
    [](std::mt19937 &generator) { return ep::floydSample<ep::NRanks*ep::NSuits, 7>(generator); };
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
            return ep::straights(ep::CSet::rankSet(cards));
        };
    auto toakCheck = [](uint64_t cards) {
        ep::core::SWAR<4, uint64_t> nibbles(cards);
        ep::RankCounts counts = nibbles;
        auto toaks = counts.greaterEqual<3>();
        if(toaks) {
            static auto count = 5;
            if(0 < --count) {
                std::cout << nibbles << ' ' << names[toaks.bestIndex()] << std::endl;
            }
        }
        return unsigned(bool(toaks));
    };
    auto nullgen = [](std::mt19937 &generator) -> uint64_t { return 0xB02; };
    //sampleGen = nullgen;
    auto empty = benchmark(experiment, count, generator);
    //checkStraight = reallyCheck;
    checkStraight = toakCheck;
    auto nonEmpty = benchmark(experiment, count, generator);
    auto normal = straights;
    auto diff = 1.0*(nonEmpty - empty);

    /*checkStraight = [](uint64_t cards) {
        return ep::straightFrontCheck(ep::CSet::rankSet(cards));
    };
    straights = 0;
    auto frontCheck = benchmark(experiment, count, generator);
    auto fronted = straights;
    straights = 0;
    checkStraight = [](uint64_t cards) {
        return ep::uncheckStraight(ep::CSet::rankSet(cards));
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

