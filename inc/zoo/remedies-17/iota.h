#ifndef ZOO_REMEDIES_17_IOTA_H
#define ZOO_REMEDIES_17_IOTA_H

#include "zoo/range_adl_overloads.h"

#include <algorithm>
    // Required for std::copy, used to copy elements between containers
#include <utility>
    // Required for std::index_sequence and std::make_index_sequence, used in
    // iota

namespace zoo {

/// \brief Generates the sequence 0, ..., N - 1 as constexpr
///
/// All because std::iota is not constexpr!
template <typename Container, std::size_t N>
struct Iota {
    static_assert(
        std::is_default_constructible_v<Container>,
        "Container must be default constructible."
    );
    static_assert(
        std::is_member_function_pointer_v<decltype(&Container::resize)>,
        "Container must support resize()."
    );

    static constexpr Container value = []() constexpr {
        Container result;
        result.resize(N);

        auto it = std::begin(result);
        for (std::size_t i = 0; i < N; ++i, ++it) {
            *it = i;
        }
        return result;
    }();
};

template <typename T, std::size_t N>
struct Iota<std::array<T, N>, N> {
    static constexpr std::array<T, N> value = []() constexpr {
        std::array<T, N> result = {}; // Ensure value initialization

        for (std::size_t i = 0; i < N; ++i) {
            result[i] = static_cast<T>(i); // Directly initialize elements
        }
        return result;
    }();
};

template <typename Container, std::size_t N>
constexpr auto Iota_v = Iota<Container, N>::value;

/// \brief constexpr containers make their elements `const`, this gives you a modifiable copy
template <typename TargetContainer = void, typename SourceContainer>
auto makeModifiable(const SourceContainer &constContainer) {
    using ValueType =
        typename std::remove_const<typename SourceContainer::value_type>::type;

    // Determine the target container type
    using ResultContainer = std::conditional_t<
        std::is_void<TargetContainer>::value,
        SourceContainer, // Default to the source container type
        TargetContainer>; // Use specified container type if provided

    // Create and populate the target container
    ResultContainer result(begin(constContainer), end(constContainer));
    return result;
}

// Specialization for std::array
template <typename T, std::size_t N>
auto makeModifiable(const std::array<T, N> &constArray) {
    using ModifiableT = std::remove_const_t<T>;
    std::array<ModifiableT, N> modifiableArray;
    std::copy(begin(constArray), end(constArray), begin(modifiableArray));
    return modifiableArray;
}

}

#endif // ZOO_REMEDIES_17_IOTA_H

