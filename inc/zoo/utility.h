#ifndef ZOO_UTIL
#define ZOO_UTIL

#include <utility>

namespace zoo {

template<typename T>
inline
void swap(T &t1, T &t2) noexcept(noexcept(std::swap(t1, t2))) {
    std::swap(t1, t2);
}

template<typename T>
inline
void swap_other_name(T &t1, T &t2) noexcept(noexcept(std::swap(t1, t2))) {
    swap(t1, t2);
}

}

#endif
