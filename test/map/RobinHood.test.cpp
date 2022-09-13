#include "zoo/map/RobinHood.h"
#include "zoo/map/RobinHoodAlt.h"
#include "zoo/map/RobinHoodUtil.h"

#include <catch2/catch.hpp>

#include <algorithm>
#include <regex>
#include <map>

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

auto blue(int sPSL, int hh, int key, int homeIndex) {
    RHC fromPointer{md};
    auto r =
        fromPointer.findMisaligned_assumesSkarupkeTail(
            hh,
            homeIndex,
            [&](int matchIndex) { return collectionOfKeys[matchIndex] == key; }
        );
}

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

auto instantiateInsert(int v, FrontendExample &f) {
    return f.insert(v, 0);
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

using SMap = zoo::rh::RH_Frontend_WithSkarupkeTail<std::string, int, 256, 5, 3>;

auto valueInvoker(void *p, std::size_t index) {
    return static_cast<SMap *>(p)->values_[index].value();
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

    std::regex words("\\w+");
    std::sregex_iterator
        wordsEnd{},
        wordIterator{HenryVChorus.begin(), HenryVChorus.end(), words};
    while(wordsEnd != wordIterator) {
        const auto &word = wordIterator->str();
        auto findResult = ex.find(word);
        auto mfr = mirror.find(word);
        WARN(word);
        if("accomplishment" == word) {
            WARN(mirror.size());
        }
        bool resultEndInMirror = mfr == mirror.end();
        bool resultEndInExample = findResult == ex.end();
        if(resultEndInMirror != resultEndInExample) {
            WARN(resultEndInMirror);
            auto r = ex.find(word);
        }
        REQUIRE(resultEndInMirror == resultEndInExample);
        auto showRecord = [&](auto iter) {
            auto [hh, indexHome, dc] = ex.findParameters(word);
            auto index = (iter - ex.values_.begin());
            auto swarIndex = index / MD::NSlots;
            WARN(
                index << ':' << swarIndex << ':' <<
                (index % MD::NSlots) << ' ' <<
                showMetadata(index, ex.md_.data()) <<
                ' ' << hh << ' ' << indexHome
            );
        };
        if(resultEndInMirror) {
            mirror[word] = 1;
            auto [iter, inserted] = ex.insert(word, 1);
            REQUIRE(inserted);
            showRecord(iter);
        } else {
            ++mirror[word];
            ++findResult->value().second;
            REQUIRE(mirror[word] == findResult->value().second);
            showRecord(findResult);
            WARN(word << ' ' << mirror[word]);
        }
        ++wordIterator;
    }
    WARN(mirror.size());
}

using RH35u32 = zoo::rh::RH_Backend<3, 5, u32>;

TEST_CASE("RobinHood basic needle", "[api][mapping][swar][robin-hood]") {

CHECK(0x0403'0201u == RH35u32::makeNeedle(0, 0).value());
CHECK(0x1615'1413u == RH35u32::makeNeedle(2, 2).value());
CHECK(0x1817'1615u == RH35u32::makeNeedle(4, 2).value());

}

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
