#pragma once

#include <stdint.h>

namespace ep { namespace core {

inline uint64_t deposit(uint64_t preselected, uint64_t extra) {
    auto depositSelector = ~preselected;
    auto deposited = __builtin_ia32_pdep_di(extra, depositSelector);
    return deposited;
}

}}
