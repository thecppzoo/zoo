#ifndef ZOO_HEADER_META_BITMASKMAKER_H
#define ZOO_HEADER_META_BITMASKMAKER_H

#include <cstdint>

namespace zoo { namespace meta {

namespace detail {

/// Repeats the given pattern in the whole of the argument
/// \tparam T the desired integral type
/// \tparam Progression how big the pattern is
/// \tparam Remaining how many more times to copy the pattern
template<typename T, T Current, int CurrentSize, bool>
struct BitmaskMaker_impl{
    constexpr static T value =
        BitmaskMaker_impl<
            T,
            T(Current | (Current << CurrentSize)),
            CurrentSize*2,
            CurrentSize*2 < sizeof(T)*8
        >::value;
};
template<typename T, T Current, int CurrentSize>
struct BitmaskMaker_impl<T, Current, CurrentSize, false> {
    constexpr static T value = Current;
};

} // detail

/// Repeats the given pattern in the whole of the argument
/// \tparam T the desired integral type
/// \tparam Current the pattern to broadcast
/// \tparam CurrentSize the number of bits in the pattern
template<typename T, T Current, int CurrentSize>
struct BitmaskMaker {
    constexpr static auto value =
        detail::BitmaskMaker_impl<
            T, Current, CurrentSize,  CurrentSize*2 <= sizeof(T)*8
        >::value;
};

/// Provides TopBlit to clear any garbage bits at the top of a bitmask made by
//BitmaskMaker
template<typename T, int CurrentSize>
struct BitmaskMakerClearTop {
    constexpr static T BitCount = sizeof(T)*8;
    constexpr static T LeastBitKeepCount = BitCount - BitCount % CurrentSize;
    constexpr static T BitMod = BitCount % CurrentSize;
    constexpr static T TopBlit = (BitMod == 0) ? ~(T(0)) :
        ((T(1) << LeastBitKeepCount) -1);
};

static_assert(0xFF == BitmaskMaker<uint8_t, 0x7, 3>::value);
static_assert(0xF0F0 == BitmaskMaker<uint16_t, 0xF0, 8>::value);
static_assert(0xEDFE'DFED == BitmaskMaker<uint32_t, 0xFED, 12>::value);

}} // zoo::meta

#endif
