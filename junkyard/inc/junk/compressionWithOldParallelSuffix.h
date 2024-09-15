
#include <zoo/swar/SWAR.h>

namespace zoo::swar::junk {

template <int NB, typename B>
constexpr SWAR<NB, B> old_parallelSuffix(SWAR<NB, B> input) {
  using S = SWAR<NB, B>;
  auto shiftClearingMask = S{static_cast<B>(~S::MostSignificantBit)},
       doubling = input, result = S{0};
  auto bitsToXOR = NB, power = 1;

#define ZTE(...)
  // ZOO_TRACEABLE_EXPRESSION(__VA_ARGS__)
  for (;;) {
    ZTE(doubling);
    if (1 & bitsToXOR) {
      ZTE(result ^ doubling);
      result = result ^ doubling;
      ZTE(doubling.shiftIntraLaneLeft(power, shiftClearingMask));
      doubling = doubling.shiftIntraLaneLeft(power, shiftClearingMask);
    }
    ZTE(bitsToXOR >> 1);
    bitsToXOR >>= 1;
    if (!bitsToXOR) {
      break;
    }
    auto shifted = doubling.shiftIntraLaneLeft(power, shiftClearingMask);
    ZTE(shifted);
    ZTE(doubling ^ shifted);
    doubling = doubling ^ shifted;
    // 01...1
    // 001...1
    // 00001...1
    // 000000001...1
    shiftClearingMask = shiftClearingMask &
                        S{static_cast<B>(shiftClearingMask.value() >> power)};
    ZTE(power << 1);
    power <<= 1;
  }
  ZTE(input);
#undef ZTE
  return S{result};
}

#define parallelSuffix old_parallelSuffix
template <int NB, typename B>
constexpr SWAR<NB, B>
compressWithOldParallelSuffix(SWAR<NB, B> input, SWAR<NB, B> compressionMask) {
// This solution uses the parallel suffix operation as a primary tool:
// For every bit postion it indicates an odd number of ones to the right,
// including itself.
// Because we want to detect the "oddness" of groups of zeroes to the right,
// we flip the compression mask.  To not count the bit position itself,
// we shift by one.
#define ZTE(...)
  // ZOO_TRACEABLE_EXPRESSION(__VA_ARGS__)
  ZTE(input);
  ZTE(compressionMask);
  using S = SWAR<NB, B>;
  auto result = input & compressionMask;
  auto groupSize = 1;
  auto shiftLeftMask = S{S::LowerBits}, shiftRightMask = S{S::LowerBits << 1};
  ZTE(~compressionMask);
  auto forParallelSuffix = // this is called "mk" in the book
      (~compressionMask).shiftIntraLaneLeft(groupSize, shiftLeftMask);
  ZTE(forParallelSuffix);
  // note: forParallelSuffix denotes positions with a zero
  // immediately to the right in 'compressionMask'
  for (;;) {
    ZTE(groupSize);
    ZTE(shiftLeftMask);
    ZTE(shiftRightMask);
    ZTE(result);
    auto oddCountOfGroupsOfZerosToTheRight = // called "mp" in the book
        parallelSuffix(forParallelSuffix);
    ZTE(oddCountOfGroupsOfZerosToTheRight);
    // compress the bits just identified in both the result and the mask
    auto moving = compressionMask & oddCountOfGroupsOfZerosToTheRight;
    ZTE(moving);
    compressionMask = (compressionMask ^ moving) | // clear the moving
                      moving.shiftIntraLaneRight(groupSize, shiftRightMask);
    ZTE(compressionMask);
    auto movingFromInput = result & moving;
    result = (result ^ movingFromInput) | // clear the moving from the result
             movingFromInput.shiftIntraLaneRight(groupSize, shiftRightMask);
    auto nextGroupSize = groupSize << 1;
    if (NB <= nextGroupSize) {
      break;
    }
    auto evenCountOfGroupsOfZerosToTheRight =
        ~oddCountOfGroupsOfZerosToTheRight;
    forParallelSuffix = forParallelSuffix & evenCountOfGroupsOfZerosToTheRight;
    auto newShiftLeftMask =
        shiftLeftMask.shiftIntraLaneRight(groupSize, shiftRightMask);
    shiftRightMask =
        shiftRightMask.shiftIntraLaneLeft(groupSize, shiftLeftMask);
    shiftLeftMask = newShiftLeftMask;
    groupSize = nextGroupSize;
  }
  ZTE(result);
#undef ZTE
  return result;
}
#undef parallelSuffix

} // namespace zoo::swar::junk

