#ifndef ZOO_HEADER_META_LOG_H
#define ZOO_HEADER_META_LOG_H

#include <zoo/meta/popcount.h>

namespace zoo { namespace meta {

constexpr int logFloor(uint64_t arg) {
    return 63 - __builtin_clzll(arg);
}

constexpr int logCeiling(uint64_t arg) {
    auto floorLog = logFloor(arg);
    return floorLog + ((arg ^ (1ull << floorLog)) ? 1 : 0);
}

/// The algorithm is, from the perspective of the most significant bit set, to copy it
/// downward to all positions.
/// First copy it once, meaning a group of two copies of the two most significant bit
/// Then copy it again, making a group of four copies, then 8 copies...
template<typename T>
constexpr int logCeiling_WithoutIntrinsic(T value) {
    constexpr auto NBitsTotal = sizeof(T) * 8;
    for(auto groupSize = 1; groupSize < NBitsTotal; groupSize <<= 1) {
        value |= (value >> groupSize);
    }
    return PopcountLogic<detail::BitWidthLog<T>, T>::execute(value) - 1;
}
    
}}

#endif
