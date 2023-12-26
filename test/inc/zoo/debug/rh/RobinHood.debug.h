#ifndef ZOO_TEST_ROBINHOOD_DEBUGGING
#define ZOO_TEST_ROBINHOOD_DEBUGGING

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
    constexpr auto HexPerSlot = (MD::NBits + 3) / 4;
    constexpr auto HexPerPSL = (MD::NBitsLeast + 3) / 4;
    constexpr auto HexPerHash = (MD::NBitsMost + 3) / 4;

    char format[60];
    const auto hexPerSlot = static_cast<int>(HexPerSlot);
    snprintf(format, 59, "%%0%dllx %%0%dllx %%0%dllx", hexPerSlot, hexPerSlot, HexPerPSL);

    auto swarNdx = begin / MD::NSlots;
    auto swarEnd = end/MD::NSlots;

    auto initial = t.md_[swarNdx];
    auto intra = 0;
    auto printLine =
        [&]() {
            char buffer[100];
            snprintf(
                buffer,
                sizeof(buffer),
                format,
                initial.at(0), initial.hashes().at(0), initial.PSLs().at(0)
            );
            out << buffer;
            if(initial.PSLs().at(0)) {
                auto &v = t.values_[swarNdx * MD::NSlots + intra].value();
                out << ' ' << v.first << ':' << v.second;
                c(out, v.first, v.second);
            }
            out << '\n';
            initial = initial.shiftLanesRight(1);
            ++intra;
        };
    do {
        intra = 0;
        for(auto n = MD::NSlots; n--; ) {
            printLine();
        }
        initial = t.md_[++swarNdx];
        out << "- " << (swarNdx * MD::NSlots) << '\n';
    } while(swarNdx < swarEnd);
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
        ;
        swarIndexBegin != swarIndexEnd;
        ++swarIndexBegin
    ) {
        auto md = map.md_[swarIndexBegin];
        auto v = md.PSLs();
        for(auto n = Table::MD::NSlots; n--; ) {
            auto current = v.at(0);
            if(prior + 1 < current) {
                const std::size_t index = swarIndexBegin * Table::MD::NSlots + Table::MD::NSlots - n - 1;
                return std::tuple(false, index);
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
