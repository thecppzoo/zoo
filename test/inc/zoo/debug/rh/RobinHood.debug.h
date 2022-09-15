#ifndef ZOO_TEST_ROBINHOOD_DEBUGGING
#define ZOO_TEST_ROBINHOOD_DEBUGGING

#include "zoo/map/RobinHood.h"

#include <sstream>
#include <stdio.h>

namespace zoo {
namespace debug {
namespace rh {

template<typename Table, typename Callback>
auto display(
    const Table &t,
    std::size_t begin, std::size_t end,
    Callback &&c
) {
    std::ostringstream out;

    using MD = typename Table::MD;
    auto swarNdx = begin / MD::NSlots;
    auto intraNdx = begin % MD::NSlots;
    auto initial = t.md_[swarNdx];
    initial = initial.shiftLanesRight(intraNdx);
    char buffer[30];
    auto printLine =
        [&]() {
            char buffer[10];
            sprintf(
                buffer,
                "%02llx %02llx %02llx",
                initial.at(0), initial.hashes().at(0), initial.PSLs().at(0)
            );
            out << buffer;
            if(initial.PSLs().at(0)) {
                auto &v = t.values_[begin].value();
                out << ' ' << v.first << ':' << v.second;
                c(out, v.first, v.second);
            }
            out << '\n';
            initial = initial.shiftLanesRight(1);
            ++begin;
        };
    switch(intraNdx) {
        do {
            case 0: printLine();
            case 1: printLine();
            case 2: printLine();
            case 3: printLine();
            case 4: printLine();
            case 5: printLine();
            case 6: printLine();
            case 7: printLine();
            out << "- " << begin << '\n';
            initial = t.md_[++swarNdx];
        } while(begin < end);
    }
    return out.str();
}

template<typename Table>
auto display(const Table &t, std::size_t begin, std::size_t end) {
    return display(t, begin, end, [](std::ostream &, auto &&, auto &&){});
}

template<typename Table>
auto satisfiesInvariant(const Table &map, std::size_t begin = 0, std::size_t end = 0) {
    auto size = map.md_.size();
    auto swarIndexBegin =
        (size <= begin) ?
            0 :
            begin / Table::MD::NSlots;
    auto swarIndexEnd =
        (begin == end) ?
            map.md_.size() :
            (end + Table::MD::NSlots - 1)/Table::MD::NSlots; // ceiling
    auto prior = map.md_[swarIndexBegin].PSLs().at(0);
    for(
        auto ndx = swarIndexBegin * Table::MD::NSlots;
        swarIndexBegin != swarIndexEnd;
        ++swarIndexBegin, ndx += Table::MD::NSlots
    ) {
        auto md = map.md_[swarIndexBegin];
        auto v = md.PSLs();
        for(auto n = Table::MD::NSlots; n--; ++ndx) {
            auto current = v.at(0);
            if(prior + 1 < current) {
                return std::tuple(false, ndx);
            }
            v = v.shiftLanesRight(1);
            prior = current;
        }
    }
    return std::tuple(true, std::size_t(0));
}

} // rh
} // debug
} // zoo

#endif
