#ifndef ZOO_JUNKYARD_CFS_CACHE_FRIENDLY_SEARCH
#define ZOO_JUNKYARD_CFS_CACHE_FRIENDLY_SEARCH

#include <zoo/algorithm/cfs.h>

namespace zoo {
namespace detail {

template<
    bool DoLower, bool DoUpper,
    typename Base, typename E, typename Comparator
>
// returns iterator to fst. element greater equal to e or end
auto cfsBounds(Base base, Base end, const E &e, Comparator c)
-> std::pair<Base, Base>
{
    auto size = end - base;
    if(0 == size) { return {base, end}; }
    auto ndx = 0;
    auto equivalent = [&](long i) {
        auto &contained = *(base + i);
        return not(c(e, contained) || c(contained, e));

    };
    auto successorOfLeaf = [&](long i) {
        while(not(i & 1)) { // i is in a higher branch
            if(0 == i) { return end; }
            // in a higher branch, return successor of parent
            i = (i >> 1) - 1;
        }
        // 0 < i => i has parent, i is in a lower branch
        return base + (i >> 1);
    };
    // called when base[i] is the first seen occurrence of e
    auto upperBoundOfEquivalentRange = [&](int i) {
        for(;;) { // invariant e == base[i]
            auto higherSubtree = (i << 1) + 2;
            if(size <= higherSubtree) {
                // base[i] == e, i does not have a higher subtree
                return successorOfLeaf(i);
            }
            if(equivalent(higherSubtree)) {
                i = higherSubtree;
                continue;
            }
            auto higherThanE = higherSubtree;
            do { // e == base[i] < base[higherThanE]
                // the result is past i up to higherThanE
                // dive into the lower subtree of higherThanE
                auto next = (higherThanE << 1) + 1;
                if(size <= next) {
                    // higherThanE is the lower-leaf in the higher
                    // subtree of i, thus higherThanE is the successor
                    return base + higherThanE;
                }
                higherThanE = next;
            } while(c(e, *(base + higherThanE)));
            // because we are in the higher subtree of i,
            // base[i] == e <= base[higherThanE]
            // we just checked not(e < base[higherThanE]) therefore
            // e == base[higherThanE]
            i = higherThanE;
        }
    };
    // e == base[i], will progress diving into the lower subtree
    // unless the lower subtree is less than e, where it will
    // hunt for e into the higher subtree
    auto lowerBoundOfEquivalentRange = [&](int i) {
        for(;;) { // e == base[i]
            auto lowerSubtree = (i << 1) + 1;
            if(size <= lowerSubtree) {
                // there is no earlier appearance of e:
                // let X be the tallest appearance of e, or the first to
                // be "hit" by the search algorithm.
                // If X is a high subtree, then parent(X) < e, thus
                // e can't appear in the low subtree of parent(X).
                // because i is in the low subtree of X, and it does
                // not have predecessors within X (it is the lowest leaf of X)
                // then it is the result
                return base + i;
            }
            while(*(base + lowerSubtree) < e) {
                // the earliest occurrence of e is past lowerSubtree up to
                // and including i
                lowerSubtree = (lowerSubtree << 1) + 2;
                if(size <= lowerSubtree) {
                    // reached a node below e without higher branch =>
                    // immediately precedes i, or that i is the
                    // beginning of the equal range
                    return base + i;
                }
            }
            // e == base[lowerSubtree]
            i = lowerSubtree;
        }
    };
    for(;;) {
        auto displaced = base + ndx;
        auto &cmp = *displaced;

        if(c(e, cmp)) {
            auto next = 2*ndx + 1;
            if(size <= next) {
                // no lower branch, not found
                return {base + ndx, base + ndx};
            }
            ndx = next;
            continue;
        }
        else if(c(cmp, e)) {
            auto next = 2*ndx + 2;
            if(size <= next) {
                // no higher branch, not found
                auto rv = successorOfLeaf(ndx);
                return {rv, rv};
            }
            ndx = next;
            continue;
        }
        // base[ndx] is the tallest in the tree occurrence of e
        // if ndx is a low subtree, base[ndx] == e < base[parent(ndx)]
        // if ndx is a high subtree, base[parent(ndx)] < e == base[ndx]
        auto upper = DoUpper ?
            upperBoundOfEquivalentRange(ndx):
            base + ndx;
        if(!DoLower) { return {base + ndx, upper}; }
        return { lowerBoundOfEquivalentRange(ndx), upper };
    }
}

}

template<
    typename Base,
    typename E, 
    typename Comparator = Less
>
Base cfsSearch(Base b, Base e, const E &v, Comparator c = Comparator{}) {
    return detail::cfsBounds<false, false>(b, e, v, c).first;
}

template<
    typename Base,
    typename E, 
    typename Comparator = Less
>
Base cfsLowerBoundOld(Base b, Base e, const E &v, Comparator c = Comparator{}) {
    return detail::cfsBounds<true, false>(b, e, v, c).first;
}

template<
    typename Base,
    typename E, 
    typename Comparator = Less
>
Base cfsHigherBoundOld(Base b, Base e, const E &v, Comparator c = Comparator{}) {
    return detail::cfsBounds<false, true>(b, e, v, c).second;
}

}

#endif
