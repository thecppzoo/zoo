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

    Metadata *md_;

    // The invariant in Robin Hood is that the element being looked for is "richer"
    // than the present elements.
    // Richer: the PSL of the needle is smaller than the PSL in the haystack
    static auto potentialMatch(Metadata needle, Metadata *haystack) noexcept {
        
    }

    static auto makeNeedle(U startingPsl, U hoistedHash) {
        //constexpr static auto Ones = meta::BitmaskMaker<U, 1 << , <#int CurrentSize#>>
    }
};

} // rh

}} // swar, zoo

#endif
