#ifndef ZOO_ROBINHOOD_H
#define ZOO_ROBINHOOD_H

#include "zoo/map/RobinHoodUtil.h"
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

namespace rh {

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

        // We could branch on 'all haystack slots are poorer than needle', but
        // branches are expensive and nondeterministic, so we think that simply
        // doing the computation is better, as all other instructions here are
        // very simple.
        //if(!haystackStrictlyRichers) { return { 0, sames }; }

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
        // to analyze before the deadline, "maskify" it.  Remember, the
        // deadline element itself can't be a potential match, it does
        // not need preservation.
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
        constexpr auto AllNSlots =
            Metadata{meta::BitmaskMaker<U, Metadata::NSlots, Width>::value};
        MisalignedGenerator_Dynamic<Metadata> p(base, int(8*misalignment));
        auto index = homeIndex;
        auto needle = makeNeedle(0, hoistedHash);
        for(;;) {
            auto result = potentialMatches(needle, *p);
            auto positives = result.potentialMatches;
            while(positives.value()) {
                auto matchSubIndex = positives.lsbIndex();
                auto matchIndex = index + matchSubIndex;
                // Possible specialist optimization to kick off all possible
                // matches to an array (like chaining evict) and check them
                // later.
                if(kc(matchIndex)) {
                    return std::tuple(matchIndex, U(0), Metadata(0));
                }
                positives = Metadata{swar::clearLSB(positives.value())};
            }
            auto deadline = result.deadline;
            if(deadline) {
                auto position = index + Metadata{deadline}.lsbIndex();
                return std::tuple(position, deadline, Metadata(needle));
            }
            // Skarupke's tail allows us to not have to worry about the end
            // of the metadata
            ++p;
            index += Metadata::NSlots;
            needle = needle + AllNSlots;
            // TODO psl overflow must be checked.
        }
    }

};

template<typename K, typename MV>
struct KeyValuePairWrapper {
    using type = std::pair<K, MV>;
    AlignedStorageFor<type> pair_;

    template<typename... Initializers>
    void build(Initializers &&...izers)
        noexcept(noexcept(pair_.template build<type>(std::forward<Initializers>(izers)...)))
    {
        pair_.template build<type>(std::forward<Initializers>(izers)...);
    }

    template<typename RHS>
    KeyValuePairWrapper &operator=(RHS &&rhs)
        noexcept(noexcept(std::declval<type &>() = std::forward<RHS>(rhs)))
    {
        *pair_.template as<type>() = std::forward<RHS>(rhs);
        return *this;
    }

    void destroy() noexcept { pair_.template destroy<type>(); }

