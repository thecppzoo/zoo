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
#include <stdexcept>

#if ZOO_CONFIG_DEEP_ASSERTIONS
    #include <assert>
#endif

namespace zoo {
namespace rh {

struct RobinHoodException: std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct MaximumProbeSequenceLengthExceeded: RobinHoodException {
    using RobinHoodException::RobinHoodException;
};
struct RelocationStackExhausted: RobinHoodException {
    using RobinHoodException::RobinHoodException;
};

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

    /// Boolean SWAR true in the first element/lane of the needle strictly poorer than its corresponding
    /// haystack
    constexpr static auto
    firstInvariantBreakage(Metadata needle, Metadata haystack) {
        auto nPSL = needle.PSLs();
        auto hPSL = haystack.PSLs();
        auto theyKeepInvariant =
            greaterEqual_MSB_off(hPSL, nPSL);
            // BTW, the reason to have encoded the PSLs in the least
            // significant bits is to be able to call the cheaper version
            // _MSB_off here

        auto theyBreakInvariant = not theyKeepInvariant;
        // because we make the assumption of LITTLE ENDIAN byte ordering,
        // we're interested in the elements up to the first haystack-richer
        auto firstBreakage = swar::isolateLSB(theyBreakInvariant.value());
        return firstBreakage;
    }

    constexpr static impl::MatchResult<PSL_Bits, HashBits, U>
    potentialMatches(
        Metadata needle, Metadata haystack
    ) noexcept {
        // We need to determine if there are potential matches to consider
        auto sames = equals(needle, haystack);
        auto deadline = firstInvariantBreakage(needle, haystack);
        // In a valid haystack, the PSLs can grow at most by 1 per entry.
        // If a PSL is richer than the needle in any place, because the
        // needle, by construction, always grows at least by 1 per entry,
        // then the PSL won't be equal again.
        // There is no need to filter potential matches using the deadline
        // as previous versions of the code did.
        return {
            deadline,
            sames
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

    template<typename KeyComparer>
    constexpr
    std::tuple<std::size_t, U, Metadata>
    findMisaligned_assumesSkarupkeTail(
        U hoistedHash, int homeIndex, const KeyComparer &kc
    ) const noexcept __attribute__((always_inline));
};

template<int PSL_Bits, int HashBits, typename U>
template<typename KeyComparer>
inline constexpr
std::tuple<std::size_t, U, typename RH_Backend<PSL_Bits, HashBits, U>::Metadata>
RH_Backend<PSL_Bits, HashBits, U>::findMisaligned_assumesSkarupkeTail(
        U hoistedHash, int homeIndex, const KeyComparer &kc
    ) const noexcept {
        auto misalignment = homeIndex % Metadata::NSlots;
        auto baseIndex = homeIndex / Metadata::NSlots;
        auto base = this->md_ + baseIndex;

        constexpr auto Ones = meta::BitmaskMaker<U, 1, Width>::value;
        constexpr auto Progression = Metadata{Ones * Ones};
        constexpr auto AllNSlots =
            Metadata{meta::BitmaskMaker<U, Metadata::NSlots, Width>::value};
        MisalignedGenerator_Dynamic<Metadata> p(base, int(Metadata::NBits * misalignment));
        auto index = homeIndex;
        auto needle = makeNeedle(0, hoistedHash);

        for(;;) {
            auto hay = *p;
            auto result = potentialMatches(needle, hay);
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
                // The deadline is relative to the misalignment.
                // To build an absolute deadline, there are two cases:
                // the bit falls in the first SWAR or the second SWAR.
                // The same applies for needle.
                // in general, for example a misaglignment of 6:
                // { . | . | . | . | . | . | . | .}{ . | . | . | . | . | . | . | . }
                //                         { a | b | c | d | e | f | g | h }
                // shift left (to higher bits) by the misalignment
                // { 0 | 0 | 0 | 0 | 0 | 0 | a | b }
                // shift right (to lower bits) by NSlots - misalignment:
                // { c | d | e | f | g | h | 0 | 0 }
                // One might hope undefined behavior might be reasonable (zero
                // result, unchanged result), but ARM proves that undefined
                // behavior is indeed undefined, so we do our right shift as a
                // double: shift by n-1, then shift by 1.
                auto mdd = Metadata{deadline};
                auto toAbsolute = [](auto v, auto ma) {
                    auto shiftedLeft = v.shiftLanesLeft(ma);
                    auto shiftedRight =
                        v.shiftLanesRight(Metadata::NSlots - ma - 1).shiftLanesRight(1);
                    return Metadata{shiftedLeft | shiftedRight};
                };
                auto position = index + Metadata{deadline}.lsbIndex();
                return
                    std::tuple(
                        position,
                        toAbsolute(mdd, misalignment).value(),
                        toAbsolute(needle, misalignment)
                    );
            }
            // Skarupke's tail allows us to not have to worry about the end
            // of the metadata
            ++p;
            index += Metadata::NSlots;
            needle = needle + AllNSlots;
        }
    }

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
    size_t RequestedSize_,
    int PSL_Bits, int HashBits,
    typename Hash = std::hash<K>,
    typename KE = std::equal_to<K>,
    typename U = std::uint64_t,
    typename Scatter = FibonacciScatter<U>,
    typename RangeReduce = LemireReduce<RequestedSize_, U>,
    typename HashReduce = TopHashReducer<HashBits, U>
>
struct RH_Frontend_WithSkarupkeTail {
    using Backend = RH_Backend<PSL_Bits, HashBits, U>;
    using MD = typename Backend::Metadata;

