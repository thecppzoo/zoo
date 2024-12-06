#ifndef ZOO_REMEDIES_17_IOTA_H
#define ZOO_REMEDIES_17_IOTA_H

#include "zoo/range_support_overloads.h"

#include <array>
#include <utility> // For std::index_sequence

#include <array>
    // Required for std::array, used in iota and the makeModifiable
    // specialization
#include <type_traits>
    // Required for std::remove_const, used to deduce the non-const value_type
namespace zoo {
    namespace impl {
        template<typename T, std::size_t... Is>
        constexpr auto iotaImpl(std::index_sequence<Is...>) {
            return std::array<T, sizeof...(Is)>{Is...};
        }
    }

    /// \brief Alas! std::iota is not constexpr
    template<typename T, std::size_t N>
    constexpr auto iota() {
        return impl::iotaImpl<T>(std::make_index_sequence<N>{});
    }
}

#endif // ZOO_REMEDIES_17_IOTA_H

