#pragma once

#include <chrono>
#include <utility>

namespace ep { namespace timing {

template<typename Callable, typename... Args>
inline long benchmark(Callable &&call, Args &&... arguments) {
    auto now = std::chrono::high_resolution_clock::now();
    call(std::forward<Args>(arguments)...);
    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - now);
    return diff.count();
}

template<typename Callable, typename... Args>
inline long countedBenchmark(Callable &&call, long count, Args &&... arguments) {
    auto now = std::chrono::high_resolution_clock::now();
    while(count--) {
        call(std::forward<Args>(arguments)...);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - now);
    return diff.count();
}
    
}}
