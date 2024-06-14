#include "zoo/map/RobinHood.h"
#include "zoo/map/RobinHoodAlt.h"
#include "zoo/map/RobinHoodUtil.h"

#include "zoo/debug/rh/RobinHood.debug.h"

#include <catch2/catch.hpp>

#include <algorithm>
#include <regex>
#include <map>
#include <fstream>
#include <unordered_map>

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
    const auto bufferSize = sizeof(buffer);
    char *ptr = buffer;
    auto val = v.v;
    auto printHalf = [&](auto low, auto high) {
        for(auto ndx = low; ndx < high; ++ndx) {
            if(v.intraIndex == ndx) {
                ptr += snprintf(ptr, bufferSize, "<");
            }
            const std::size_t va = val & 0xFF;
            ptr += snprintf(ptr, bufferSize, "%02lx", va);
            if(v.intraIndex == ndx) {
                ptr += snprintf(ptr, bufferSize, ">");
            }
            val >>= 8;
        }
    };
    printHalf(0, 4);
    ptr += snprintf(ptr, bufferSize, "'");
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

using SMap = zoo::rh::RH_Frontend_WithSkarupkeTail<std::string, int, 255, 5, 3>;
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

    std::map<std::string, int> converter;
    auto differentWords = 0;
    zoo::rh::RH_Frontend_WithSkarupkeTail<int, int, 190, 5, 3> wordFreqByInt;

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
            converter[word] = differentWords++;
            REQUIRE(inserted);
            REQUIRE(allKeysThere());
        } else {
            ++mirror[word];
            ++findResult->second;
            REQUIRE(mirror[word] == findResult->second);
            ZOO_TEST_TRACE_WARN(word << ' ' << mirror[word]);
        }
        auto [ok, failureNdx] = zoo::debug::rh::satisfiesInvariant(ex);
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

template<typename Container>
struct ContainerCorpus {
    Container &c_;
    typename Container::const_iterator position_;

    ContainerCorpus(Container &c): c_(c), position_(begin(c)) {}

    operator bool() const noexcept { return c_.end() != position_; }

    auto &key() { return *position_; }

    void next() { ++position_; }
};

TEST_CASE("RH Validation") {
    std::ifstream corpus("/tmp/RobinHood.corpus.txt");
    if(!corpus) {
        abort();
    }
    std::string line;
    std::regex words{"\\w+"};
    std::vector<std::string> strings;
    std::vector<int> ints;
    std::unordered_map<int, std::string> decoder;

    auto initialize = [&]() {
        corpus.clear();
        corpus.seekg(0);
        auto wordCount = 0, characterCount = 0, dfw = 0;
        std::unordered_map<std::string, int> encoder;

        while(corpus) {
            getline(corpus, line);

            std::sregex_iterator
                wordsEnd{},
                wordIterator{line.begin(), line.end(), words};
            while(wordsEnd != wordIterator) {
                const auto &str = wordIterator->str();
                strings.push_back(str);
                auto fr = encoder.find(str);
                if(encoder.end() == fr) {
                    encoder.insert(fr, std::pair{str, dfw});
                    decoder[dfw] = str;
                    ints.push_back(dfw);
                    ++dfw;
                } else {
                    ints.push_back(fr->second);
                }
                characterCount += str.size();
                ++wordCount;
                ++wordIterator;
            }
        }
        return std::tuple(wordCount, characterCount);
    };
    auto [wc, cc] = initialize();

    auto tcc = 0, twc = 0, tdwc = 0, tmax = 0;
    if(true) {
        ContainerCorpus corpus(ints);
        auto wordCount = 0, characterCount = 0, differentWords = 0;
        using Map = zoo::rh::RH_Frontend_WithSkarupkeTail<int, int, 6000, 5, 3>;
        Map m;
        auto max = 0;
        while(corpus) {
            const auto &word = corpus.key();
            ++wordCount;
            characterCount += 1;
            auto fr = m.find(word);
            if(m.end() == fr) {
                ++differentWords;
                auto [where, whether] =
                    m.insert(typename Map::value_type{word, 1});
                REQUIRE(whether);
                auto at = m.displacement(m.begin(), where);
                auto [consistent, ndx] = zoo::debug::rh::satisfiesInvariant(m, at - 20, at + 5);
                INFO("inconsistency at " << ndx << " doing '" << decoder[word] << '\'');
                INFO(zoo::debug::rh::display(
                    m, ndx - 20, ndx + 2,
                    [&](std::ostream &o, int code, int reps) {
                        o << ' ' << decoder[code];
                    }
                ));
                REQUIRE(consistent);
            } else {
                auto v = ++fr->second;
                if(max < v) { max = v; }
            }
            corpus.next();
        }
        tcc = characterCount; twc = wordCount; tdwc = differentWords; tmax = max;
    };
}

