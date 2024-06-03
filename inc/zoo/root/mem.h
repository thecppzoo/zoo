#ifndef ZOO_ROOT_MEM_H
#define ZOO_ROOT_MEM_H

/// \file zoo/root/mem.h Memory utilities

// used for multiple return values in blockAlignedLoad
#include <tuple>
// memcpy
#include <string.h>
// for uintptr_t, there are differences between C and C++
#include <cstdlib>

namespace zoo {

/// @brief Loads the "block" containing the pointer, by proper alignment
/// @tparam PtrT Pointer type for loading
/// @tparam Block as the name indicates
/// @param pointerInsideBlock the potentially misaligned pointer
/// @param b where the loaded bytes will be put
/// @return a pair to indicate the aligned pointer to the base of the block
/// and the misalignment, in bytes, of the source pointer
/// \note The misalignment is in the range [ 0, sizeof(Block) [
template<typename PtrT, typename Block>
std::tuple<PtrT *, int>
blockAlignedLoad(PtrT *pointerInsideBlock, Block *b) {
    // conversion (casting) to use modulo arithmetic to detect misalignment
    // Surprisingly, intptr_t is optional, let alone uintptr_t
    using UPtr = std::make_unsigned_t<std::intptr_t>;
    auto asUint = reinterpret_cast<UPtr>(pointerInsideBlock);
    constexpr auto Alignment = alignof(Block), Size = sizeof(Block);
    // It is preferable to have this hard error than to SFINAE it away
    static_assert(Alignment == Size);
    auto misalignment = asUint % Alignment;
    auto *base = reinterpret_cast<PtrT *>(asUint - misalignment);
    memcpy(b, base, Size);
    return { base, misalignment };
}

}

#endif
