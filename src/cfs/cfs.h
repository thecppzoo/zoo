#ifndef ZOO_CFS_CACHE_FRIENDLY_SEARCH
#define ZOO_CFS_CACHE_FRIENDLY_SEARCH

namespace zoo {

constexpr unsigned long long log2Floor(unsigned long long arg) {
    return 63 - __builtin_clzll(arg);
}

constexpr unsigned long long log2Ceiling(unsigned long long arg) {
    return 63 - __builtin_clzll(2*arg - 1);
}

template<typename Output, typename Input>
void transform(Output output, Input base, Input end) {
    auto s = end - base;
    auto logP = log2Floor(s + 1); // n
    auto power2 = 1ul << logP;
    auto fullSubtreeSize = power2 - 1;
    // Full tree has (2^n) - 1 elements
    auto excess = s - fullSubtreeSize;
    auto twiceExcess = excess << 1;
    for(auto straddle = power2; 1 < straddle; ) {
        auto nextStraddle = straddle >> 1;
        auto unshifted = nextStraddle - 1;
        auto shifted = unshifted + excess;
        if(shifted < twiceExcess) { // below the "cut"
            auto i = straddle - 1;
            auto prevStraddle = straddle << 1;
            auto addendum = 0;
            do {
                *output++ = *(base + i + addendum);
                addendum += prevStraddle;
            } while(i + addendum < twiceExcess);
            auto halvedAddendum = addendum >> 1;
            shifted += halvedAddendum;
        }
        while(shifted < s) { // the root may be below the cut
            *output++ = *(base + shifted);
            shifted += straddle;
        }
        straddle = nextStraddle;
    }

    // now just write the excess leaves
    for(auto ndx = 0ul, top = 2*excess; ndx < top; ndx += 2) {
        *output++ = *(base + ndx);
    }
}

template<typename Container>
inline auto reserveIfAvailable(Container &c, std::size_t s) ->
decltype(c.reserve(s), (void)0) {
    c.reserve(s);
}
inline void reserveIfAvailable(...) {}

template<typename Sorted>
Sorted sortedToCacheFriendlySearch(const Sorted &s) {
    Sorted rv;
    reserveIfAvailable(rv, s.size());
    sortedToCacheFriendlySearch(back_inserter(rv), 0, s);
    return rv;
}


}

#endif
