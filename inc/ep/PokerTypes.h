#pragma once

#include "core/SWAR.h"

namespace ep {

/// Number of numbers in the card set
constexpr auto NRanks = 13;
/// Number of suits
constexpr auto NSuits = 4;

namespace canonical {

constexpr auto NRanks = 13;
constexpr auto NSuits = 4;

};

constexpr auto SuitBits = 1 << core::metaLogCeiling(NRanks);
constexpr auto RankBits = 1 << core::metaLogCeiling(NSuits);

using SWARSuit = core::SWAR<SuitBits>;
using SWARRank = core::SWAR<RankBits>;

}
