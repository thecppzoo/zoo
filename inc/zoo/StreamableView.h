#ifndef ZOO_STREAMABLE_VIEW_H
#define ZOO_STREAMABLE_VIEW_H

#include "zoo/range_streamability_traits.h"

#include <ostream>
#include <utility>

namespace zoo {

template<
    typename Range,
    typename = std::enable_if_t<
        RangeWithStreamableElements_v<Range> &&
        !Streamable_v<Range>
    >
>
class StreamableView {
public:
    explicit StreamableView(const Range &range) : range_(range) {}

    const Range &base() const { return range_; }

private:
    const Range &range_;
};


// Overload operator<< for StreamableView
template <typename Range>
std::ostream &operator<<(std::ostream &os, const StreamableView<Range> &view) {
    const auto &range = view.base();
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

} // namespace zoo

#endif // ZOO_STREAMABLE_VIEW_H

