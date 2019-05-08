#ifndef DESTROY_H
#define DESTROY_H

namespace zoo { namespace meta {

template<typename T>
void destroy(T &t) {
    t.~T();
}

template<typename T, int L>
void destroy(T (&arr)[L]) {
    for(int i = L; i--; ) { destroy(arr[i]); }
}

}}

#endif
