#include "zoo/map/RobinHood.h"
#include "zoo/debug/rh/RobinHood.debug.h"

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch.hpp>

#include <fstream>
#include <regex>
#include <map>
#include <unordered_map>

template<typename K, typename MV>
auto &value(zoo::rh::KeyValuePairWrapper<K, MV> *p) {
    return p->value();
};

template<typename It>
auto &value(It it) {
    return *it;
}

auto cc = 0, wc = 0, dwc = 0, max = 0;

auto length(const std::string &s) { return s.length(); }
auto length(int) { return 1; }

template<typename Map, typename Corpus, typename... MapConstructionParameters>
auto benchmarkCore(
    Corpus &&corpusArg,
    const char *what,
    MapConstructionParameters &&...cps
) {
    // not measuring the performance of destruction
    std::vector<Map> temporaries;
    temporaries.reserve(110);
    auto tcc = 0, twc = 0, tdwc = 0, tmax = 0;
    BENCHMARK(what) {
        Corpus corpus(corpusArg);
        auto wordCount = 0, characterCount = 0, differentWords = 0;
        temporaries.emplace_back();
        auto &m = temporaries.back();
        auto max = 0;
        while(corpus) {
            const auto &word = corpus.key();
            ++wordCount;
            characterCount += length(word);
            auto fr = m.find(word);
            if(m.end() == fr) {
                ++differentWords;
                m.insert(typename Map::value_type{word, 1});
            } else {
                auto v = ++value(fr).second;
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

template<typename... T>
void f(T &&...) {}

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

TEST_CASE("Robin Hood") {
    std::ifstream corpus("/tmp/RobinHood.corpus.txt");
    if(!corpus) {
        abort();
    }
    std::string line;
    std::regex words{"\\w+"};
    std::vector<std::string> strings;
    std::vector<int> ints;

    auto initialize = [&]() {
        corpus.clear();
        corpus.seekg(0);
        auto wordCount = 0, characterCount = 0;
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
                    converter.insert(fr, std::pair{str, wordCount});
                    ints.push_back(wordCount);
                } else {
                    ints.push_back(fr->second);
                }
                ++wordCount;
                characterCount += str.size();
                ++wordIterator;
            }
        }
        return std::tuple(wordCount, characterCount);
    };
    auto [wc, cc] = initialize();
    auto count = 0;
    BENCHMARK("baseline") {
        ContainerCorpus fromStrings(strings);
        auto charCount = 0;
        while(fromStrings) {
            charCount += fromStrings.key().length();
            fromStrings.next();
        }
        count = charCount;
        return charCount;
    };
    REQUIRE(cc == count);
    benchmarkCore<zoo::rh::RH_Frontend_WithSkarupkeTail<int, int, 7000, 6, 2>>(ContainerCorpus{ints}, "rh -  int");
    auto mapR = benchmarkCore<std::map<std::string, int>>(ContainerCorpus{strings}, "std::map");
    using RH6000 = zoo::rh::RH_Frontend_WithSkarupkeTail<std::string, int, 6000, 5, 3>;
    auto rhR = benchmarkCore<RH6000>(ContainerCorpus{strings}, "RH");
    CHECK(rhR == mapR);
    auto uoR = benchmarkCore<std::unordered_map<std::string, int>>(ContainerCorpus{strings}, "std::unordered_map", 6000);
    CHECK(mapR == uoR);
    benchmarkCore<std::map<std::string, int>>(ContainerCorpus{strings}, "std::map");
    benchmarkCore<std::unordered_map<int, int>>(ContainerCorpus{ints}, "std::unordered_map -  int", 6000);
    WARN("Results: " << mapR);
}
