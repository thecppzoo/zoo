#ifndef ZOO_RANGE_STREAM_H
#define ZOO_RANGE_STREAM_H

#include "zoo/pp/traits_support.h"
#include "zoo/range_streamability_traits.h"
#include <ostream>

#if 0
// Template operator<< for ranges with streamable value types
template<
    typename Range,
    typename = std::enable_if_t<
        zoo::RangeWithStreamableValueType_v<Range>
    >,
    typename = std::enable_if_t<!zoo::IsStreamable_v<Range>>
>
std::ostream &operator<<(std::ostream &os, const Range &range) {
    auto it = std::begin(range);
    auto end = std::end(range);

    os << "{ ";
    while (it != end) {
        os << *it;
        if (++it != end) {
            os << ", ";
        }
    }
    os << " }";
    return os;
}
#endif

#endif // ZOO_RANGE_STREAM_H
