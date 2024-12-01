#include "zoo/Str.h"

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch.hpp>

#include <fstream>
#include <regex>
#include <map>
#include <unordered_map>
#include <string_view>

auto loadTextCorpus(std::string_view path) {
    std::vector<std::string> strings;
    std::vector<int> ints;
    std::unordered_map<std::string, int> forwardMap;
    int nextIndex = 0;

    std::ifstream text{path};
    if(!text) { abort(); }
    std::string line;
    std::regex separator{"\\w+"};
    text.clear();
    text.seekg(0);
    while(text) {
        getline(text, line);
        std::sregex_iterator
            wordsEnd{},
            wordIterator{line.begin(), line.end(), separator};
        while(wordsEnd != wordIterator) {
            const auto &word = wordIterator->str();
            strings.push_back(word);
            auto [where, notPresent] =
                forwardMap.insert({ word, nextIndex });
            ints.push_back(
                notPresent ? nextIndex++ : where->second
            );
            ++wordIterator;
        }
    }
    WARN(forwardMap.size() << " different strings identified");
    REQUIRE(ints.size() == strings.size());
    REQUIRE(forwardMap.size() == nextIndex);
    return std::tuple(strings, ints);
}

using ZStr = zoo::Str<void *>;

namespace zoo {

template<typename StoragePrototype>
bool operator<(
    const Str<StoragePrototype> &left,
    const Str<StoragePrototype> &right
) {
    auto lS = left.size(), rS = right.size();
    auto minLength = lS < rS ? lS : rS;
    auto preRV = memcmp(left.c_str(), right.c_str(), minLength);
    return preRV < 0 || (0 == preRV && lS < rS);
}

}

TEST_CASE("Str benchmarks") {
    auto [strings, integers]  = loadTextCorpus(
        "/tmp/deleteme/TheTurnOfTheScrew.txt.lowercase.txt"
    );
    std::vector<ZStr> zss;
    for(auto &source: strings) {
        zss.push_back({source.data(), source.size() + 1});
    }
    REQUIRE(strings.size() == integers.size());
    REQUIRE(zss.size() == integers.size());
    auto buildHistogram =
        [](auto &histogram, const auto &events) {
            for(auto &event: events) {
                ++histogram[event];
            }
        };
    auto &strs = strings;
    auto &ints = integers;
    
    std::map<ZStr, int> zooH;
    std::map<int, int> intH;
    std::map<std::string, int> stdH;
    
    auto hSize = [&](auto &h, auto &series) {
        h.clear();
        buildHistogram(h, series);
        return h.size();
    };
    auto iS = hSize(intH, ints);
    REQUIRE(hSize(zooH, zss) == iS);

    BENCHMARK("zoo::Str<void *>") {
        zooH.clear();
        buildHistogram(zooH, zss);
        return zooH.size();
    };
    BENCHMARK("Baseline") {
        intH.clear();
        buildHistogram(intH, ints);
        return intH.size();
    };
    BENCHMARK("std::string") {
        stdH.clear();
        buildHistogram(stdH, strs);
        return stdH.size();
    };
    BENCHMARK("zoo::Str<void *> 2") {
        zooH.clear();
        buildHistogram(zooH, zss);
        return zooH.size();
    };
    std::map<zoo::Str<void *[2]>, int> zooTwoPointersH;
    std::vector<zoo::Str<void *[2]>> twoPSeries;
    for(auto &source: strings) {
        twoPSeries.push_back({ source.data(), source.size() + 1 });
    }
    BENCHMARK("zoo::Str<void *[2]>") {
        zooTwoPointersH.clear();
        buildHistogram(zooTwoPointersH, twoPSeries);
        return zooTwoPointersH.size();
    };

    std::map<zoo::Str<void *[8]>, int> zooTwoPointersH8;
    std::vector<zoo::Str<void *[8]>> twoPSeries8;
    for(auto &source: strings) {
        twoPSeries8.push_back({ source.data(), source.size() + 1 });
    }
    BENCHMARK("zoo::Str<void *[8]>") {
        zooTwoPointersH8.clear();
        buildHistogram(zooTwoPointersH8, twoPSeries8);
        return zooTwoPointersH8.size();
    };

}
