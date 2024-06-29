#include "zoo/swar/SWAR.h"

#include <cstdint>
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
/// \note The misalignment is in the range [ 0, sizeof(Block) [
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

template<typename S, typename S::type Pattern>
constexpr
S adjustMisalignmentWithPattern(S data, int misalignment) {
    constexpr typename S::type Zero{0};
    constexpr auto AllOnes = ~Zero;

    auto keepMask = (AllOnes) << (misalignment * 8); // assumes 8 bits per char
    auto keep = data & S{keepMask};
    auto rest = Pattern & ~keepMask;

    return keep | S{rest};
}

/// \brief Helper function to fix the non-string part of block
template<typename S>
constexpr
S adjustMisalignmentFor_strlen(S data, int misalignment) {
    // The speculative load has the valid data in the higher lanes.
    // To use the same algorithm as the rest of the implementation, simply
    // populate with ones the lower part, in that way there won't be nulls.
    constexpr typename S::type Zero{0};
    constexpr auto AllOnes = ~Zero;
    return adjustMisalignmentWithPattern<S, AllOnes>(data, misalignment);
}

/// \brief Helper function to fix the non-string part of block
template<typename S>
constexpr
S adjustMisalignmentFor_strcmp(S data, int misalignment) {
    constexpr typename S::type Zero{0};
    return adjustMisalignmentWithPattern<S, Zero>(data, misalignment);
}

using S = zoo::swar::SWAR<8, uint64_t>;
static_assert(adjustMisalignmentWithPattern<S, ~typename S::type{0}>(S{0x123456789}, 1).value() == 0x1234567FF);

static_assert(adjustMisalignmentWithPattern<S, typename S::type{0}>(S{0x123456789}, 2).value() == 0x123450000);

}

