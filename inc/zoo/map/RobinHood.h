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

        constexpr auto PSLs() const noexcept { return this->least(); }
    };

    struct PointerAsProvider {
        Metadata *p_;

        constexpr Metadata &operator*() noexcept { return *p_; }
        constexpr PointerAsProvider operator++() noexcept { return {++p_}; }
    };

    constexpr static inline auto Width = Metadata::NBits;
    Metadata *md_;

    struct MatchResult {
        bool finality;
        Metadata sames;
    };

    // The invariant in Robin Hood is that the element being looked for is "richer"
    // than the present elements.
    // Richer: the PSL of the needle is smaller than the PSL in the haystack
    template<typename Provider>
    constexpr static auto potentialMatch(
        Metadata needle, Provider haystackProvider
    ) noexcept {
        auto haystack = *haystackProvider;
        auto needlePSLs = needle.least();
        auto haystackPSLs = haystack.least();
        auto deadlineV = greaterEqual_MSB_off(haystackPSLs, needlePSLs);
        auto deadline = deadlineV.value();
        if(deadline) {
            auto maskifyDeadline = (deadline << 1) - 1;
            auto potentialMatchesMask =
                Metadata(maskifyDeadline >> Metadata::NBits);
            auto restrictedNeedle = needle & potentialMatchesMask;
            auto restrictedHaystack = haystack & potentialMatchesMask;
            auto sames = equals(restrictedNeedle, restrictedHaystack);
            return MatchResult{deadline, sames};
        }
    }

    /*constexpr static auto makePotentialPSLNeedle(U startingPSL) {
        constexpr auto Ones = meta::BitmaskMaker<U, 1, Width>::value;
        constexpr auto  Progression = Ones * Ones;
        auto*/
    constexpr static auto makeNeedle(U startingPSL, U hoistedHash) {
        constexpr auto Ones = meta::BitmaskMaker<U, 1, Width>::value;
        constexpr auto Progression = Ones * Ones;
        auto core = startingPSL | (hoistedHash << PSL_Bits);
        auto broadcasted = broadcast(Metadata(core));
        auto withProgression = Metadata(Progression) + broadcasted;
        return withProgression;
    }

    template<typename Provider>
    constexpr static auto startMatch(U PSL, U hoistedHash, Provider provider) {
        auto needle = makeNeedle(PSL, hoistedHash);
        return potentialMatch(needle, provider);
    }
};

} // rh

}} // swar, zoo

#endif
