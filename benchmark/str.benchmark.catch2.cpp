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

template<typename STR>
auto allocationPtr(const STR &c) {
    return c.c_str();
}
template<typename StoragePrototype>
auto allocationPtr(const zoo::Str<StoragePrototype> &c) {
    return c.allocationPtr();
}

template<typename STR>
auto onHeap(const STR &c) {
    auto cStr = c.c_str();
    uintptr_t asInteger;
    memcpy(&asInteger, &cStr, sizeof(char *));
    constexpr auto StringTypeSize = sizeof(STR);
    auto baseAddress = &c;
    uintptr_t baseAddressAsInteger;
    memcpy(&baseAddressAsInteger, &baseAddress, sizeof(char *));
    if(
        baseAddressAsInteger <= asInteger &&
        asInteger < baseAddressAsInteger + StringTypeSize
    ) { return false; }
    return true;
}

template<typename StoragePrototype>
auto onHeap(const zoo::Str<StoragePrototype> &c) {
    return c.onHeap();
}

template<typename>
struct IsZooStr: std::false_type {};
template<typename SP>
struct IsZooStr<zoo::Str<SP>>: std::true_type {};

static_assert(IsZooStr<zoo::Str<void *[2]>>::value);
static_assert(!IsZooStr<std::string>::value);

template<typename STR>
auto minimumUsedBytes(const STR &s) {
    if(!onHeap(s)) {
        return sizeof(STR);
    }
    auto where = allocationPtr(s);
    uintptr_t asInt;
    memcpy(&asInt, &where, sizeof(char *));
    auto trailingZeroes = __builtin_ctzll(asInt);
    auto impliedAllocationSize = 1 << trailingZeroes;
    auto roundSizeToAllocationSizeMask = impliedAllocationSize - 1;
    auto size = s.size() + 1;
    if constexpr(IsZooStr<STR>::value) {
        size += zoo::Log256Celing(size);
    }
    // example, allocation size is 64, the mask is then 0b1.1111 (5 bits)
    // and size is 17: that's 0b1.0001, the bytes 18 to 63 are not used,
    // so, the bytes that this code takes are 63
    return (size | roundSizeToAllocationSizeMask) + sizeof(STR);
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

#include <sstream>
// by ChatGPT
std::string toEngineeringString(double value, int precision) {
    if (value == 0.0) {
        return "0.0";
    }

    int exponent = static_cast<int>(std::floor(std::log10(std::abs(value))));
    int engineeringExponent = exponent - (exponent % 3); // Align to multiple of 3
    double scaledValue = value / std::pow(10, engineeringExponent);

    // Use a stringstream for formatting
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision);
    oss << scaledValue << "e" << engineeringExponent;

    return oss.str();
}

TEST_CASE("Efficiency counters") {
    auto [strs, _] = loadTextCorpus(
        "/tmp/deleteme/TheTurnOfTheScrew.txt.lowercase.txt"
    );
    constexpr char LongString[] =
        "A very long string, contents don't matter, just size";
    SECTION("minimum tests") {
        REQUIRE(sizeof(std::string) == minimumUsedBytes(std::string("Hola")));
        REQUIRE(sizeof(std::string) < minimumUsedBytes(std::string(LongString)));
    }
    strs.push_back(LongString);

    auto process = [](auto &strings) {
        auto
            allocationCount = 0,
            significantBytes = 0,
            totalBytesCommitted = 0;
        auto stringCount = strings.size() + 1;
        for(auto &s: strings) {
            if(onHeap(s)) { ++allocationCount; }
            significantBytes += s.size();
            totalBytesCommitted += minimumUsedBytes(s);
        }
        auto average = [stringCount](double v) {
            return toEngineeringString(v / stringCount, 3);
        };
        WARN(
sizeof(typename std::remove_reference_t<decltype(strings)>::value_type) << ' ' << typeid(decltype(strings)).name() <<
"\nCount: " << stringCount <<
"\nAllocations: " << allocationCount <<
"\nSignificant Bytes: " << significantBytes <<
"\nTotalBytes: " << totalBytesCommitted <<
"\nEfficiency: " <<
    toEngineeringString(significantBytes/double(totalBytesCommitted), 3) <<
"\nAverages (allocations, size)" <<
    average(allocationCount) << ' ' << average(significantBytes) <<
"\n"
        );
    };
    SECTION("std") {
        process(strs);
    }

    #define QUOTE(a) #a
    #define STRINGIFY(a) QUOTE(a)
    #define STORAGE_PROTOTYPE_X_LIST \
        X(void *)\
        X(void *[2])\
        X(void *[3])\
        X(void *[4])
    #define X(prototype) \
        SECTION("zoo::Str<" STRINGIFY(prototype) ">") { \
            std::vector<zoo::Str<prototype>> zoos; \
            for(auto &s: strs) { zoos.emplace_back(s.c_str(), s.size() + 1); } \
            process(zoos); \
        }
    STORAGE_PROTOTYPE_X_LIST
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
