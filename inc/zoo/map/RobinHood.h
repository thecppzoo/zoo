#ifndef ZOO_ROBINHOOD_H
#define ZOO_ROBINHOOD_H

#include "zoo/swar/SWAR.h"
#include "zoo/AlignedStorage.h"

#ifndef ZOO_CONFIG_DEEP_ASSERTIONS
    #define ZOO_CONFIG_DEEP_ASSERTIONS 0
#endif

#include <tuple>
#include <array>
#include <functional>

#if ZOO_CONFIG_DEEP_ASSERTIONS
    #include <assert>
#endif

namespace zoo {

template<typename T>
struct GeneratorFromPointer {
    T *p_;

    constexpr T &operator*() noexcept { return *p_; }
    constexpr GeneratorFromPointer operator++() noexcept { return {++p_}; }
};

template<typename T, int MisalignmentBits>
struct MisalignedGenerator {
    T *base_;
    constexpr static auto Width = sizeof(T) * 8;

    constexpr T operator*() noexcept {
        auto firstPart = base_[0];
        auto secondPart = base_[1];
            // how to make sure the "logical" shift right is used, regardless
            // of the signedness of T?
            // I'd prefer to not use std::make_unsigned_t, since how do we
            // "make unsigned" user types?
        auto firstPartLowered = firstPart.value() >> MisalignmentBits;
        auto secondPartRaised = secondPart.value() << (Width - MisalignmentBits);
        return T{firstPartLowered | secondPartRaised};
    }

    constexpr MisalignedGenerator operator++() noexcept { return {++base_}; }
};

template<typename T>
struct MisalignedGenerator<T, 0>: GeneratorFromPointer<T> {};

template<typename T>
struct MisalignedGenerator_Dynamic {
    constexpr static auto Width = sizeof(T) * 8;
    T *base_;

    int misalignmentFirst, misalignmentSecond;

    MisalignedGenerator_Dynamic(T *base, int ma):
        base_(base),
        misalignmentFirst(ma), misalignmentSecond(Width - ma)
    {}
    

    constexpr T operator*() noexcept {
        auto firstPart = base_[0];
        auto secondPart = base_[1];
            // how to make sure the "logical" shift right is used, regardless
            // of the signedness of T?
            // I'd prefer to not use std::make_unsigned_t, since how do we
            // "make unsigned" user types?
        auto firstPartLowered = firstPart.value() >> misalignmentFirst;
        auto secondPartRaised = secondPart.value() << misalignmentSecond;
        return T{firstPartLowered | secondPartRaised};
    }

    constexpr MisalignedGenerator_Dynamic operator++() noexcept {
        ++base_; return *this;
    }
};

namespace rh {

namespace impl {

/// \todo decide on whether to rename this?
template<int PSL_Bits, int HashBits, typename U = std::uint64_t>
struct Metadata: swar::SWARWithSubLanes<HashBits, PSL_Bits, U> {
    using Base = swar::SWARWithSubLanes<HashBits, PSL_Bits, U>;
    using Base::Base;

    constexpr auto PSLs() const noexcept { return this->least(); }
    constexpr auto Hashes() const noexcept { return this->most(); }
};

template<int PSL_Bits, int HashBits, typename U = std::uint64_t>
struct MatchResult {
    U deadline;
    Metadata<PSL_Bits, HashBits, U> potentialMatches;
};

}

template<int PSL_Bits, int HashBits, typename U = std::uint64_t>
struct RH_Backend {
    using Metadata = impl::Metadata<PSL_Bits, HashBits, U>;

    constexpr static inline auto Width = Metadata::NBits;
    Metadata *md_;

