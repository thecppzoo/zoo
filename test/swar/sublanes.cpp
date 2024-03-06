#include "zoo/swar/SWARWithSublanes.h"

using namespace zoo;
using namespace zoo::swar;

// 3 bits on msb side, 5 bits on lsb side.
using Lanes = SWARWithSubLanes<5, 3, u32>;
using S8u32 = SWAR<8, u32>;
static constexpr inline u32 all0 = 0;
static constexpr inline u32 allF = broadcast<8>(S8u32(0x0000'00FFul)).value();

static_assert(allF == Lanes(allF).value());
static_assert(0xFFFF'FFFF == Lanes(allF).value());

static_assert(0xFFFF'FFE0 == Lanes(allF).least(0,0).value());
static_assert(0xFFFF'FFE1 == Lanes(allF).least(1,0).value());
static_assert(0xFFFF'E0FF == Lanes(allF).least(0,1).value());
static_assert(0xFFFF'E1FF == Lanes(allF).least(1,1).value());

static_assert(0xFFE0'FFFF == Lanes(allF).least(0,2).value());
static_assert(0xFFE1'FFFF == Lanes(allF).least(1,2).value());
static_assert(0xE0FF'FFFF == Lanes(allF).least(0,3).value());
static_assert(0xE1FF'FFFF == Lanes(allF).least(1,3).value());

static_assert(0xFFFF'FF1F == Lanes(allF).most(0,0).value());
static_assert(0xFFFF'FF3F == Lanes(allF).most(1,0).value());
static_assert(0xFFFF'1FFF == Lanes(allF).most(0,1).value());
static_assert(0xFFFF'3FFF == Lanes(allF).most(1,1).value());

static_assert(0xFF1F'FFFF == Lanes(allF).most(0,2).value());
static_assert(0xFF3F'FFFF == Lanes(allF).most(1,2).value());
static_assert(0x1FFF'FFFF == Lanes(allF).most(0,3).value());
static_assert(0x3FFF'FFFF == Lanes(allF).most(1,3).value());

static_assert(0x0000'001f == Lanes(all0).least(31, 0).most(0, 0).value());
static_assert(0x0000'1f00 == Lanes(all0).least(31, 1).most(0, 1).value());
static_assert(0x001f'0000 == Lanes(all0).least(31, 2).most(0, 2).value());
static_assert(0x1f00'0000 == Lanes(all0).least(31, 3).most(0, 3).value());

static_assert(0x0000'00e0 == Lanes(all0).least(0, 0).most(31, 0).value());
static_assert(0x0000'e000 == Lanes(all0).least(0, 1).most(31, 1).value());
static_assert(0x00e0'0000 == Lanes(all0).least(0, 2).most(31, 2).value());
static_assert(0xe000'0000 == Lanes(all0).least(0, 3).most(31, 3).value());

static_assert(0x1F1F'1F1F == Lanes(allF).least().value());
static_assert(0xE0E0'E0E0 == Lanes(allF).most().value());

static_assert(0x0000'001F == Lanes(allF).least(0).value());
static_assert(0x0000'1F00 == Lanes(allF).least(1).value());
static_assert(0x001F'0000 == Lanes(allF).least(2).value());
static_assert(0x1F00'0000 == Lanes(allF).least(3).value());

static_assert(0x0000'00E0 == Lanes(allF).most(0).value());
static_assert(0x0000'E000 == Lanes(allF).most(1).value());
static_assert(0x00E0'0000 == Lanes(allF).most(2).value());
static_assert(0xE000'0000 == Lanes(allF).most(3).value());
