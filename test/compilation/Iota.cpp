#include "zoo/remedies-17/iota.h"

struct ConstexprVector {
    int data[100];
    std::size_t sz;

    constexpr ConstexprVector(): data{}, sz(0) {}

    constexpr void resize(std::size_t size) { sz = size; }

    constexpr int *begin() { return data; }
    constexpr int *end() { return data + sz; }
};

constexpr auto
JustCompilationSuffices = zoo::Iota<ConstexprVector, 5>::value;