    /*! \brief SWAR check for a potential match
    The invariant in Robin Hood is that the element being looked for, the "needle", is "richer"
    than the elements already present, the "haystack".
    "Richer" means that the PSL is smaller.
    A PSL of 0 can only happen in the haystack, to indicate the slot is empty, this is "richest".
    The first time the needle has a PSL greater than the haystacks' means the matching will fail,
    because the hypothetical prior insertion would have "stolen" that slot.
    If there is an equal, it would start a sequence of potential matches.  To determine an actual match:
    1. A cheap SWAR check of hoisted hashes
    2. If there are still potential matches (now also the hoisted hashes), fall back to non-SWAR,
    or iterative and expensive "deep equality" test for each potential match, outside of this function
    
    The above makes it very important to detect the first case in which the PSL is greater equal to the needle.
    We call this the "deadline".
    Because we assume the LITTLE ENDIAN byte ordering, the first element would be the least significant
    non-false Boolean SWAR.
    
    Note about performance:
    Every "early exit" faces a big justification hurdle, the proportion of cases
    they intercept to be large enough that the branch prediction penalty of the entropy introduced is
    overcompensated.
    */
    constexpr static impl::MatchResult<PSL_Bits, HashBits>
    potentialMatches(
        Metadata needle, Metadata haystack
    ) noexcept {
        // We need to determine if there are potential matches to consider
        auto sames = equals(needle, haystack);

        auto needlePSLs = needle.PSLs();
        auto haystackPSLs = haystack.PSLs();
        auto haystackStrictlyRichers =
            // !(needlePSLs >= haystackPSLs) <=> haystackPSLs < needlePSLs
            not greaterEqual_MSB_off(needlePSLs, haystackPSLs);
            // BTW, the reason to have encoded the PSLs in the least
            // significant bits is to be able to call the cheaper version
            // _MSB_off here

        if(!haystackStrictlyRichers) {
            // The search could continue, but there could be potential matches
            // from this SWAR
            return { 0, sames };
        }
        // Performance wise, this test is profitable because the search
        // has reached finality:
        // There is a haystack element richer than the potential PSL of the
        // needle, if the needle is in the table, it would have been
        // inserted before that haystack element.
        // If the needle is not in the table, that element, that we call
        // "deadline" would be the slot to insert it.

        // I (Eddie) believe an early exit of not having "sames" is not
        // profitable here
        // filter the needle, haystack equals by being before the deadline
        // (by the way, the deadline means that slot can't be equal)
        auto haystackRichersAsNumber = haystackStrictlyRichers.value();
        // because we make the assumption of LITTLE ENDIAN byte ordering,
        // we're interested in the elements up to the first haystack-richer
        auto deadline = swar::isolateLSB(haystackRichersAsNumber);
        // to analize before the deadline, "maskify" it.  Remember, the
        // deadline element itself can't be a potential match, it does
        // not need preservation
        auto deadlineMaskified = Metadata{deadline - 1};
        auto beforeDeadlineMatches = sames & deadlineMaskified;
        return {
            deadline,
            beforeDeadlineMatches
        };
    }

    /*! \brief converts the given starting PSL and reduced hash code into a SWAR-ready needle
    
    The given needle would have a PSL as the starting (PSL + 1) in the first slot, the "+ 1" is because
    the count starts at 1, in this way, a haystack PSL of 0 is always "richer"
    */
    constexpr static auto makeNeedle(U startingPSL, U hoistedHash) {
        constexpr auto Ones = meta::BitmaskMaker<U, 1, Width>::value;
        constexpr auto Progression = Ones * Ones;
        auto core = startingPSL | (hoistedHash << PSL_Bits);
        auto broadcasted = broadcast(Metadata(core));
        auto startingPSLmadePotentialPSLs = Metadata(Progression) + broadcasted;
        return startingPSLmadePotentialPSLs;
    }

    template<int Misalignment, typename KeyComparer>
    constexpr static auto
    unalignedFind(
        U hoistedHash,
        Metadata *base,
        int homeIndex,
        const KeyComparer &kc
    ) noexcept {
        constexpr auto Ones = meta::BitmaskMaker<U, 1, Width>::value;
        constexpr auto Progression = Metadata{Ones * Ones};

        MisalignedGenerator<Metadata, 8*Misalignment> p{base};
        auto index = homeIndex;
        auto needle = makeNeedle(0, hoistedHash);
        for(;;) {
            auto result = potentialMatches(needle, *p);
            auto positives = result.potentialMatches;
            auto deadline = result.deadline;
            while(positives.value()) {
                auto matchSubIndex = positives.lsbIndex();
                auto matchIndex = index + matchSubIndex;
                if(kc(matchIndex)) {
                    return std::tuple(true, matchIndex);
                }
                positives = Metadata{clearLSB(positives.value())};
            }
            if(deadline) {
                auto position = index + Metadata{deadline}.lsbIndex();
                return std::tuple(false, position);
            }
            // Skarupke's tail allows us to not have to worry about the end
            // of the metadata
            ++p;
            index += Metadata::NSlots;
            needle = needle + Progression;
        }
    }
        
    template<typename KeyComparer>
    constexpr auto
    findThroughIndirectJump(
        U hh, int index, const KeyComparer &kc
    ) const noexcept {
        auto misalignment = index % Metadata::NSlots;
        auto baseIndex = index / Metadata::NSlots;
        auto base = this->md_ + baseIndex;
        switch(misalignment) {
            case 0: return unalignedFind<0>(hh, base, index, kc);
            case 1: return unalignedFind<1>(hh, base, index, kc);
            case 2: return unalignedFind<2>(hh, base, index, kc);
            case 3: return unalignedFind<3>(hh, base, index, kc);
            case 4: return unalignedFind<4>(hh, base, index, kc);
            case 5: return unalignedFind<5>(hh, base, index, kc);
            case 6: return unalignedFind<6>(hh, base, index, kc);
            case 7: return unalignedFind<7>(hh, base, index, kc);
        }
        __builtin_unreachable();
    }

