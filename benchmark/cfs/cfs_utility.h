#ifndef ZOO_BENCHMARK_CFS_UTILITY
#define ZOO_BENCHMARK_CFS_UTILITY

#include <vector>

std::vector<int> linear_vector(int n);

int randomTwo30();

std::vector<int> makeRandomVector(int size);

template<typename C, typename V>
auto xorChecksum(const C &c, V v) {
    for(auto &e: c) {
        v ^= e;
    }
    return v;
}

#endif