    auto &value() noexcept { return *this->pair_.template as<type>(); }
    const auto &value() const noexcept { return const_cast<KeyValuePairWrapper *>(this)->value(); }
};

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
    std::array<KeyValuePairWrapper<K, MV>, SlotCount> values_;
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
                values_[index].destroy();
                #if ZOO_CONFIG_DEEP_ASSERTIONS
                    ++destroyedCount;
                #endif
            }
        }
        #if ZOO_CONFIG_DEEP_ASSERTIONS
            assert(destroyedCount == elementCount_);
        #endif
    }

    auto findParameters(const K &k) const noexcept {
        auto hashCode = Hash{}(k);
        auto fibonacciScrambled = fibonacciIndexModulo(hashCode);
        auto homeIndex =
            lemireModuloReductionAlternative<RequestedSize>(fibonacciScrambled);
        auto hoisted = hashReducer<HashBits>(hashCode);
        return
            std::tuple{
                hoisted,
                homeIndex,
                [thy = this, &k](size_t ndx) noexcept {
                    return KE{}(thy->values_[ndx].value().first, k);
                }
            };
    }

    auto find(const K &k) const noexcept {
        auto [hoisted, homeIndex, keyChecker] = findParameters(k);
        auto thy = const_cast<RH_Frontend_WithSkarupkeTail *>(this);
        Backend be{thy->md_.data()};
        auto [index, deadline, dontcare] =
            be.findMisaligned_assumesSkarupkeTail(
                hoisted, homeIndex, keyChecker
            );
        return deadline ? values_.end() : values_.data() + index;
    }

    auto insert(const K &k, const MV &mv) {
        auto [hoistedT, homeIndex, kc] = findParameters(k);
        auto hoisted = hoistedT;
        auto thy = const_cast<RH_Frontend_WithSkarupkeTail *>(this);
        Backend be{thy->md_.data()};
        auto [iT, deadlineT, needleT] =
            be.findMisaligned_assumesSkarupkeTail(hoisted, homeIndex, kc);
        auto index = iT;
        auto deadline = deadlineT;
        auto needle = needleT;
        if(!deadline) { return std::pair{values_.data() + index, false}; }

        // Do the chain of relocations
        // From this point onward, the hashes don't matter except for the
        // updates to the metadata, the relocations

        auto swarIndex = index / MD::Lanes;
        auto intraIndex = index % MD::Lanes;
        auto mdp = this->md_.data() + swarIndex;
        constexpr auto MaxRelocations = 64;
        std::array<std::size_t, MaxRelocations> relocations;
        auto relocationsCount = 0;
        //auto needlePSLs = needle.PSLs();
        for(;;) {
            // Loop invariant:
            // deadline is true, swarIndex (but not `index`), intraIndex is set
            // mdp points to the haystack that gave the deadline
            // needle is correct
            
            // Make a backup for making the new needle since we will change
            // this in the eviction
            auto mdBackup = *mdp;
            auto evictedPSL = mdBackup.PSLs().at(intraIndex);
            if(0 == evictedPSL) { // end of eviction chain!
                assignMetadataElement(deadline, needle, mdp);
                if(0 == relocationsCount) { // direct build of a new value
                    values_[index].build(std::pair{k, mv});
                    return std::pair{values_.data() + index, true};
                }
                // do the pair relocations
                while(relocationsCount--) {
                    auto fromIndex = relocations[relocationsCount];
                    values_[index].value() =
                        std::move(values_[fromIndex].value());
                    index = fromIndex;
                }
                values_[index].value() = std::pair(k, mv);
                return std::pair{values_.data() + index, true};
            }
            // evict the "deadline" element:
            // first, insert the current needle in its place (the needle stole)
            // make it the new needle.
            // find the place for it: when Robin Hood breaks again.

            // for this search, we need to make a search needle with only
            // the PSL being evicted.

            // The needle stole the entry: replace it with the needle
            assignMetadataElement(deadline, needle, mdp);
            // now the needle will be the old metadata entry
            needle = mdBackup;
            // "push" the index of the element that will be evicted
            relocations[relocationsCount++] = index;

            // now, where should the evicted element go to?
            // assemble a new needle
            
            // Constants relevant for the rest
            constexpr auto Ones = meta::BitmaskMaker<U, 1, MD::NBits>::value;
                // | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
            constexpr auto ProgressionFromOne = MD(Ones * Ones);
                // | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
            constexpr auto ProgressionFromZero =
                MD(ProgressionFromOne - MD(Ones));
                // | 0 | 1 | 2 | 3 | ...       | 7 |
            constexpr auto BroadcastSWAR_ElementCount =
                MD(meta::BitmaskMaker<U, MD::Lanes, MD::NBits>::value);
                // | 8 | 8 | 8 | 8 | ...       | 8 |
            constexpr auto SWARIterationAddendumBase =
                ProgressionFromZero + BroadcastSWAR_ElementCount;
                // | 8 | 9 | ...               | 15 |

            auto broadcastedEvictedPSL = broadcast(MD(evictedPSL));
            auto evictedPSLWithProgressionFromZero =
                broadcastedEvictedPSL + ProgressionFromZero;
                // | ePSL+0 | ePSL+1 | ePSL+2 | ePSL+3 | ... | ePSL+7 |
            auto needlePSLs =
                evictedPSLWithProgressionFromZero.shiftLanesLeft(intraIndex);
                    // zeroes make the new needle
                    // "richer" in all elements lower than the deadline
                    // because of the progression starts with 0
                    // the "deadline" element will have equal PSL, not
                    // "poorer".
                // assuming the deadline happened in the index 2:
                // needlePSLs = |    0   |   0    | ePSL   | ePSL+1 | ... | ePSL+5 |
            // find the place for the new needle, without checking the keys.
            auto haystackPSLs = mdBackup.PSLs();
            // haystack < needle => !(haystack >= needle)
            auto breaksRobinHood =
                not greaterEqual_MSB_off(haystackPSLs, needlePSLs);
            if(!breaksRobinHood) {
                // no place for the evicted element found in this swar.
                // increment the PSLs in the needle to check the next haystack

                // for the next swar, we will want (continuing the assumption
                // of the deadline happening at index 2)
                // | ePSL+6 | ePSL+7 | ePSL+8 | ePSL+9 | ... | ePSL+13 |
                // from evictedPSLWithProgressionFromZero,
                // shift "right" NLanes - intraIndex (keep the last two lanes):
                // | ePSL+6 | ePSL+7 | 0 | ... | 0 |
                auto lowerPart =
                    evictedPSLWithProgressionFromZero.
                        shiftLanesRight(MD::Lanes - intraIndex);
                // the other part, of +8 onwards, is
                // ProgressionFromZero + BroadcastElementCount, shifted:
                //    | 0 | 1 | 2 | 3 | ...       | 7 |
                // +  | 8 | 8 | 8 | 8 | ...       | 8 |
                // == | 8 | 9 | ...               | 15 |
                // shifted two lanes:
                //    | 0 | 0 | 8 | 9 | ...       | 13 |
                auto topAdd =
                    (ProgressionFromZero + BroadcastSWAR_ElementCount).
                        shiftLanesLeft(intraIndex);
                needlePSLs = needlePSLs + lowerPart + topAdd;
                for(;;) { // hunt for the next deadline
                    ++swarIndex;
                        // should the maintenance of `index` be replaced
                        // with pointer arithmetic on mdp?
                    ++mdp;
                    haystackPSLs = mdp->PSLs();
                    breaksRobinHood =
                        not greaterEqual_MSB_off(haystackPSLs, needlePSLs);
                    if(breaksRobinHood) { break; }
                    needlePSLs = needlePSLs + BroadcastSWAR_ElementCount;
                }
            }
            deadline = isolateLSB(breaksRobinHood);
            intraIndex = breaksRobinHood.lsbIndex();
        }
    }

    static void
    assignMetadataElement(U deadline, MD needle, MD *haystack) noexcept {
        auto deadlineAsElementWithValue1 = deadline >> (MD::NBits - 1);
        auto deadlineElementLowBitsOn = deadline - deadlineAsElementWithValue1;
        auto deadlineElementBlitMask = MD(deadline | deadlineElementLowBitsOn);
        *haystack =
            (*haystack & ~deadlineElementBlitMask) |
            (needle & deadlineElementBlitMask);
    }
};

} // rh

} // swar, zoo

#endif