    constexpr static inline auto RequestedSize = RequestedSize_;
    constexpr static inline auto LongestEncodablePSL = (1 << PSL_Bits);
    constexpr static inline auto WithTail =
        RequestedSize +
        LongestEncodablePSL // the Skarupke tail
    ;
    constexpr static inline auto SWARCount =
        (
            WithTail +
            MD::NSlots - 1 // to calculate the ceiling rounding
        ) / MD::NSlots
    ;
    constexpr static inline auto SlotCount = SWARCount * MD::NSlots;
    constexpr static inline auto HighestSafePSL =
        LongestEncodablePSL - MD::NSlots - 1;

    using MetadataCollection = std::array<MD, SWARCount>;
    using value_type = std::pair<K, MV>;

    MetadataCollection md_;
    /// \todo Scatter key and value in a flavor
    std::array<KeyValuePairWrapper<K, MV>, SlotCount> values_;
    size_t elementCount_;

    RH_Frontend_WithSkarupkeTail() noexcept: elementCount_(0) {
        for(auto &mde: md_) { mde = MD{0}; }
    }

    template<typename Callable>
    void traverse(Callable &&c) const {
        for(size_t swarIndex = 0; swarIndex < SWARCount; ++swarIndex) {
            auto PSLs = md_[swarIndex].PSLs();
            auto occupied = booleans(PSLs);
            while(occupied) {
                auto intraIndex = occupied.lsbIndex();
                c(swarIndex, intraIndex);
                occupied = occupied.clearLSB();
            }
        }
    }

    ~RH_Frontend_WithSkarupkeTail() {
        traverse([thy=this](std::size_t sI, std::size_t intra) {
            thy->values_[intra + sI * MD::NSlots].destroy();
        });
    }

    RH_Frontend_WithSkarupkeTail(const RH_Frontend_WithSkarupkeTail &model):
        RH_Frontend_WithSkarupkeTail()
    {
        model.traverse([thy=this,other=&model](std::size_t sI, std::size_t intra) {
            auto index = intra + sI * MD::NSlots;
            thy->values_[index].build(other->values_[index].value());
            thy->md_[sI] = thy->md_[sI].blitElement(other->md_[sI], intra);
            ++thy->elementCount_;
        });
    }

    RH_Frontend_WithSkarupkeTail(RH_Frontend_WithSkarupkeTail &&donor) noexcept:
        md_(donor.md_), elementCount_(donor.elementCount_)
    {
        traverse([thy=this, other=&donor](std::size_t sI, std::size_t intra) {
            auto index = intra + sI * MD::NSlots;
            thy->values_[index].build(std::move(other->values_[index].value()));
        });
    }


    auto findParameters(const K &k) const noexcept {
        auto [hoisted, homeIndex] =
            findBasicParameters<
                K, RequestedSize, HashBits, U,
                Hash, Scatter, RangeReduce, HashReduce
            >(k);
        return
            std::tuple{
                hoisted,
                homeIndex,
                [thy = this, &k](size_t ndx) noexcept {
                    return KE{}(thy->values_[ndx].value().first, k);
                }
            };
    }

