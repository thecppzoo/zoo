#ifndef ZOO_UTIL
#define ZOO_UTIL

#include <utility>

#include <algorithm>
    // for swap

namespace zoo {

// makes std::swap available for types in namespace zoo
using std::swap;

template<typename T>
inline
void swap_other_name(T &t1, T &t2) noexcept(noexcept(std::swap(t1, t2))) {
    swap(t1, t2);
}

}

#endif
