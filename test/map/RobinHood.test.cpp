#include "zoo/map/RobinHood.h"
#include "zoo/map/RobinHoodAlt.h"
#include "zoo/map/RobinHoodUtil.h"

#include <catch2/catch.hpp>

#include <algorithm>
#include <regex>
#include <map>
#include <sstream>

using namespace zoo;
using namespace zoo::swar;
using namespace zoo::rh;

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

// Robin Hood, canonical test
using RHC = zoo::rh::RH_Backend<5, 3>;

int *collectionOfKeys;
RHC::Metadata *md;

using FrontendExample =
    zoo::rh::RH_Frontend_WithSkarupkeTail<int, int, 1024, 5, 3>;

auto instantiateFrontEndDefConst() {
    return new FrontendExample;
}

auto instantiateFED(FrontendExample *ptr) {
    delete ptr;
}

auto instantiateFind(int v, FrontendExample &f) {
    return f.find(v);
}

static_assert(
    0xE9E8E7E6E5E4E3E2ull == RHC::makeNeedle(1, 7).value()
);

struct V {
    u64 v;
    u64 intraIndex;
};

std::ostream &operator<<(std::ostream &out, V v) {
    char buffer[30];
    char *ptr = buffer;
    auto val = v.v;
    auto printHalf = [&](auto low, auto high) {
        for(auto ndx = low; ndx < high; ++ndx) {
            if(v.intraIndex == ndx) {
                ptr += sprintf(ptr, "<");
            }
            ptr += sprintf(ptr, "%02llx", val & 0xFF);
            if(v.intraIndex == ndx) {
                ptr += sprintf(ptr, ">");
            }
            val >>= 8;
        }
    };
    printHalf(0, 4);
    ptr += sprintf(ptr, "'");
    printHalf(4, 8);
    out << buffer;
    return out;
}

template<typename MD>
auto showMetadata(std::size_t index, MD *md) {
    auto swarIndex = index / MD::NSlots;
    return V{md[swarIndex].value(), index % MD::NSlots};
}

#define ZOO_TEST_TRACE_WARN(...)
//WARN(__VA_ARGS__)

using SMap = zoo::rh::RH_Frontend_WithSkarupkeTail<std::string, int, 155, 5, 3>;

auto valueInvoker(void *p, std::size_t index) {
    return static_cast<SMap *>(p)->values_[index].value();
}

auto validateMetadata(SMap &map) {
    auto prior = 0;
    auto ndx = 0;
    for(auto md: map.md_) {
        auto v = md.PSLs();
        assert(0 == ndx % SMap::MD::NSlots);
        for(auto n = SMap::MD::NSlots; n--; ++ndx) {
            auto current = v.at(0);
            if(prior + 1 < current) {
                auto d = map.md_.data();
                ZOO_TEST_TRACE_WARN("broken " << showMetadata(ndx - SMap::MD::NSlots, d));
                ZOO_TEST_TRACE_WARN("broken " << showMetadata(ndx, d) << ' ' << ndx << ' ' << map.values_[ndx].value().first);
                ZOO_TEST_TRACE_WARN("broken " << showMetadata(ndx + SMap::MD::NSlots, d));
                return std::tuple(false, ndx); }
            v = v.shiftLanesRight(1);
            prior = current;
        }
    }
    return std::tuple(true, 0);
}

