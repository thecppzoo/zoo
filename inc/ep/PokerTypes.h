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

constexpr auto SuitSize = 1 << core::metaLogCeiling(NRanks);
constexpr auto RankSize = 1 << core::metaLogCeiling(NSuits);

using SWARSuit = core::SWAR<SuitSize>;
using SWARRank = core::SWAR<RankSize>;

constexpr auto NCommunity = 5;
constexpr auto NPlayer = 2;
constexpr auto TotalHand = NCommunity + NPlayer;

}
