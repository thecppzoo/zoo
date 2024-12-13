#ifndef ZOO_QUICKSORT
#define ZOO_QUICKSORT

#include <zoo/algorithm/moveRotation.h> // for moveRotate

#include <zoo/algorithm/less.h>

#include <array> // for temporary storage
#include <stdexcept>

#ifdef _MSC_VER
#include <iso646.h>
#endif

namespace zoo {

/// ImplicitPivotResult is returned by implicitPivotPartition and contains:
/// - `pivot_`: An iterator to the pivot's final position.
/// - `bias_`: A signed value indicating the relative sizes of the partitions.
template<typename FI>
struct ImplicitPivotResult {
    FI pivot_;
    long bias_;
};

/// \brief Partitions the range `[b, e)` around an implicit pivot (the first
/// element).
///
/// During partitioning:
/// - Elements less than the pivot are moved to the left.
/// - Elements greater than or equal to the pivot are moved to the right.
/// - The pivot itself is placed in its final position.
///
/// The function also computes a bias value indicating the relative sizes of
/// the two partitions:
/// - `bias > 0`: More elements in the "greater-than-or-equal-to" partition.
/// - `bias < 0`: More elements in the "less-than" partition.template<typename FI, typename Comparison>
///
/// /// @pre `b != e`.
///
/// @note This function operates in \(O(n)\) time complexity and requires \(O(1)\)
/// additional space.
///
/// @throws Propagates exceptions thrown by `cmp` or `moveRotate`.
///
/// @example Example usage:
/// ```cpp
/// std::vector<int> data = {4, 7, 2, 8, 5};
/// auto result =
///     zoo::implicitPivotPartition(
///         data.begin(),
///         data.end(),
///         std::less<>{}
///     );
/// assert(result.pivot_ == data.begin() + 2); // Final pivot position
/// assert(*result.pivot_ == 4);               // Pivot value
auto implicitPivotPartition(FI b, FI e, Comparison cmp) ->
    ImplicitPivotResult<FI> 
{
    auto bias = 0;
    auto pivot = b++;
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

/// Implementation of quicksort.
/// @param begin An iterator that satisfies the ForwardIterator concept,
///    pointing to the beginning of the range to be sorted.
/// @param end The end of the range (exclusive).
/// @param cmp A callable object defining a strict weak ordering.
template<typename I, typename Comparison = Less>
void quicksort(I begin, I end, Comparison cmp = Comparison{}) {
    if(begin == end) { return; }

    constexpr static const auto FrameCount = 64;
    struct Frame {
        I b, e;
    };
    std::array<Frame, FrameCount> stack;
    auto index = 0;

    for(;;) {
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

/// is_sorted scans a range and returns wheter it is sorted.
/// @param begin The beginning of the range to partition.
/// @param end The end of the range to partition (exclusive).
/// @param cmp The comparison function to compare elements.
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

/// quickselect implements the retrieval of the nth element
/// @param begin A forward iterator indicating the beginning of the range.
/// @param nth A forward iterator indicating the position of interest. Note
///     that it also serves as pivot. Items to the left of it will precede
///     it in cmp(). items to the right will succeed it.
/// @param end A forward iterator indicating the end of the range 
///     (exclusive).
/// @param cmp A callable object defining a strict weak ordering.
template<typename FI, typename Comparison = Less>
void quickselect(FI begin, FI nth, FI end, Comparison cmp = Comparison{}) {
    if (begin == end || nth == end) return;

    while (true) {
        auto result = implicitPivotPartition(begin, end, cmp);
        auto pivot = result.pivot_;

        if (pivot == nth) {
            return; // Found the nth element
        } else if (nth < pivot) {
            // Focus on the lower partition
            end = pivot;
        } else {
            // Focus on the higher partition
            begin = std::next(pivot);
        }
    }
}

}

#endif
