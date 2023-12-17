#ifndef ZOO_CFS_CACHE_FRIENDLY_SEARCH
#define ZOO_CFS_CACHE_FRIENDLY_SEARCH

#include "zoo/algorithm/less.h"
#include "zoo/meta/log.h"

#ifndef SIMPLIFY_INCLUDES
// because of std::declval needed to default comparator
#include <utility>
// because of std::decay needed to decay deferenced iterator
#include <type_traits>

#include <ciso646>
#endif

namespace zoo {

template<typename Output, typename Input>
void transformToCFS(Output output, Input base, Input end) {
    auto s = end - base;
    auto logP = meta::logFloor(s + 1); // n
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
    auto top = 2*excess;
    for(auto ndx = 0ll; ndx < top; ndx += 2) {
        *output++ = *(base + ndx);
    }
}

namespace detail {

enum SearchPolicy {
    ONLY_LOWER_BOUND,
    ONLY_UPPER_BOUND,
    RANGE
};

template<
    SearchPolicy Policy, typename Base, typename E, typename Comparator
>
auto cfsBound(
    Base base,
    Base supremum,
    std::size_t size, std::size_t ndx,
    const E &e,
    Comparator c
) -> std::pair<Base, Base> {
    if(0 == size) { return {supremum, supremum}; }
    auto
        // case size is odd: the last valid index is a higher subtree
        // iH*2 + 1 == size <=> iH == (size - 1)/2 == size >> 1
        // iL == iH == (size - 1) >> 1
        // case size is even: the last valid index is a lower subtree
        // iH*2 == size <=> iH == size/2 == size >> 1
        // iL == iH - 1 == (size - 1) >> 1
        firstInvalidHigherParent = size >> 1,
        firstInvalidLowerParent = (size - 1) >> 1;
    // infimum: largest position *before* e
    auto infimum = supremum;
    for(;;) {
        auto current = base + ndx;
        auto &cmp = *current;
        if(ONLY_UPPER_BOUND == Policy ? not c(e, cmp) : c(cmp, e)) {
            if(firstInvalidLowerParent <= ndx) { break; }
            ndx = (ndx << 1) + 2;
            continue;
        }
        infimum = current;
        if(firstInvalidHigherParent <= ndx) { break; }
        ndx = (ndx << 1) + 1;
    }
    if(RANGE != Policy) { return {infimum, infimum}; }

    // Assumes streaks of equivalent elements are much less than the size
    //
    // Looking for the end of the streak of equivalent elements to e.
    // In the case ndx is a left subtree, we have
    // e <= base[ndx] <= base[parent(ndx)]
    // if e < base[parent(ndx)] the upper bound is in
    // [successor(ndx), parent(ndx)]
    ndx = infimum - base;
    while(1 & ndx) {
        auto parent = ndx >> 1;
        auto current = base + parent;
        auto &cmp = *current;        
        if(c(e, cmp)) {
            supremum = base + parent;
            break;
        }
        ndx = parent;
    }
    // Either there is no parent, or the parent is an upper bound, the supremum
    // can only be in the higher subtree of ndx
    auto upper =
        cfsBound<ONLY_UPPER_BOUND>(
            base, supremum, size, (ndx << 1) + 2, e, c
        ).first;
    return {infimum, upper};
}

}

template<
    typename Base,
    typename E, 
    typename Comparator = Less
>
auto cfsLowerBound(Base b, Base e, const E &v, Comparator c = Comparator{}) {
    return
        detail::cfsBound<detail::ONLY_LOWER_BOUND>(b, e, e - b, 0, v, c).first;
}

template<
    typename Base,
    typename E, 
    typename Comparator = Less
>
auto cfsHigherBound(Base b, Base e, const E &v, Comparator c = Comparator{}) {
    return
        detail::cfsBound<detail::ONLY_UPPER_BOUND>(b, e, e - b, 0, v, c).first;
}

template<
    typename Base,
    typename E, 
    typename Comparator = Less
>
auto cfsEqualRange(Base b, Base e, const E &v, Comparator c = Comparator{}) {
    return detail::cfsBound<detail::RANGE>(b, e, e - b, 0, v, c);
}

struct ValidResult {
    bool worked_;
    long failureLocation;
    operator bool() const { return worked_; }
};

template<typename I, typename Comparator = Less>
auto validHeap(
    I base, long current, long max, Comparator c = Comparator{}
) -> ValidResult {
    for(;;) {
        auto higherSubtree = current*2 + 2;
        if(max <= higherSubtree) {
            if(max < higherSubtree) { return {true, 0} ; } // current is a leaf
            // max == higherSubtree, there is only the lower subtree
            auto subLeaf = higherSubtree - 1;
            return {not c(*(base + current), *(base + subLeaf)), subLeaf};
        }
        // there are both branches
        auto &element = *(base + current);
        auto lowerSubtree = higherSubtree - 1;
        if(c(element, *(base + lowerSubtree))) { return {false, lowerSubtree}; }
        auto recursionResult = validHeap(base, lowerSubtree, max);
        if(!recursionResult) { return recursionResult; }
        if(c(*(base + higherSubtree), element)) {
            return {false, higherSubtree};
        }
        current = higherSubtree;
    }
}

template<typename I, typename Comparator = Less>
auto validHeap(I begin, I end, Comparator c = Comparator{}) {
    return validHeap(begin, 0, end - begin, c);
}

template<typename R, typename Comparator = Less>
bool validHeap(const R &range, Comparator c = Comparator{}) {
    return validHeap(range.begin(), range.end(), c);
}

}

#endif
