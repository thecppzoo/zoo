#ifndef ZOO_RANGE_ADL_OVERLOADS_H
#define ZOO_RANGE_ADL_OVERLOADS_H

/// \file
/// \brief Introduces `std` range functions into the `zoo` namespace to enable
/// unqualified calls that benefit from overload resolution via ADL.
///
/// Provides `using` declarations for standard range-based utility functions.
///
/// These `using` directives bring range-related functions from the standard
/// library (e.g., `std::begin`, `std::end`, `std::size`, etc.) into the `zoo`
/// namespace. This enables code inside `zoo` to use either the `std::` or
/// `zoo::` overloads without qualification.
///
/// The reason for this design is that if code within `zoo` explicitly qualifies
/// a call with `std::` (e.g., `std::begin(container)`), it will **always** use
/// the `std::` implementation, even when a more appropriate overload exists
/// in the `zoo` namespace. By introducing these `using` directives, we allow
/// unqualified calls like `begin(container)` or `size(container)`. The compiler
/// will then perform **argument-dependent lookup (ADL)** to select the best
/// match from either `std` or `zoo` namespaces.
///
/// This ensures flexibility and maintainability, as the code automatically
/// benefits from any custom `zoo` overloads while still supporting the standard
/// behavior for types without `zoo` implementations.

#include <iterator>

namespace zoo {
    using std::begin;
    using std::end;
    using std::cbegin;
    using std::cend;
    using std::rbegin;
    using std::rend;
    using std::crbegin;
    using std::crend;
    using std::size;
    using std::empty;
}

#endif // ZOO_RANGE_SUPPORT_OVERLOADS_H