    template<typename KeyComparer>
    constexpr auto
    findMisaligned_assumesSkarupkeTail(
        U hoistedHash, int homeIndex, const KeyComparer &kc
    ) const noexcept {
        auto misalignment = homeIndex % Metadata::NSlots;
        auto baseIndex = homeIndex / Metadata::NSlots;
        auto base = this->md_ + baseIndex;

        constexpr auto Ones = meta::BitmaskMaker<U, 1, Width>::value;
        constexpr auto Progression = Metadata{Ones * Ones};
        MisalignedGenerator_Dynamic<Metadata> p(base, int(8*misalignment));
        auto index = homeIndex;
        auto needle = makeNeedle(0, hoistedHash);
        for(;;) {
            auto result = potentialMatches(needle, *p);
            auto positives = result.potentialMatches;
            auto deadline = result.deadline;
            while(positives.value()) {
                auto matchSubIndex = positives.lsbIndex();
                auto matchIndex = index + matchSubIndex;
                if(kc(matchIndex)) {
                    return std::tuple(true, matchIndex);
                }
                positives = Metadata{swar::clearLSB(positives.value())};
            }
            if(deadline) {
                auto position = index + Metadata{deadline}.lsbIndex();
                return std::tuple(false, position);
            }
            // Skarupke's tail allows us to not have to worry about the end
            // of the metadata
            ++p;
            index += Metadata::NSlots;
            needle = needle + Progression;
        }
    }
};

template<int NBits, typename U>
constexpr auto hashReducer(U n) noexcept {
    constexpr auto Shift = (NBits * ((sizeof(U)*8 / NBits) - 1));
    constexpr auto AllOnes = meta::BitmaskMaker<U, 1, NBits>::value;
    auto temporary = AllOnes * n;
    auto higestNBits = temporary >> Shift;
    return
        (0 == (64 % NBits)) ?
            higestNBits :
            swar::isolate<NBits>(higestNBits);
}


template<typename T>
constexpr auto fibonacciIndexModulo(T index) {
    constexpr std::array<uint64_t, 4>
        GoldenRatioReciprocals = {
            159,
            40503,
            2654435769,
            11400714819323198485ull,
        };
    constexpr T MagicalConstant =
        T(GoldenRatioReciprocals[meta::logFloor(sizeof(T)) - 3]);
    return index * MagicalConstant;
}

template<size_t Size, typename T>
constexpr auto lemireModuloReductionAlternative(T input) noexcept {
    static_assert(sizeof(T) == sizeof(uint64_t));
    static_assert(Size < (1ull << 32));
    auto halved = uint32_t(input);
    return Size * input >> 32;
}

template<
    typename K,
    typename MV,
    size_t RequestedSize,
    int PSL_Bits, int HashBits,
    typename Hash = std::hash<K>,
    typename KE = std::equal_to<K>,
    typename U = std::uint64_t
>
struct RH_Frontend_WithSkarupkeTail {
    using Backend = RH_Backend<PSL_Bits, HashBits, U>;
    using MD = typename Backend::Metadata;

    constexpr static inline auto WithTail =
        RequestedSize +
        (1 << PSL_Bits) - 1 // the Skarupke tail
        // also note it is desirable to have an odd number of elements
    ;
    constexpr static inline auto SWARCount =
        (
            WithTail +
            MD::NSlots - 1 // to calculate the ceiling rounding
        ) / MD::NSlots
    ;
    constexpr static inline auto SlotCount = SWARCount * MD::NSlots;

    std::array<MD, SWARCount> md_;
    using value_type = std::pair<K, MV>;

    /// \todo Scatter key and value in a flavor
    std::array<zoo::AlignedStorageFor<value_type>, SlotCount> values_;
    size_t elementCount_;

    RH_Frontend_WithSkarupkeTail() noexcept: elementCount_(0) {
        for(auto &mde: md_) { mde = MD{0}; }
    }

    ~RH_Frontend_WithSkarupkeTail() {
        #if ZOO_CONFIG_DEEP_ASSERTIONS
            size_t destroyedCount = 0;
        #endif
        for(size_t ndx = SlotCount; ndx--; ) {
            auto PSLs = md_[ndx].PSLs();
            auto occupied = booleans(PSLs);
            if(!occupied) { continue; }
            auto baseIndex = ndx * MD::NSlots;
            while(occupied) {
                auto subIndex = occupied.lsbIndex();
                auto index = baseIndex + subIndex;
                values_[index].template destroy<value_type>();
                #if ZOO_CONFIG_DEEP_ASSERTIONS
                    ++destroyedCount;
                #endif
            }
        }
        #if ZOO_CONFIG_DEEP_ASSERTIONS
            assert(destroyedCount == elementCount_);
        #endif
    }

    auto find(const K &k) const noexcept {
        auto keyChecker =
            [thy = this, &k](size_t ndx) noexcept {
                return KE{}(thy->values_[ndx].template as<value_type>()->first, k);
            };
        auto hashCode = Hash{}(k);
        auto fibonacciScrambled = fibonacciIndexModulo(hashCode);
        auto homeIndex =
            lemireModuloReductionAlternative<RequestedSize>(fibonacciScrambled);
        auto hoisted = hashReducer<HashBits>(hashCode);
        auto thy = const_cast<RH_Frontend_WithSkarupkeTail *>(this);
        Backend be{thy->md_.data()};
        auto [found, index] =
            be.findMisaligned_assumesSkarupkeTail(
                hoisted, homeIndex, keyChecker
            );
        return found ? values_.data() + index : values_.end();
    }
};

} // rh

} // swar, zoo

#endif