    template<typename ValuteTypeCompatible>
    auto insert(ValuteTypeCompatible &&val) {
        auto &k = val.first;
        auto &mv = val.second;
        auto [hoistedT, homeIndexT, kc] = findParameters(k);
        auto hoisted = hoistedT;
        auto homeIndex = homeIndexT;
        auto thy = const_cast<RH_Frontend_WithSkarupkeTail *>(this);
        Backend be{thy->md_.data()};
        auto [iT, deadlineT, needleT] =
            be.findMisaligned_assumesSkarupkeTail(hoisted, homeIndex, kc);
        auto index = iT;
        if(HighestSafePSL < index - homeIndex) {
            throw MaximumProbeSequenceLengthExceeded("Scanning for eviction, from finding");
        }
        auto deadline = deadlineT;
        if(!deadline) { return std::pair{values_.data() + index, false}; }
        auto needle = needleT;
        auto rv =
            insertionEvictionChain(
                index, deadline, needle,
                std::forward<ValuteTypeCompatible>(val)
            );
        ++elementCount_;
        return rv;
    }

    // Do the chain of relocations
    // From this point onward, the hashes don't matter except for the
    // updates to the metadata, the relocations
    template<typename VTC>
    auto insertionEvictionChain(
        std::size_t index,
        U deadline,
        MD needle,
        VTC &&val
    ) {
        auto &k = val.first;
        auto &mv = val.second;
        auto swarIndex = index / MD::Lanes;
        auto intraIndex = index % MD::Lanes;
        auto mdp = this->md_.data() + swarIndex;

        // Because we have not decided about strong versus basic exception
        // safety guarantee, for the time being we will just put a very large
        // number here.
        constexpr auto MaxRelocations = 100000;
        std::array<std::size_t, MaxRelocations> relocations;
        std::array<int, MaxRelocations> newElements;
        auto relocationsCount = 0;
        auto elementToInsert = needle.at(intraIndex);

        // The very last element in the metadata will always have a psl of 0
        // this serves as a sentinel for insertions, the only place to make
        // sure the table has not been exhausted is an eviction chain that
        // ends in the sentinel
        // Also, the encoding for the PSL may be exhausted
        for(;;) {
            // Loop invariant:
            // deadline, index, swarIndex, intraIndex, elementToInsert correct
            // mdp points to the haystack that gave the deadline
            auto md = *mdp;
            auto evictedPSL = md.PSLs().at(intraIndex);
            if(0 == evictedPSL) { // end of eviction chain!
                if(SlotCount - 1 <= index) {
                    throw MaximumProbeSequenceLengthExceeded("full table");
                }
                if(0 == relocationsCount) { // direct build of a new value
                    values_[index].build(
                        std::piecewise_construct,
                        std::tuple(std::forward<VTC>(val).first),
                        std::tuple(std::forward<VTC>(val).second)
                    );
                    *mdp = mdp->blitElement(intraIndex, elementToInsert);
                    return std::pair{values_.data() + index, true};
                }
                // the last element is special because it is a
                // move-construction, not a move-assignment
                --relocationsCount;
                auto fromIndex = relocations[relocationsCount];
                values_[index].build(
                    std::move(values_[fromIndex].value())
                );
                md_[swarIndex] =
                    md_[swarIndex].blitElement(
                        intraIndex, elementToInsert
                    );
                elementToInsert = newElements[relocationsCount];
                index = fromIndex;
                swarIndex = index / MD::NSlots;
                intraIndex = index % MD::NSlots;
                // do the pair relocations
                while(relocationsCount--) {
                    fromIndex = relocations[relocationsCount];
                    values_[index].value() =
                        std::move(values_[fromIndex].value());
                    md_[swarIndex] =
                        md_[swarIndex].blitElement(intraIndex, elementToInsert);
                    elementToInsert = newElements[relocationsCount];
                    index = fromIndex;
                    swarIndex = index / MD::NSlots;
                    intraIndex = index % MD::NSlots;
                }
                values_[index].value() = std::forward<VTC>(val);
                md_[swarIndex] =
                    md_[swarIndex].blitElement(intraIndex, elementToInsert);
                return std::pair{values_.data() + index, true};
            }
            if(HighestSafePSL < evictedPSL) {
                throw MaximumProbeSequenceLengthExceeded("Encoding insertion");
            }
            
            // evict the "deadline" element:
            // first, insert the element in its place (it "stole")
            // find the place for the evicted: when Robin Hood breaks again.

            // for this search, we need to make a search needle with only
            // the PSL being evicted.

            // "push" the index of the element that will be evicted
            relocations[relocationsCount] = index;
            // we have a place for the element being inserted, at this index
            newElements[relocationsCount++] = elementToInsert;
            if(MaxRelocations <= relocationsCount) {
                throw RelocationStackExhausted("Relocation Stack");
            }

            // now the insertion will be for the old metadata entry
            elementToInsert = md.hashes().at(intraIndex);

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
            auto haystackPSLs = md.PSLs();
            // haystack < needle => !(haystack >= needle)
            auto breaksRobinHood =
                not greaterEqual_MSB_off(haystackPSLs, needlePSLs);
            if(!bool(breaksRobinHood)) {
                // no place for the evicted element found in this swar.
                // increment the PSLs in the needle to check the next haystack

                // for the next swar, we will want (continuing the assumption
                // of the deadline happening at index 2)
                // old needle:
                // |    0   |    0   |  ePSL  | ePSL+1 | ... | ePSL+5  |
                // desired new needle PSLs:
                // | ePSL+6 | ePSL+7 | ePSL+8 | ePSL+9 | ... | ePSL+13 |
                // from evictedPSLWithProgressionFromZero,
                // shift "right" NLanes - intraIndex (keep the last two lanes):
                // | ePSL+6 | ePSL+7 | 0 | ... | 0 |
                auto lowerPart =
                    evictedPSLWithProgressionFromZero.
                        shiftLanesRight(MD::Lanes - intraIndex - 1).
                        shiftLanesRight(1);
                // the other part, of +8 onwards, is BroadcastElementCount,
                // shifted:
                //    | 8 | 8 | 8 | 8 | ...       | 8 |
                // shifted two lanes:
                //    | 0 | 0 | 8 | 8 | ...       | 8 |
                //
                auto topAdd =
                    BroadcastSWAR_ElementCount.shiftLanesLeft(intraIndex);
                needlePSLs = needlePSLs + lowerPart + topAdd;
                for(;;) { // hunt for the next deadline
                    ++swarIndex;
                        // should the maintenance of `index` be replaced
                        // with pointer arithmetic on mdp?
                    index += MD::NSlots;
                    ++mdp;
                    haystackPSLs = mdp->PSLs();
                    breaksRobinHood =
                        not greaterEqual_MSB_off(haystackPSLs, needlePSLs);
                    if(breaksRobinHood) { break; }
                    evictedPSL += MD::NSlots;
                    if(HighestSafePSL < evictedPSL) {
                        throw MaximumProbeSequenceLengthExceeded("Scanning for eviction, insertion");
                    }
                    needlePSLs = needlePSLs + BroadcastSWAR_ElementCount;
                }
            }
            deadline = swar::isolateLSB(breaksRobinHood.value());
            intraIndex = breaksRobinHood.lsbIndex();
            index = swarIndex * MD::NSlots + intraIndex;
            elementToInsert = elementToInsert | needlePSLs.at(intraIndex);
        }
    }