#include <random>

TEST_CASE("Robin Hood - big", "[robin-hood]") {
    std::random_device rd;
    auto seed = rd();
    WARN("Seed:" << seed);
    std::mt19937 g;
    g.seed(seed);
    std::array<uint64_t, 40000> elements;
    auto counter = 0;
    for(auto &e: elements) { e = counter++; }
    std::shuffle(elements.begin(), elements.end(), g);
    using RH = zoo::rh::RH_Frontend_WithSkarupkeTail<int, int, 50000, 11, 5>;
    RH rh;
    for(auto &e: elements) {
        RH::value_type insertable{counter++, 1};
        auto ir = rh.insert(insertable);
        REQUIRE(ir.second);
    }
    auto [valid, problem] = zoo::debug::rh::satisfiesInvariant(rh, 0, 0);
    if(!valid) {
        std::ofstream bug("/tmp/bug.txt");
        bug << zoo::debug::rh::display(rh, 0, rh.SlotCount);
        REQUIRE(valid);
    }
}

struct TakeLamb {
    template<typename Callable>
    TakeLamb(Callable &&c) {
        c();
    }
};

template<typename K, typename MV>
auto &value(zoo::rh::KeyValuePairWrapper<K, MV> *p) {
    return p->value();
};

template<typename It>
auto &value(It it) {
    return *it;
}

#define Q(L)
#define BENCHMARK(name) TakeLamb bla##__LINE__ = [&]()

TEST_CASE("Robin Hood - Random", "[robin-hood]") {
    std::random_device rd;
    auto seed = rd();
    //auto seed = 2334323242;
    WARN("Seed:" << seed);
    std::mt19937 g;
    g.seed(seed);
    std::array<uint64_t, 100000> elements;
    auto counter = 0;
    for(auto &e: elements) { e = counter++; }
    std::shuffle(elements.begin(), elements.end(), g);
    using RH = zoo::rh::RH_Frontend_WithSkarupkeTail<int, int, 100000, 5, 11>;
    RH rh;
    std::unordered_map<int, int> um;
    std::map<int, int> m;
    for(auto &e: elements) {
        RH::value_type insertable{e, 1};
        try {
            auto ir = rh.insert(insertable);
            REQUIRE(ir.second);
        } catch(std::exception &e) {
            WARN("Exception: " << e.what());
            auto [hoisted, homeIndex, dontcare] =
                rh.findParameters(insertable.first);
            std::ofstream bolted("/tmp/onException.txt");
            bolted << "Inserting " << insertable.first << " failed\n";
            bolted << "Hoisted code " << std::hex << hoisted << " @" << std::dec << homeIndex << '\n';
            bolted << zoo::debug::rh::display(rh, 0, RH::SlotCount);
            REQUIRE(false);
        }
        um.insert(insertable);
        m.insert(insertable);
    }
    auto [valid, problem] = zoo::debug::rh::satisfiesInvariant(rh, 0, 0);
    if(!valid) {
        std::ofstream bug("/tmp/bug.txt");
        bug << "seed " << seed << " counter " << (counter - 1) << '\n';
        bug << "happened at " << problem << '\n';
        bug << zoo::debug::rh::display(rh, 0, rh.SlotCount);
        REQUIRE(valid);
    }
    WARN("Unordered Map load factor " << um.load_factor());
    BENCHMARK("baseline - running the mt19937") {
        auto gc = g;
        auto rv = 0;
        for(auto count = 10000; count--; ) {
            rv ^= gc();
        }
        return rv;
    };
    constexpr auto drawFrom = 2 * elements.size();
    auto core = [&](auto &map, const char *name) {
        auto found = 0, notFound = 0, max = 0;
        auto end = map.end();
        auto resultCode = 0;
        auto passes = 0;
        std::mt19937 gc;
        gc.seed(seed);
        BENCHMARK(name) {
            ++passes;
            for(auto count = 10000; count--; ) {
                auto key = gc() / drawFrom;
                auto findResult = map.find(key);
                if(end == findResult) {
                    ++notFound;
                } else {
                    auto &v = value(findResult).second;
                    ++v;
                    ++found;
                    if(max < v) { max = v; }
                }
            }
            resultCode = found ^ notFound ^ max;
            return resultCode;
        };
        return std::tuple(found, notFound, max, resultCode, passes);
    };
    auto umr = core(um, "Unordered Map");
    auto rhr = core(rh, "Robin Hood");
    auto mr = core(m, "std::map");

    /*CHECK(umr == rhr);
    CHECK(mr == rhr);
    CHECK(mr == umr);*/
    /*std::ofstream chain("/tmp/chain.txt");
    chain << "Seed " << seed << '\n';
    chain << zoo::debug::rh::display(rh, 0, RH::SlotCount);*/
}
