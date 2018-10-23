#ifndef ZOO_CFS_CACHE_FRIENDLY_SEARCH
#define ZOO_CFS_CACHE_FRIENDLY_SEARCH

#ifndef SIMPLIFY_INCLUDES
#include <utility>
#endif

namespace zoo {

template<typename Iterator>
struct LessForIterated {
    using type = decltype(*std::declval<Iterator>());
    bool operator()(const type &left, const type &right) {
        return left < right;
    }
};

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

template<bool Early, typename Base, typename E, typename Comparator>
// returns iterator to fst. element greater equal to e or end
Base cfsLowerBound_impl(Base base, Base end, const E &e, Comparator c) {
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
    for(;;) {
        auto displaced = base + ndx;
        auto &cmp = *displaced;
        if(c(e, cmp)) {
            auto next = 2*ndx + 1;
            if(size <= next) { return base + ndx; }
            ndx = next;
            continue;
        }
        else if(c(cmp, e)) {
            auto next = 2*ndx + 2;
            if(size <= next) { return successorOfLeaf(); }
            ndx = next;
            continue;
        }
        // cmp == e
        if(Early) { return displaced; }
        do {
            auto lowerSubtree = (ndx << 1) + 1;
            if(size <= lowerSubtree) {
                // got to a leaf.
                // the element at ndx is the minimum of either
                // the whole structure, thus the result, or
                // the minimum of a higher subtree with parent X
                // however, if base[X] == e the search would have gone to
                // the lower subtree of X and this code would not have been
                // reached, thus base[X] < e.  The minimum of a higher subtree
                // is the successor of the parent of the higher subtree
                return base + ndx;
            }
            ndx = lowerSubtree;
        }  while(not(c(*(base + ndx), e) || c(e, *(base + ndx))));
        // difference found!
        // search in the lower subtree as if the element
        // has not been found yet.  If e is in the lower
        // subtree, the result will be there, if it is not, then
        // the search will return the immediate successor, which
        // is ndx, thus ndx would be the first occurrence of e
    }
}

template<
    typename Base,
    typename E, 
    typename Comparator = LessForIterated<Base>
>
Base cfsSearch(Base b, Base e, const E &v, Comparator c = Comparator{}) {
    return cfsLowerBound_impl<true>(b, e, v, c);
}

template<
    typename Base,
    typename E, 
    typename Comparator = LessForIterated<Base>
>
Base cfsLowerBound(Base b, Base e, const E &v, Comparator c = Comparator{}) {
    return cfsLowerBound_impl<false>(b, e, v, c);
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