    auto begin() const noexcept { return this->values_.begin(); }
    auto end() const noexcept { return this->values_.end(); }

    typename std::array<KeyValuePairWrapper<K, MV>, SlotCount>::iterator
    inline find(const K &k) noexcept __attribute__((always_inline));

    auto find(const K &k) const noexcept {
        const_cast<RH_Frontend_WithSkarupkeTail *>(this)->find(k);
    }
};

template<
    typename K,
    typename MV,
    size_t RequestedSize_,
    int PSL_Bits, int HashBits,
    typename Hash,
    typename KE,
    typename U,
    typename Scatter,
    typename RangeReduce,
    typename HashReduce
>
auto
RH_Frontend_WithSkarupkeTail<
    K, MV, RequestedSize_, PSL_Bits, HashBits, Hash, KE, U, Scatter,
    RangeReduce, HashReduce
>::find(const K &k) noexcept ->
typename std::array<KeyValuePairWrapper<K, MV>, SlotCount>::iterator
{
        auto [hoisted, homeIndex, keyChecker] = findParameters(k);
        Backend be{this->md_.data()};
        auto [index, deadline, dontcare] =
            be.findMisaligned_assumesSkarupkeTail(
                hoisted, homeIndex, keyChecker
            );
        return deadline ? values_.end() : values_.data() + index;
    }

} // rh

} // swar, zoo

#endif