template<typename Table>
auto display(const Table &t, std::size_t begin, std::size_t end) {
    std::ostringstream out;

    using MD = typename Table::MD;
    //auto mdp = t.md_.data();
    auto swarNdx = begin / MD::NSlots;
    auto intraNdx = begin % MD::NSlots;
    auto initial = t.md_[swarNdx];
    initial = initial.shiftLanesRight(intraNdx);
    char buffer[30];
    auto printLine =
        [&]() {
            char buffer[4];
            sprintf(buffer, "%02llx", initial.at(0));
            out << buffer;
            if(initial.PSLs().at(0)) {
                auto &v = t.values_[begin].value();
                out << ' ' << v.first << ':' << v.second;
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

TEST_CASE("Robin Hood", "[api][mapping][swar][robin-hood]") {
    std::string HenryVChorus =
        "O for a Muse of fire, that would ascend\n"
        "The brightest heaven of invention,\n"
        "A kingdom for a stage, princes to act\n"
        "And monarchs to behold the swelling scene!\n"
        "Then should the warlike Harry, like himself,\n"
        "Assume the port of Mars; and at his heels,\n"
        "Leash'd in like hounds, should famine, sword and fire\n"
        "Crouch for employment. But pardon, and gentles all,\n"
        "The flat unraised spirits that have dared\n"
        "On this unworthy scaffold to bring forth\n"
        "So great an object: can this cockpit hold\n"
        "The vasty fields of France? or may we cram\n"
        "Within this wooden O the very casques\n"
        "That did affright the air at Agincourt?\n"
        "O, pardon! since a crooked figure may\n"
        "Attest in little place a million;\n"
        "And let us, ciphers to this great accompt,\n"
        "On your imaginary forces work.\n"
        "Suppose within the girdle of these walls\n"
        "Are now confined two mighty monarchies,\n"
        "Whose high upreared and abutting fronts\n"
        "The perilous narrow ocean parts asunder:\n"
        "Piece out our imperfections with your thoughts;\n"
        "Into a thousand parts divide on man,\n"
        "And make imaginary puissance;\n"
        "Think when we talk of horses, that you see them\n"
        "Printing their proud hoofs i' the receiving earth;\n"
        "For 'tis your thoughts that now must deck our kings,\n"
        "Carry them here and there; jumping o'er times,\n"
        "Turning the accomplishment of many years\n"
        "Into an hour-glass: for the which supply,\n"
        "Admit me Chorus to this history;\n"
        "Who prologue-like your humble patience pray,\n"
        "Gently to hear, kindly to judge, our play.\n"
    ;
    std::transform(
        HenryVChorus.begin(), HenryVChorus.end(),
        HenryVChorus.begin(), [](char c) { return std::tolower(c); }
    );

    SMap ex;
    using MD = SMap::MD;
    std::map<std::string, int> mirror;

    auto allKeysThere = [&]() {
        for(auto &v: mirror) {
            auto fr = ex.find(v.first);
            if(ex.end() == fr) { return false; }
        }
        return true;
    };
    
    std::regex words("\\w+");
    std::sregex_iterator
        wordsEnd{},
        wordIterator{HenryVChorus.begin(), HenryVChorus.end(), words};
    while(wordsEnd != wordIterator) {
        const auto &word = wordIterator->str();
        ZOO_TEST_TRACE_WARN(word);
        auto findResult = ex.find(word);
        auto mfr = mirror.find(word);
        bool resultEndInMirror = mfr == mirror.end();
        bool resultEndInExample = findResult == ex.end();
        REQUIRE(resultEndInMirror == resultEndInExample);
        auto showRecord = [&](auto iter) {
            auto [hh, indexHome, dc] = ex.findParameters(word);
            auto index = (iter - ex.values_.begin());
            auto swarIndex = index / MD::NSlots;
            ZOO_TEST_TRACE_WARN(
                index << ':' << swarIndex << ':' <<
                (index % MD::NSlots) << ' ' <<
                showMetadata(index, ex.md_.data()) <<
                ' ' << hh << ' ' << indexHome
            );
        };
        if(resultEndInMirror) {
            mirror[word] = 1;
            auto [iter, inserted] = ex.insert(SMap::value_type{word, 1});
            REQUIRE(inserted);
            REQUIRE(allKeysThere());
        } else {
            ++mirror[word];
            ++findResult->value().second;
            REQUIRE(mirror[word] == findResult->value().second);
            ZOO_TEST_TRACE_WARN(word << ' ' << mirror[word]);
        }
        auto [ok, failureNdx] = validateMetadata(ex);
        CHECK(ok);
        if(!ok) {
            auto ms = mirror.size();
            ZOO_TEST_TRACE_WARN("At " << ms << ' ' << failureNdx);
            ZOO_TEST_TRACE_WARN(word << '\n' << display(ex, failureNdx - 2, failureNdx + 14));
        }
        ++wordIterator;
    }
    WARN(mirror.size());
}

using FrontendSmall32 =
    zoo::rh::RH_Frontend_WithSkarupkeTail<int, int, 16, 5, 3,
        std::hash<int>, std::equal_to<int>, u32>;

TEST_CASE("Robin Hood Metadata peek/poke u32",
          "[api][mapping][swar][robin-hood]") {
    FrontendSmall32::MetadataCollection md;
    md.fill(FrontendSmall32::MD{0});  // Zero for testing.
    for (auto i = 0; i < md.size(); i++) {
        auto [psl1, hash1] = zoo::rh::impl::peek(md, i);
        CHECK(0 == psl1);
        CHECK(0 == hash1);
    }
    for (auto i = 0; i < md.size(); i++) {
        zoo::rh::impl::poke(md, i, 16+1, 4+1);
        auto [psl2, hash2] = zoo::rh::impl::peek(md, i);
        CHECK(17 == psl2);
        CHECK(5 == hash2);
    }
}


TEST_CASE("Robin Hood Metadata peek/poke u32 synthetic metadata basic",
          "[api][mapping][swar][robin-hood]") {
    FrontendSmall32 table;

    zoo::rh::impl::poke(table.md_, 1, 0x1, 0x7);
    CHECK(std::tuple{1,0x7} == zoo::rh::impl::peek(table.md_, 1));
    CHECK(std::tuple{0,0} == zoo::rh::impl::peek(table.md_, 0));
    CHECK(std::tuple{0,0} == zoo::rh::impl::peek(table.md_, 2));

    // If we ask for a skarupke tail 
    FrontendSmall32::Backend be{table.md_.data()};
    auto [index, deadline, metadata] =
        be.findMisaligned_assumesSkarupkeTail(0x7, 1, [](int i) {return true;});
    CHECK(1 == index);
    CHECK(0x0000'0000u == deadline);
    CHECK(0x0000'0000u == metadata.value());
}

template <typename MetadataCollection>
void writeFourBlock(int offset, MetadataCollection& md) {
    zoo::rh::impl::poke(md, 1+offset, 0x1, 0x7);
    zoo::rh::impl::poke(md, 2+offset, 0x1, 0x5);
    zoo::rh::impl::poke(md, 3+offset, 0x1, 0x3);
    CHECK(std::tuple{0x1, 0x7} == zoo::rh::impl::peek(md, 1+offset));
    CHECK(std::tuple{0x1, 0x5} == zoo::rh::impl::peek(md, 2+offset));
    CHECK(std::tuple{0x1, 0x3} == zoo::rh::impl::peek(md, 3+offset));
}

template <typename MetadataCollection>
void writeIncrementPSL(int offset, MetadataCollection& md) {
    zoo::rh::impl::poke(md, 1+offset, 0x1, 0x7);
    zoo::rh::impl::poke(md, 2+offset, 0x2, 0x5);
    zoo::rh::impl::poke(md, 3+offset, 0x3, 0x3);
    CHECK(std::tuple{0x1, 0x7} == zoo::rh::impl::peek(md, 1+offset));
    CHECK(std::tuple{0x2, 0x5} == zoo::rh::impl::peek(md, 2+offset));
    CHECK(std::tuple{0x3, 0x3} == zoo::rh::impl::peek(md, 3+offset));
}

TEST_CASE("Robin Hood Metadata peek/poke u32 synthetic metadata block of three",
          "[api][mapping][swar][robin-hood]") {
    FrontendSmall32 table;
    writeFourBlock(0, table.md_);
    CHECK(std::tuple{0,0} == zoo::rh::impl::peek(table.md_, 0));
    CHECK(std::tuple{1,0x7} == zoo::rh::impl::peek(table.md_, 1));
    CHECK(std::tuple{1,0x5} == zoo::rh::impl::peek(table.md_, 2));
    CHECK(std::tuple{1,0x3} == zoo::rh::impl::peek(table.md_, 3));
    CHECK(std::tuple{0,0} == zoo::rh::impl::peek(table.md_, 4));

    FrontendSmall32::Backend be{table.md_.data()};
    auto kcGen = [](int z) { return [z](int i){ return i == z;};};
    auto trueKc = [](int z) { return true; };
    {
    auto [index, deadline, metadata] =
        be.findMisaligned_assumesSkarupkeTail(0x7, 1, trueKc);
    CHECK(1 == index);
    CHECK(0x0000'0000u == deadline);
    CHECK(0x0000'0000u == metadata.value());
    }
    {
    auto [index, deadline, metadata] =
        be.findMisaligned_assumesSkarupkeTail(0x5, 2, trueKc);
    CHECK(2 == index);
    CHECK(0x0000'0000u == deadline);
    CHECK(0x0000'0000u == metadata.value());
    }
    {
    auto [index, deadline, metadata] =
        be.findMisaligned_assumesSkarupkeTail(0x3, 3, trueKc);
    CHECK(3 == index);
    CHECK(0x0000'0000u == deadline);
    CHECK(0x0000'0000u == metadata.value());
    }
    {
    auto [index, deadline, metadata] =
        be.findMisaligned_assumesSkarupkeTail(0x6, 1, trueKc);
    // try to find at homeIndex 1, receive idx 2 as insertionIndex, as idx 2
    // has a psl of 1 and needle has psl of 2 at that point.
    CHECK(2 == index);
    CHECK(0x0080'0000u == deadline);
    // Returned metadata contains one valid lane: the lane at the offset of
    // deadline.
    CHECK(0x00c2'0000u ==
        metadata.isolateLane(FrontendSmall32::MD{deadline}.lsbIndex()));
    }
}

TEST_CASE("Robin Hood Metadata peek/poke u32 synthetic metadata psl one",
          "[api][mapping][swar][robin-hood]") {
    FrontendSmall32 table;
    writeFourBlock(0, table.md_);
    writeFourBlock(5, table.md_);
    writeFourBlock(10, table.md_);
    writeFourBlock(15, table.md_);
    auto trueKc = [](int z) { return true; };

    FrontendSmall32::Backend be{table.md_.data()};
    for (auto i = 0 ; i< table.md_.size() ;i++) {
        auto [p,h] =zoo::rh::impl::peek(table.md_, i);
        if ( p == 0) continue;
        // All lookups for entries in the metadata should work correctly.
        auto [index, deadline, metadata] =
            be.findMisaligned_assumesSkarupkeTail(h, i, trueKc);
        CHECK(p == 1);
        CHECK(i == index);
        CHECK(0x0000'0000u == deadline);
        CHECK(0x0000'0000u == metadata.value());
        // No entries have a 0 hash, all psl are 1, so all entry points will be
        // 1 after a valid entry.
        auto [missIndex, missDeadline, missMetadata] =
            be.findMisaligned_assumesSkarupkeTail(0, i, trueKc);
        CHECK(i+1 == missIndex);
        CHECK((missIndex)%4 ==
            FrontendSmall32::MD{missDeadline}.lsbIndex());
        CHECK(0x02 == 
            missMetadata.at(FrontendSmall32::MD{missDeadline}.lsbIndex()));
    }
    {
    auto [index, deadline, metadata] =
        be.findMisaligned_assumesSkarupkeTail(0x6, 1, trueKc);
    // try to find at homeIndex 1, receive idx 2 as insertionIndex, as idx 2
    // has a psl of 1 and needle has psl of 2 at that point.
    CHECK(2 == index);
    CHECK(0x0080'0000u == deadline);
    // Returned metadata contains one valid lane: the lane at the offset of
    // deadline.
    CHECK(0x00c2'0000u ==
        metadata.isolateLane(FrontendSmall32::MD{deadline}.lsbIndex()));
    }
}

TEST_CASE("Robin Hood Metadata peek/poke u32 synthetic metadata psl not one",
          "[api][mapping][swar][robin-hood]") {
    FrontendSmall32 table;
    writeIncrementPSL(0, table.md_);
    writeIncrementPSL(5, table.md_);
    writeIncrementPSL(10, table.md_);
    writeIncrementPSL(15, table.md_);
    auto trueKc = [](int z) { return true; };

    FrontendSmall32::Backend be{table.md_.data()};
    for (auto i = 0 ; i< table.md_.size() ;i++) {
        auto [p,h] =zoo::rh::impl::peek(table.md_, i);
        if ( p == 0) continue;
        // All lookups for entries in the metadata should work correctly.
        auto [index, deadline, metadata] =
            be.findMisaligned_assumesSkarupkeTail(h, i-p+1, trueKc);
        CHECK(i == index);
        CHECK(0x0000'0000u == deadline);
        CHECK(0x0000'0000u == metadata.value());
        // No entries have a 0 hash, metadata has 3 long blocks that have 1,2,3
        // psl, all missed key lookups will have a 4 psl, so their index will
        // be at i - psl + 4.
        auto [missIndex, missDeadline, missMetadata] =
            be.findMisaligned_assumesSkarupkeTail(0, i-p+1, trueKc);
        CHECK(i-p+4 == missIndex);
        CHECK((missIndex)%4 ==
            FrontendSmall32::MD{missDeadline}.lsbIndex());
        CHECK(0x04 == 
            missMetadata.at(FrontendSmall32::MD{missDeadline}.lsbIndex()));
    }
}

using RH35u32 = zoo::rh::RH_Backend<3, 5, u32>;

static_assert(0x0403'0201u == RH35u32::makeNeedle(0, 0).value());
static_assert(0x1615'1413u == RH35u32::makeNeedle(2, 2).value());
static_assert(0x1817'1615u == RH35u32::makeNeedle(4, 2).value());

TEST_CASE("RobinHood potentialMatches", "[api][mapping][swar][robin-hood]") {
    //U deadline, Metadata<PSL_Bits, HashBits, U> potentialMatches;
    auto needle = RH35u32::Metadata{RH35u32::makeNeedle(2, 2).value()};

    // If haystack is identical to needle, we get no deadline and 4 potential
    // matches.
    auto haystack = RH35u32::Metadata{0x1615'1413};
    CHECK(haystack.value() == needle.value());
    auto m1 = RH35u32::potentialMatches(needle, haystack);
    CHECK(0x0000'0000u == m1.deadline);

    CHECK(0x8080'8080u == m1.potentialMatches.value());

    // If haystack has no matches and is poorer than needle, we should get no
    // deadline and no potential matches.
    auto missHaystack = RH35u32::Metadata{0x0707'0707u};
    auto m2 = RH35u32::potentialMatches(needle, missHaystack);
    CHECK(0x0000'0000u == m2.potentialMatches.value());
    CHECK(0 == m2.deadline);

    // If the haystack has no matches and has a richer element, we should get a
    // deadline and no matches.
    auto deadlineHaystack = RH35u32::Metadata{0x0515'0403u};
    auto m3 = RH35u32::potentialMatches(needle, deadlineHaystack);
    CHECK(0x0080'0000u == m3.potentialMatches.value());
    CHECK(0x80'00'00'00 == m3.deadline);
}

TEST_CASE(
    "BadMix",
    "[hash]"
) {
    u64 v = 0x0000'0000'0000'0001ull;
    // badmix turns to all ones, chopped to type width.
    CHECK(0xffff'ffff'ffff'ffffull == zoo::rh::badMixer<64>(v));
    CHECK(0x0000'ffff'ffff'ffffull == zoo::rh::badMixer<48>(v));
    CHECK(0x0000'0000'ffff'ffffull == zoo::rh::badMixer<32>(v));
    CHECK(0x0000'0000'0000'ffffull == zoo::rh::badMixer<16>(v));
}

TEST_CASE(
    "SlotMetadataBasic",
    "[robinhood]"
) {
    using SM = SWAR<8, u32>;
    using SO35u32Ops = SlotOperations<3,5,u32>;
    using MD35u32Ops = SlotMetadata<3,5,u32>;
    CHECK(0x0504'0302u == SO35u32Ops::needlePSL(1).value());

    // 0 psl is reserved.
    const auto psl1 = 0x0403'0201u;
    const auto hash1 = 0x8080'8080u;
    {
    MD35u32Ops m;
    m.data_ = MD35u32Ops::SSL{0x0403'8201};
    CHECK(0x0000'8000u == m.attemptMatch(SM{hash1}, SM{psl1}).value());
    CHECK(0x0000'8000u == SO35u32Ops::attemptMatch(m.data_, SM{hash1}, SM{psl1}).value());
    }
    {
    MD35u32Ops m;
    m.data_ = MD35u32Ops::SSL{0x0401'8201};
    CHECK(0x0000'8001u == m.attemptMatch(SM{hash1}, SM{psl1}).value());
    CHECK(0x0000'8001u == SO35u32Ops::attemptMatch(m.data_, SM{hash1}, SM{psl1}).value());
    } 
}

