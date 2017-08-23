#include "util/align.h"

template<typename T>
inline void work(T *ptr, T val) {
    for(int i = 32; i--; ) { ptr[i] *= val; }
}

void accumulator(float *fp, float v) {
    auto aligned = __builtin_assume_aligned(fp, 32);
    auto p = reinterpret_cast<float *>(aligned);
    work(p, v);
}

void accumulator(zoo::assume_aligned<float, 32> aap, float v) {
    work(aap.pointer(), v);
}
