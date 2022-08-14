#pragma once

#include "zoo/swar/SWAR.h"

#include <vector>

namespace zoo {

namespace rh {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

template<int NBits>
constexpr auto cheapOkHash(u64 n) noexcept {
    constexpr auto shift = (NBits * ((64 / NBits)-1));
    constexpr u64 allOnes = meta::BitmaskMaker<u64, 1, NBits>::value;
    auto temporary = allOnes * n;
    auto higestNBits = temporary >> shift;
    return (0 == (64 % NBits)) ? 
        higestNBits : swar::isolate<NBits, u64>(higestNBits);
}

template<typename Key>auto fibhash(Key k) noexcept {
    constexpr auto FibonacciGoldenRatioReciprocal64 = 11400714819323198485ull;
    constexpr auto FibonacciGoldenRatioReciprocal32 = 2654435769u;
    if constexpr(sizeof(Key) == 8) {
        return k * FibonacciGoldenRatioReciprocal64;
    } else if constexpr(sizeof(Key) == 4){
        return k * 2654435769u;
    }
    return 1;
}

template<typename Key, int NBits> auto badMixer(Key k) noexcept {
  constexpr Key allOnes = meta::BitmaskMaker<Key, 1, sizeof(Key)>::value;
  constexpr Key mostSigNBits = swar::mostNBitsMask<NBits, Key>();
  auto tmp = k * allOnes;
  
  auto mostSigBits = tmp >> mostSigNBits;
  return mostSigBits >> (sizeof(Key) - NBits);
}

template<typename Key>auto reducedhashUnitary(Key k) noexcept {
  return 1;
}

template<typename Key>auto slotFromKeyUnitary(Key k) noexcept {
  return 1;
}

}}
