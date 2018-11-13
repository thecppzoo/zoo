#ifndef ZOO_QUICKSORT
#define ZOO_QUICKSORT

#include <zoo/algorithm/moveRotation.h> // for moveRotate

#include <zoo/algorithm/less.h>

#include <array> // for temporary storage

namespace zoo {

template<typename FI>
struct ImplicitPivotResult {
    FI pivot_;
    long bias_;
};

/// \tparam FI is a forward iterator
/// \pre b != e
template<typename FI, typename Comparison>
auto implicitPivotPartition(FI b, FI e, Comparison cmp) ->
    ImplicitPivotResult<FI> 
{
    auto bias = 0;
    auto pivot = b++;
    /*if(e == b) { return pivot; }
    if(cmp(*b, *pivot)) {
        auto third = next(b);
        if(third == e) {
            moveRotation(*pivot, *b);
            return pivot;
        }
    }*/
    for(; b != e; ++b) {
        // invariant: ..., L0, P == *pivot, G0, G1, ... Gn, *b
        // where Lx means lower-than-pivot and Gx higher-equal-to-pivot
        if(!cmp(*b, *pivot)) {
            ++bias;
            continue;
        }
        --bias;
        // ..., L1, P == *pivot, G0, G1, ..., Gn, L0 == *b, X0, ...
        // The pivot is greater than the element:
        // insert *b into the lower partition:
        //  1. *b goes into the pivot position
        //  2. the pivot increases by one
        //  3. the element at pivot + 1, the new pivot, must be greater
        // than or equal to any Lx, the value of *pivot satisfies this
        // property.
        // These requirements can be satisfied by rotating the elements
        // at positions (pivot, b, pivot + 1)
        // ..., L1, L0, P == *pivot, G1, ..., Gn, G0 == *b, X0, ...
        auto oldPivot = pivot++;
        if(b == pivot) {
            moveRotation(*oldPivot, *pivot);
        } else {
            moveRotation(*oldPivot, *b, *pivot);
        }
        // tmp = *b, *b = *1, *1 = *0, *0 = tmp
        /*moveRotation(*oldPivot, *b);
        moveRotation(*b, *pivot);*/
    }
    return {pivot, bias};
}

template<typename I, typename Comparison = Less>
void quicksort(I begin, I end, Comparison cmp = Comparison{}) {
    if(begin == end) { return; }

    constexpr static const auto FrameCount = 64;
    struct Frame {
        I b, e;
    };
    std::array<Frame, FrameCount> stack;
    auto index = 0;

    for(;;) { // to do recursion in the lower partition
        auto result = implicitPivotPartition(begin, end, cmp);
        auto pivot = result.pivot_;
        auto bias = result.bias_;
        auto higherBegin = next(pivot);
        if(higherBegin == end) { // no higher-recursion needed
            if(begin != pivot) {
                end = pivot; // then just do lower recursion
                continue; // without leaving a frame
            }
            // there is no lower-recursion either
            if(!index) { return; }
            auto &frame = stack[--index];
            begin = frame.b;
            end = frame.e;
            continue;
        }
        // higher recursion needed
        if(begin == pivot) { // no lower recursion needed
            begin = higherBegin; // becomes the higher recursion
            continue;
        }
        // both lower and higher recursions needed, make frame for the larger
        // partition:
        // The smaller partition is less than or equal to half the elements:
        // size(smaller) <= size/2 => depth of recursion <= log2(N)
        if(0 < bias) { // lower partition is smaller
            stack[index] = { higherBegin, end };
            end = pivot;            
        } else { // higher partition is smaller
            stack[index] = { begin, pivot };
            begin = higherBegin;
        }
        if(FrameCount <= ++index) {
            throw std::runtime_error("quicksort stack exhausted");
        }
    }
}

template<typename FI, typename Comparison = Less>
bool is_sorted(FI begin, FI end, Comparison cmp = Comparison{}) {
    if(begin == end) { return true; }
    auto old = begin++;
    while(begin != end) {
        if(not cmp(*old, *begin)) { return false; }
        old = begin++;
    }
    return true;
}

}

#endif
