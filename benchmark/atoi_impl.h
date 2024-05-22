#include "zoo/swar/SWAR.h"

#include <tuple>
#include <string.h>

uint64_t calculateBase10(
    zoo::swar::SWAR<8, __uint128_t> convertedToIntegers
) noexcept;

uint32_t calculateBase10(
    zoo::swar::SWAR<8, uint64_t> convertedToIntegers
) noexcept;

namespace zoo {

/// @brief Loads the "block" containing the pointer, by proper alignment
/// @tparam PtrT Pointer type for loading
/// @tparam Block as the name indicates
/// @param pointerInsideBlock the potentially misaligned pointer
/// @param b where the loaded bytes will be put
/// @return a pair to indicate the aligned pointer to the base of the block
/// and the misalignment, in bytes, of the source pointer
template<typename PtrT, typename Block>
std::tuple<PtrT *, int>
blockAlignedLoad(PtrT *pointerInsideBlock, Block *b) {
    uintptr_t asUint = reinterpret_cast<uintptr_t>(pointerInsideBlock);
    constexpr auto Alignment = alignof(Block), Size = sizeof(Block);
    static_assert(Alignment == Size);
    auto misalignment = asUint % Alignment;
    auto *base = reinterpret_cast<PtrT *>(asUint - misalignment);
    memcpy(b, base, Size);
    return { base, misalignment };
}

}
