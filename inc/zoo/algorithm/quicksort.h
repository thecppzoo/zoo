#ifndef ZOO_QUICKSORT
#define ZOO_QUICKSORT

#include <zoo/algorithm/moveRotation.h> // for moveRotate

#include <zoo/algorithm/less.h>

#include <array> // for temporary storage

namespace zoo {

/// \tparam FI is a forward iterator
/// \pre b != e
template<typename FI, typename Comparison>
FI implicitPivotPartition(FI b, FI e, Comparison cmp) {
    auto pivot = b++;
    for(; b != e; ++b) {
        // invariant: ..., L0, P == *pivot, G0, G1, ... Gn, *b
        // where Lx means lower-than-pivot and Gx higher-equal-to-pivot
        if(!cmp(*b, *pivot)) { continue; }

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
    return pivot;
}

template<typename I, typename Comparison = Less>
void quicksort(I begin, I end, Comparison cmp = Comparison{}) {
    if(begin == end) { return; }

    constexpr static const auto FrameCount = 100;
    struct Frame {
        I b, e;
    };
    std::array<Frame, FrameCount> stack;
    auto index = 0;
    for(;;) { // outer loop for doing a frame
        for(;;) { // to do recursion in the lower partition
            auto pivot = implicitPivotPartition(begin, end, cmp);
            auto higherBegin = next(pivot);
            if(higherBegin == end) { // no higher-recursion needed
                if(begin == pivot) { // neither lower-recursion
                    if(!index--) { return; }
                    break;
                }
                end = pivot; // lower-recursion
                continue;
            }
            // higher recursion needed
            if(begin == pivot) { // no lower recursion needed
                begin = higherBegin;
                continue; 
            }
            stack[index] = { higherBegin, end };
            end = pivot;
            if(FrameCount <= ++index) {
                throw std::runtime_error("quicksort stack exhausted");
            }
        }
        auto &frame = stack[index];
        begin = frame.b;
        end = frame.e;
    }
}

}

#endif
