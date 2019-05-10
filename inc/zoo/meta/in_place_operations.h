#ifndef IN_PLACE_OPERATIONS_H
#define IN_PLACE_OPERATIONS_H

namespace zoo { namespace meta {

template<typename T>
void copy_in_place(void *place, const T &t) {
    new(place) T{t};
}

template<typename T, int L>
void copy_in_place(void *place, const T (&arr)[L]) {
    auto destination = reinterpret_cast<T *>(place);
    for(int i = 0; i < L; i++) { copy_in_place(destination + i, arr[i]); }
}

template<typename T>
void move_in_place(void *place, T &&t) {
    new(place) T{std::move(t)};
}

template<typename T, int L>
void move_in_place(void *place, T (&arr)[L]) {
    auto destination = reinterpret_cast<T *>(place);
    for(int i = 0; i < L; i++) {
        move_in_place(destination + i, std::move(arr[i]));
    }
}

template<typename T>
void destroy_in_place(T &t) {
    t.~T();
}

template<typename T, int L>
void destroy_in_place(T (&arr)[L]) {
    for(int i = L; i--; ) { destroy_in_place(arr[i]); }
}

}}

#endif
