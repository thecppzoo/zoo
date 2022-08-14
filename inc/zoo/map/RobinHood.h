#ifndef ZOO_ROBINHOOD_H
#define ZOO_ROBINHOOD_H

#include "zoo/swar/SWAR.h"

namespace zoo { namespace swar {

namespace rh {

template<int PSL_Bits, int HashBits, typename U = std::uint64_t>
struct RH {
    struct Metadata: SWARWithSubLanes<HashBits, PSL_Bits, U> {
        using Base = SWARWithSubLanes<HashBits, PSL_Bits, U>;
        using Base::Base;
    };

    constexpr static inline auto Width = Metadata::NBits;
    Metadata *md_;

    // The invariant in Robin Hood is that the element being looked for is "richer"
    // than the present elements.
    // Richer: the PSL of the needle is smaller than the PSL in the haystack
    constexpr static auto potentialMatch(Metadata needle, Metadata *haystack) noexcept {
        
    }

    constexpr static auto makeNeedle(U startingPSL, U hoistedHash) {
        constexpr auto Ones = meta::BitmaskMaker<U, 1, Width>::value;
        constexpr auto Progression = Ones * Ones;
        auto core = startingPSL | (hoistedHash << PSL_Bits);
        auto broadcasted = broadcast(Metadata(core));
        auto withProgression = Metadata(Progression) + broadcasted;
        return withProgression;
    }
};

} // rh

}} // swar, zoo

#endif
