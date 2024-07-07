#include "zoo/swar/SWAR.h"

/// \brief Best implementation of Lemire's technique
///
/// 128 integers are currently an emulation that uses 64 computation
/// together with arquitectural instructions such as
/// 1. Add with carry
/// 2. Shifts widened to two general purpose register source/destination operands
/// 3. Multiplication widened.
/// In general, 128 bit processing would have significant penalties.
/// This function reflects our best understanding of how to mix 64 and 128 bit processing
uint64_t calculateBase10(
    zoo::swar::SWAR<8, __uint128_t> convertedToIntegers
) noexcept;

/// \brief Lemire's technique extended to 16 bytes, straightforward use of 128 bit types provided by
/// the compiler
uint64_t calculateBase10_128(
    zoo::swar::SWAR<8, __uint128_t> convertedToIntegers
) noexcept;

uint32_t calculateBase10(
    zoo::swar::SWAR<8, uint64_t> convertedToIntegers
) noexcept;
