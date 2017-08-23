#pragma once

namespace clangisbetter {

template<typename T>
T *reiterate_alignment(T *argument) {
    return
        reinterpret_cast<T *>(
            __builtin_assume_aligned(argument, alignof(T))
        );
}

}
namespace zoo {

template<typename T, int Alignment>
struct assume_aligned {
    struct alignas(Alignment) Aligned { char member[Alignment]; };
    assume_aligned(T *ptr): m_ptr(reinterpret_cast<const Aligned *>(ptr)) {}
    T *pointer() const noexcept {
        return const_cast<T *>(
            #ifdef __CLANG__
                m_ptr
            #else
                reinterpret_cast<const T *>(
                    clangisbetter::reiterate_alignment(m_ptr)
                )
            #endif
        );
    }

protected:
    const Aligned *m_ptr;
};

}
