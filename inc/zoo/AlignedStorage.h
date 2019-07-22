//
//  AlignedStorage.h
//
//  Created by Eduardo Madrid on 7/2/19.
//

#ifndef ZOO_AlignedStorage_h
#define ZOO_AlignedStorage_h

#include <new>
#include <type_traits>

namespace zoo {

constexpr auto VPSize = sizeof(void *), VPAlignment = alignof(void *);

///! \note What about constructors and destructor?
template<int S = VPSize, int A = VPAlignment>
struct AlignedStorage {
    constexpr static auto Size = S, Alignment = A;

    alignas(A) char space_[Size];

    template<typename T>
    T *as() noexcept { return reinterpret_cast<T *>(&space_); }
    template<typename T>
    const T *as() const noexcept {
        return const_cast<AlignedStorage *>(this)->as<T>();
    }

    template<typename T>
    static constexpr auto SuitableType() {
        return sizeof(T) <= S && 0 == A % alignof(T);
    }

    template<typename T, typename... Args>
    #define PP_ZOO_BUILD_EXPRESSION new(space_) T(std::forward<Args>(args)...)
    auto build(Args &&...args) noexcept(noexcept(PP_ZOO_BUILD_EXPRESSION)) ->
        std::enable_if_t<SuitableType<T>(), decltype(PP_ZOO_BUILD_EXPRESSION)>
    {
        return PP_ZOO_BUILD_EXPRESSION;
    #undef PP_ZOO_BUILD_EXPRESSION
    }

    template<typename T>
    auto destroy() noexcept -> std::enable_if_t<SuitableType<T>()>
    { as<T>()->~T(); }
};

}

#endif /* AlignedStorage_h */
