#include "ep/PokerTypes.h"

using ul = uint64_t;

ul popcountsByFourLogic(ul input) {
    return ep::core::popcount<1>(input);
}

uint16_t popcountLookupBy4Nibbles[1 << 16];
uint16_t popcountLookupBy1Short[1 << 16];

union bits64 {
    ul all64;
    uint16_t by16[4];
    uint8_t by8[8];
   

    bits64() = default;
    bits64(ul v): all64(v) {}
};

ul popcountsByLookupBy4(bits64 arg) {
    arg.by16[0] = popcountLookupBy4Nibbles[arg.by16[0]];
    arg.by16[1] = popcountLookupBy4Nibbles[arg.by16[1]];
    arg.by16[2] = popcountLookupBy4Nibbles[arg.by16[2]];
    arg.by16[3] = popcountLookupBy4Nibbles[arg.by16[3]];
    return arg.all64;
}

ul popcountsBy16Logic(ul input) {
    return ep::core::popcount<3>(input);
}

ul popcountsByAssembler16(bits64 arg) {
    arg.by8[0] = __builtin_popcount(arg.by8[0]);
    arg.by8[1] = __builtin_popcount(arg.by8[1]);
    arg.by8[2] = __builtin_popcount(arg.by8[2]);
    arg.by8[3] = __builtin_popcount(arg.by8[3]);
    arg.by8[4] = __builtin_popcount(arg.by8[4]);
    arg.by8[5] = __builtin_popcount(arg.by8[5]);
    arg.by8[6] = __builtin_popcount(arg.by8[6]);
    arg.by8[7] = __builtin_popcount(arg.by8[7]);
    return arg.all64;
}

unsigned table24[1 << 24];

ul by24(ul v) {
    ul mask = (1 << 24) - 1;
    ul rv = table24[v & mask];
    v >>= 24;
    rv |= (uint64_t(table24[v & mask]) << 24);
    v >>= 24;
    rv |= (uint64_t(table24[v & mask]) << 48);
    return rv;
}

ul popcountsByLookupBy16(bits64 arg) {
    constexpr auto mask = ep::core::makeBitmask<16>((uint64_t(1) << 13) - 1);
    arg.all64 &= mask;
    arg.by16[0] = popcountLookupBy1Short[arg.by16[0]];
    arg.by16[1] = popcountLookupBy1Short[arg.by16[1]];
    arg.by16[2] = popcountLookupBy1Short[arg.by16[2]];
    arg.by16[3] = popcountLookupBy1Short[arg.by16[3]];
    return arg.all64;
}

#include <chrono>
#include <utility>

ul result = 0;

uint64_t LCG(uint64_t v) {
    return 134775813 * v + 1;
}

uint64_t seed = 134775811;

template<typename Callable, typename... Args>
long benchmark(Callable &&call, int count, Args &&... arguments) {
    auto now = std::chrono::high_resolution_clock::now();
    for(auto n = count; n--; ) {
        seed ^= call(std::forward<Args>(arguments)...);
        seed = LCG(seed);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - now);
    return diff.count();
}

#include <iostream>

int
#ifdef BENCHMARK
main
#else
mainpc
#endif
(int argc, const char *argv[]) {
    constexpr auto count = 1 << 27;
    seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto doLCG = []() {
        return 0;
    };
    auto justLCG = benchmark(doLCG, count);
    auto doLookups = []() {
        return popcountsByLookupBy4(seed);
    };
    auto lookups = benchmark(doLookups, count);
    auto doLogic = []() {
        return popcountsBy16Logic(seed);
    };
    auto logic = benchmark(doLogic, count);
    auto regularPattern = []() {
        auto static n = 0ull;
        return popcountsByLookupBy4(n++);
    };
    auto base = 1.0 * justLCG;
    auto fraction = [&](const char *n, int t) {
        std::cout << n << ' ' << t/1000 << ' ' << (t - base)/base << ' ' <<
            count/t << " | ";
    };
    fraction("LCG    ", justLCG);
    fraction("Lookups", lookups);
    fraction("Assembler", logic);
    fraction("Regular", benchmark(regularPattern, count));
    fraction("Lookup/truncated", benchmark(
        []() { return popcountsByLookupBy16(seed); },
        count
    ));
    fraction("Logic4", benchmark([]() { return ep::core::popcount<1>(seed); }, count));
    fraction("Logic-16", benchmark([]() { return ep::core::popcount_logic<3>(seed); }, count));
    std::cout << std::hex << seed << std::endl;
    return 0;
}

