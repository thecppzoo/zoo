#include "zoo/map/RobinHood.h"
#include "zoo/debug/rh/RobinHood.debug.h"

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch.hpp>

#include <fstream>
#include <regex>
#include <map>
#include <unordered_map>
#include <random>

auto length(const std::string &s) { return s.length(); }
auto length(int) { return 1; }

template<typename Map, typename Corpus, typename... MapConstructionParameters>
auto nonRandom_insertion_find_benchmarkCore(
    Corpus &&corpusArg,
    const char *what,
    MapConstructionParameters &&...cps
) {
    // not measuring the performance of destruction
    std::vector<Map> temporaries;
    temporaries.reserve(1000);
    auto tcc = 0, twc = 0, tdwc = 0, tmax = 0;
    auto pass = 0;
    BENCHMARK(what) {
        ++pass;
        Corpus corpus(corpusArg);
        auto wordCount = 0, characterCount = 0, differentWords = 0;
        temporaries.emplace_back(std::forward<MapConstructionParameters>(cps)...);
        auto &m = temporaries.back();
        auto max = 0;
        while(corpus) {
            const auto &word = corpus.key();
            ++wordCount;
            characterCount += length(word);
            auto fr = m.find(word);
            if(m.end() == fr) {
                ++differentWords;
                try {
                    m.insert(typename Map::value_type{word, 1});
                } catch(std::exception &e) {
                    WARN(pass << ' ' << wordCount << ' ' << word << ' ' << e.what());
                    throw;
                }
            } else {
                auto v = ++(fr->second);
                if(max < v) { max = v; }
            }
            corpus.next();
        }
        tcc = characterCount; twc = wordCount; tdwc = differentWords; tmax = max;
        return std::tuple(tcc, twc, tdwc, max);
    };
    return std::tuple(twc, tcc, tdwc, tmax);
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

namespace std {

template<typename TT, size_t... Indices>
ostream &printTupleElements(ostream &out, TT &&tu, index_sequence<Indices...>) {
    [](...){}((out << ' ' << get<Indices>(tu), 0)...);
    return out;
}

template<typename... Ts>
ostream &operator<<(ostream &out, const tuple<Ts...> &tu) {
    return printTupleElements(out, tu, make_index_sequence<sizeof...(Ts)>{});
}

}

// The canonical example: 5 bits for PSL, 3 for hoisted hashes
template<typename K, typename V, std::size_t RS>
using RHF = zoo::rh::RH_Frontend_WithSkarupkeTail<K, V, RS, 5, 3>;
// Small, non-trivial size
template<typename K, typename V>
using RHF7000 = RHF<K, V, 7000>;

TEST_CASE("Robin Hood - insertion and finding", "[robin-hood]") {
    // the sequence of strings from a corpus
    std::vector<std::string> strings;
    // a sequence of integers from the strings encoded as intgers
    std::vector<int> ints;

    auto initialize = [&]() {
        std::ifstream corpus("/tmp/RobinHood.corpus.txt");
        if(!corpus) {
            abort();
        }
        std::string line;
        std::regex words{"\\w+"};

        corpus.clear();
        corpus.seekg(0);
        auto wordCount = 0, characterCount = 0, dwc = 0;
        std::unordered_map<std::string, int> converter;

        while(corpus) {
            getline(corpus, line);
            
            std::sregex_iterator
                wordsEnd{},
                wordIterator{line.begin(), line.end(), words};
            while(wordsEnd != wordIterator) {
                const auto &str = wordIterator->str();
                strings.push_back(str);
                auto fr = converter.find(str);
                if(converter.end() == fr) {
                    converter.insert(fr, std::pair{str, dwc});
                    ints.push_back(dwc);
                    ++dwc;
                } else {
                    ints.push_back(fr->second);
                }
                ++wordCount;
                characterCount += str.size();
                ++wordIterator;
            }
        }
        return std::tuple(wordCount, characterCount, dwc);
    };
    auto [wc, cc, dwc] = initialize();
    WARN("Number of different words" << dwc);
    auto rhR =
        nonRandom_insertion_find_benchmarkCore<
            RHF7000<std::string, int>
        >(ContainerCorpus{strings}, "RH");
    auto rhRint =
        nonRandom_insertion_find_benchmarkCore<
            RHF7000<int, int>
        >(ContainerCorpus{ints}, "RH - ints");

    auto count = 0;
    BENCHMARK("baseline - hashing strings") {
        ContainerCorpus fromStrings(strings);
        auto charCount = 0;
        auto xorAccumulatedHashes = std::hash<std::string>{}("Hello");
        while(fromStrings) {
            charCount += fromStrings.key().length();
            xorAccumulatedHashes ^= std::hash<std::string>{}(fromStrings.key());
            fromStrings.next();
        }
        count = charCount;
        return charCount ^ xorAccumulatedHashes;
    };
    REQUIRE(cc == count);

    auto mapR =
        nonRandom_insertion_find_benchmarkCore<
            std::map<std::string, int>
        >(ContainerCorpus{strings}, "std::map");
    CHECK(rhR == mapR);
    auto uoR =
        nonRandom_insertion_find_benchmarkCore<
            std::unordered_map<std::string, int>
        >(ContainerCorpus{strings}, "std::unordered_map", 7000);
    CHECK(mapR == uoR);
    nonRandom_insertion_find_benchmarkCore<
        std::unordered_map<int, int>
    >(ContainerCorpus{ints}, "std::unordered_map - int", 7000);
}

TEST_CASE("Robin Hood - Random", "[robin-hood]") {
    std::random_device rd;
    auto seed = rd();
    WARN("Seed:" << seed);
    std::mt19937 g;
    g.seed(seed);
    constexpr auto ElementCount = 100000;
    std::array<uint64_t, ElementCount> elements;
    auto counter = 0;
    for(auto &e: elements) { e = counter++; }
    std::shuffle(elements.begin(), elements.end(), g);
    using RH = zoo::rh::RH_Frontend_WithSkarupkeTail<int, int, ElementCount, 5, 3>;
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
    WARN("Robin Hood load factor " << double(ElementCount)/RH::SlotCount);
    BENCHMARK("baseline - running the mt19937") {
        auto gc = g;
        auto rv = 0;
        for(auto count = 10000; count--; ) {
            rv ^= gc();
        }
        return rv;
    };
    constexpr auto drawFrom = int(9 * elements.size());
    auto core = [&](auto &map, const char *name) {
        auto found = 0, notFound = 0, max = 0;
        auto end = map.end();
        auto resultCode = 0;
        auto passes = 0;
        std::mt19937 gc;
        gc.seed(seed);
        BENCHMARK(name) {
            ++passes;
            for(auto count = 2*drawFrom; count--; ) {
                auto key = gc() % (drawFrom/2);
                auto findResult = map.find(key);
                if(end == findResult) {
                    ++notFound;
                } else {
                    auto &v = findResult->second;
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

    /*
    Since the number of passes is not the same, some extra-logic is needed
    to be able to compare the results.
    CHECK(umr == rhr);
    CHECK(mr == rhr);
    CHECK(mr == umr);
    */
    std::ofstream chain("/tmp/chain.txt");
    chain << "Seed " << seed << '\n';
    chain << zoo::debug::rh::display(rh, 0, RH::SlotCount);
}

template<std::size_t InsertionCount, typename Map, typename... MapConstructionParms>
void randomInsertionCore(std::mt19937 g, const char *name, MapConstructionParms &&...ps) {
BENCHMARK(name) {
    auto previouslyFound = 0;
    Map m(std::forward<MapConstructionParms>(ps)...);
    for(auto count = InsertionCount; count--; ) {
        auto ir = m.insert(typename Map::value_type(g(), 1));
        if(!ir.second) {
            ++previouslyFound;
        }
    }
};}

template<std::size_t RS, int PSL, int H>
using RHT = zoo::rh::RH_Frontend_WithSkarupkeTail<int, int, RS, PSL, H>;

TEST_CASE("Robin Hood - insertions", "[robin-hood]") {
    std::random_device rd;
    std::mt19937 g;
    auto seed = rd();
    g.seed(seed);
    WARN("Seed: " << seed);
    using um = std::unordered_map<int, int>;
    randomInsertionCore<1000, RHT<1500, 6, 2>>(g, "1000 - 6/2");
    randomInsertionCore<1000, um>(g, "1000 - std", 1500);
    randomInsertionCore<1000, RHT<1500, 5, 3>>(g, "1000 - 5/3");
    randomInsertionCore<2000, RHT<2500, 6, 2>>(g, "2000 - 6/2");
    randomInsertionCore<2000, um>(g, "2000 - std", 2000);
    randomInsertionCore<50000, um>(g, "50000 - std", 50000);
}

