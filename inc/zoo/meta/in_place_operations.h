#ifndef IN_PLACE_OPERATIONS_H
#define IN_PLACE_OPERATIONS_H

namespace zoo { namespace meta {

template<typename T>
void default_in_place(T *where) {
    new(where) T;
}

template<typename T>
void default_in_place(std::size_t &, T *arr);

template<typename T, std::size_t L>
void default_in_place(T (&arr)[L]) {
    auto n = L;
    default_in_place(n, &arr[0]);
}

template<typename T>
void default_in_place(std::size_t &count, T *arr) {
    auto n = count;
    auto
        base = arr,
        destination = base;
    try {
        for(auto i = n; i--; ++destination) { default_in_place(destination); }
    } catch(...) {
        count = destination - base;
        throw;
    }
}

template<typename T>
void copy_in_place(void *place, const T &t) {
    new(place) T{t};
}

template<typename T>
void copy_in_place(std::size_t &, void *, const T *);

template<typename T, std::size_t L>
void copy_in_place(void *place, const T (&arr)[L]) {
    auto n = L;
    copy_in_place(n, place, &arr[0]);
}

template<typename T>
void copy_in_place(std::size_t &count, void *place, const T *arr) {
    auto n = count;
    auto
        base = static_cast<T *>(place),
        destination = base;
    try {
        for(std::size_t i = 0; i < n; i++) {
            copy_in_place(destination, arr[i]);
            ++destination;
        }
    } catch(...) {
        count = destination - base;
        throw;
    }
}

template<typename T>
void move_in_place(void *place, T &&t) {
    new(place) T{std::move(t)};
}

template<typename T>
void move_in_place(void *, T *, std::size_t) noexcept;

template<typename T, std::size_t L>
void move_in_place(void *place, T (&arr)[L]) noexcept {
    move_in_place(place, &arr[0], L);
}

template<typename T>
void move_in_place(void *place, T *arr, std::size_t n) noexcept {
    auto destination = static_cast<T *>(place);
    for(std::size_t i = 0; i < n; i++) {
        move_in_place(destination + i, std::move(arr[i]));
    }
}

template<typename T>
void destroy_in_place(T &t) noexcept {
    t.~T();
}

template<typename T>
void destroy_in_place(T *, std::size_t) noexcept;

template<typename T, std::size_t L>
void destroy_in_place(T (&arr)[L]) {
    destroy_in_place(&arr[0], L);
}

template<typename T>
void destroy_in_place(T *arr, std::size_t n) noexcept {
    for(auto i = n; i--; ) { destroy_in_place(arr[i]); }
}

}}

#endif
