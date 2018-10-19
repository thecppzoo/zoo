#ifndef ZOO_GENERALIZED_QUICKSORT
#define ZOO_GENERALIZED_QUICKSORT

void swap(int &l, int &r) {
    auto tmp = l;
    l = r;
    r = tmp;
}

#include <array>

template<typename I>
I partition_noRandomAccess(I pivot, I b, I e) {
    while(b != e) {
        // ..., P == *pivot, G0, G1, ..., Gn, L == *b
        if(*b < *pivot) {
            // target: ..., L, P == *pivot, G1, ...., G0 == *b
            swap(*pivot, *b);
            // ..., L == *pivot, G0, G1, ..., Gn, P == *b
            swap(*++pivot, *b);
        }
        ++b;
    }
    return pivot;
}

template<typename I>
I partition_noRandomAccessRotate(I pivot, I b, I e) {
    while(b != e) {
        if(*b < *pivot) {
            // pivot <- b, b <- (pivot + 1), (pivot + 1) <- pivot
            auto tmp = *pivot;
            *pivot = *b;
            *b = *++pivot;
            *pivot = tmp;
        }
        ++b;
    }
    return pivot;    
}

template<typename I, typename ParFun>
void quicksortNoRandomAccess_impl(I begin, I end, ParFun pf) {
    auto bPtr = &*begin;
    struct Frame {
        I b, e;
    };
    std::array<Frame, 50> frames;
    int index = 0;
    frames[0].b = begin;
    frames[0].e = end;
    for(;;) {
        auto &frame = frames[index];
        auto b = frame.b;
        auto e = frame.e;

        auto oldB = b;
        auto oldBPtr = &*oldB;
        if(b != e) {
            auto pivot = b++;
            if(b != e) {
                //pivot = partition_noRandomAccess(pivot, b, e);
                pivot = pf(pivot, b, e);
                ++index;
                if(50 <= index) { throw std::runtime_error("Exhausted"); }
                frames[index].b = oldB;
                frames[index].e = pivot;
                ++pivot;
                frame.b = pivot;
                continue;
            }
        }
        if(!index--) { break; }
    }
}


template<typename I>
void quicksortNoRandomAccess(I begin, I end) {
    quicksortNoRandomAccess_impl(begin, end, partition_noRandomAccess<I>);
}

template<typename I>
void quicksort(I b, I e) {
    auto oldB = b;
    if(b == e) { return; }
    auto pivot = b++;
    if(b == e) { return; }
    pivot = partition_noRandomAccess(pivot, b, e);
    quicksort(oldB, pivot);
    ++pivot;
    quicksort(pivot, e);
}

#endif
