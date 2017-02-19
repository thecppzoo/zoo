#pragma once

#include <stdint.h>

namespace ep { namespace core {

constexpr int metaLogFloor(uint64_t arg) {
    return 63 - __builtin_clzll(arg);
}

constexpr int metaLogCeiling(uint64_t arg) {
    auto floorLog = metaLogFloor(arg);
    return floorLog + ((arg ^ (1ull << floorLog)) ? 1 : 0);
}
    
}}
