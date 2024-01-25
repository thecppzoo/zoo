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

/*! \file RobinHood.h
\brief User entry point to the implementation of hash tables using the "Robin
Hood" invariant.

The "Robin Hood" monicker means that each key has a preferred or "home" slot
in the hash table.  If, upon insertion, the key can not be inserted into its
home slot, then the insertion would look to insert it as close as possible to
the home slot.

In this code base, the acronym PSL is used frequently, it means "Probe Sequence
Length", this is the distance from the preferred or "home" slot and the current
search position.
For a practical reason, a key inserted into its home has a PSL of 1, in this
way, the metadata indicates with a PSL of 0 that no key is in the slot,
or that the slot is free.

The invariant is that a key won't be inserted further away from its home than
the key in the current slot.  That is, a key is "richer" than another if it is
closer to its "home", the insertion mechanism would "evict" a key that would be
richer than the key being inserted.  In this regard, the "Robin Hood" metaphor
is realized: the insertion "steals" from the rich to give it to the poor.

\note All of this codebase makes the unchecked assumption that the byte ordering
is LITTLE ENDIAN

\todo complement with the other theoretical and practical comments relevant,
including:
1. How the table is not stable with regards to insetions and deletions,
2. How an insertion can cascade into very long chains of evictions/reinsertions
3. The theoretical guarantee that the longest PSL is in the order of Log(N)
4. How it seems that in practice the theoretical guarantee is not achieved.
...

\todo determine a moment to endure the version control pain of making the
indentation consistent.
*/

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

/// \brief The canonical backend (implementation)
template<int PSL_Bits, int HashBits, typename U = std::uint64_t>
struct RH_Backend {
    using Metadata = impl::Metadata<PSL_Bits, HashBits, U>;

    constexpr static inline auto Width = Metadata::NBits;
    Metadata *md_;

    /// Boolean SWAR true in the first element/lane of the needle strictly
    /// poorer than its corresponding haystack
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

     // This should be more generic: if PSLs breach a broadcast PSL, saturate
     // This should be more generic: if a SWAR breaches a SWAR condition, saturate.
     constexpr static auto
     needlePSLSaturation(Metadata nPSL) {
         // create a saturator for max PSL.  If any needle saturates, all later PSLs will be set to saturated.
         constexpr auto saturatedPSL = broadcast(Metadata(Metadata::MaxPSL));
         //auto nPSL = needle.PSLs();
         auto saturation = greaterEqual_MSB_off(nPSL, saturatedPSL);
         auto invertSatMask = ((swar::isolateLSB(saturation.value()) - 1) );
         auto satMask = (~(swar::isolateLSB(saturation.value()) - 1) );
         //if (not bool(saturation)) return std::tuple{nPSL, false};
         // Least sig lane is saturated, all more sig must be made saturated.
         auto needlePSLsToSaturate = Metadata{satMask & saturatedPSL.value()};
         // addition might have overflown nPSL before entering function
         return std::tuple{Metadata{nPSL.PSLs() | needlePSLsToSaturate}, bool(saturation)};  // saturated at any point, last swar to check.
     }


    /*! \brief SWAR check for a potential match

    The invariant in Robin Hood is that the element being looked for, the
    "needle", is at least as "rich" as the elements already present (the
    "haystack").
    "Richer" means that the PSL is smaller.
    A PSL of 0 can only happen in the haystack, to indicate the slot is empty,
    this is "richest".
    The first time the needle has a PSL greater than the haystacks' means the
    matching will fail, because the hypothetical prior insertion would have
    "stolen" that slot.
    If the PSLs are equal, it starts a sequence of potential matches.  To
    determine if there is an actual match, perform:
    1. A cheap SWAR check of hoisted hashes
    2. If there are still potential matches (now also the hoisted hashes), fall
    back to non-SWAR, or iterative and expensive "deep equality" test for each
    potential match, outside of this function.

    The above makes it very important to detect the first case in which the PSL
    is greater equal to the needle.  We call this the "deadline".
    We assume the LITTLE ENDIAN byte ordering: the first element will
    be the least significant non-false Boolean SWAR.

    Note about performance:
    Every "early exit" faces a big justification hurdle, the proportion of cases
    they intercept must be large enough that the branch prediction penalty of the
    entropy introduced (by the early exit) is overcompensated.
    */

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
    inline constexpr
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

/// \brief The slots in the table may have a key-value pair or not, this
/// optionality is not suitably captured by any standard library component,
/// hence we need to implement our own.
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

    auto valuePtr() const noexcept {
        auto deconstified = const_cast<KeyValuePairWrapper *>(this);
        return deconstified->pair_.template as<type>();
    }

    auto &value() noexcept { return *valuePtr(); }
    const auto &value() const noexcept { return const_cast<KeyValuePairWrapper *>(this)->value(); }
};

/// \brief Frontend with the "Skarupke Tail"
///
/// Normally we need to explicitly check for whether key searches have reached
/// the end of the table.  Malte Skarupke devised a tail of table entries to
/// make this explicit check unnecessary: Regardless of the end of the table,
/// a search must terminate in failure if the maximum PSL is reached, then,
/// by just adding an extra maximum PSL entries to the table, while keeping the
/// slot indexing function the same, searches at the end of the table will never
/// attempt to go past the real end, but return not-found within the tail.
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
        if(!deadline) { return std::pair{iterator(values_.data() + index), false}; }
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
        constexpr auto MaxRelocations = 150000;
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
                    return std::pair{iterator(values_.data() + index), true};
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
                return std::pair{iterator(values_.data() + index), true};
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

    struct const_iterator {
        const KeyValuePairWrapper<K, MV> *p_;

        // note: ++ not yet implemented, we can't iterate ;-)

        const value_type &operator*() noexcept { return p_->value(); }
        const value_type *operator->() noexcept { return &p_->value(); }

        bool operator==(const const_iterator &other) const noexcept {
            return p_ == other.p_;
        }

        bool operator!=(const const_iterator &other) const noexcept {
            return p_ == other.p_;
        }

        const_iterator(const KeyValuePairWrapper<K, MV> *p): p_(p) {}
        const_iterator(const const_iterator &) = default;
    };

    struct iterator: const_iterator {
        value_type *ncp() { return const_cast<value_type *>(&this->p_->value()); }
        value_type &operator*() noexcept { return *ncp(); }
        value_type *operator->() noexcept { return ncp(); }
        using const_iterator::const_iterator;
    };

    const_iterator begin() const noexcept { return this->values_.data(); }
    const_iterator end() const noexcept {
        return this->values_.data() + this->values_.size();
    }

    inline iterator find(const K &k) noexcept __attribute__((always_inline));

    const_iterator find(const K &k) const noexcept {
        const_cast<RH_Frontend_WithSkarupkeTail *>(this)->find(k);
    }

    auto displacement(const_iterator from, const_iterator to) {
        return to.p_ - from.p_;
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
>::find(const K &k) noexcept -> iterator
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
