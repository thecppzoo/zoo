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
void transformToCFS(Output output, Input base, Input end) {
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

template<typename Base, typename E>
// returns iterator to fst. element greater equal to e or end
Base cfsLowerBound(Base base, Base end, const E &e) {
    auto size = end - base;
    if(0 == size) { return base; }
    auto ndx = 0;
    auto successorOfLeaf = [&]() {
        while(not(ndx & 1)) {
            if(0 == ndx) { return end; }
            // in a higher branch, return successor of parent
            ndx = (ndx >> 1) - 1;
        }
        // 0 < ndx => ndx has parent, ndx is in a lower branch
        return base + (ndx >> 1);
    };
    auto predecessorOfLeaf = [&]() {
        auto original = ndx;
        for(;;) {
            if(0 == ndx) { return base + original; }
            if(not(ndx & 1)) { break; } // ndx is in a high-branch
            ndx = (ndx >> 1);
        }
        // Not in the root, on a higher branch
        return base + (ndx >> 1) - 1; 
    };
    for(;;) {
        auto displaced = base + ndx;
        auto &cmp = *displaced;
        if(e < cmp) {
            auto next = 2*ndx + 1;
            if(size <= next) { return base + ndx; }
            ndx = next;
        }
        else if(cmp < e) {
            auto next = 2*ndx + 2;
            if(size <= next) { return successorOfLeaf(); }
            ndx = next;
        }
        else { return displaced; }
    }
}

template<typename I>
bool validHeap(I base, int current, int max) {
    for(;;) {
        auto higherSubtree = current*2 + 2;
        if(max <= higherSubtree) {
            if(max < higherSubtree) { return true; } // current is a leaf
            // max == higherSubtree, there is only the lower subtree
            auto subLeaf = higherSubtree - 1;
            return *(base + subLeaf) <= *(base + current);
        }
        // there are both branches
        auto &element = *(base + current);
        auto lowerSubtree = higherSubtree - 1;
        if(element < *(base + lowerSubtree)) { return false; }
        if(!validHeap(base, lowerSubtree, max)) { return false; }
        if(*(base + higherSubtree) < element) { return false; }
        current = higherSubtree;
    }
}

template<typename I>
bool validHeap(I begin, I end) {
    return validHeap(begin, 0, end - begin);
}

template<typename R>
bool validHeap(const R &range) {
    //std::cout << range << std::endl;
    return validHeap(range.begin(), range.end());
}

}

#endif
