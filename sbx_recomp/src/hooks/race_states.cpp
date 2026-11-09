// The race states as our own C++ (--native_race_states, on by default):
// the race state machine's simple handlers (its step cleared, the fades,
// quit, unload, the standing start, begin), the race progress' reset and
// a racer's time count, and the route helpers every racer's progress goes
// through each tick (a time's difference, whether a route position isn't
// after another, a section before or beside another).
//
// Written from SBX's generated code. The ticks themselves (rolling and
// standing starts, the countdown, the race, after the race, quit), the
// progress' route following, laps, checks and finish, the ranking and the
// machine's set-up stay recompiled.
//
// The route's sections: [[0x826B57F0] + 12] + 4 x section, each with its
// links' signed halfwords at +28 + 4 x link (links 0 to 2 the ones ahead,
// 6 to 9 the ones beside).

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_states, true, "SBX",
                    "Our own C++ for the race states' simple handlers, the progress' reset and "
                    "the route helpers each racer's progress uses every tick, instead of the "
                    "recompiled functions; the race's ticks stay recompiled");

namespace {

using guest_memory::Fill;
using guest_memory::kAbsByte;
using guest_memory::kAbsHalf;
using guest_memory::kAbsImage;
using guest_memory::kAbsWord;
using guest_memory::kArg;
using guest_memory::kByte;
using guest_memory::kHalf;
using guest_memory::kImage;
using guest_memory::kSelf;
using guest_memory::kWord;

// RaceStates::ClearStep (sub_82161218).
// 1 stores
constexpr Fill kRaceStates_ClearStep0[] = {
    {9608, kWord, 0x0},
};

// RaceStates::OnQuit (sub_82161238).
// 1 stores
constexpr Fill kRaceStates_OnQuit0[] = {
    {9612, kWord, 0x0},
};

// RaceStates::FadeInFree (sub_82161248).
// 3 stores
constexpr Fill kRaceStates_FadeInFree0[] = {
    {9616, kWord, 0x0, 2, 4}, {9612, kWord, 0x0},
};

// RaceStates::OnStandingStart (sub_82162280).
// 8 stores
constexpr Fill kRaceStates_OnStandingStart0[] = {
    {9620, kWord, 0x0, 2, 8}, {9632, kWord, 0x0}, {9612, kWord, 0x0, 2, 4}, {9636, kWord, 0x0, 2, 4},
    {0x826B65D0, kAbsWord, 0x0},
};

// RaceProgress::Reset (sub_821627B0).
// 564 stores
constexpr Fill kRaceProgress_Reset0[] = {
    {40, kWord, 0x0, 3, 4}, {564, kWord, 0x0}, {572, kHalf, 0xFFFF}, {568, kWord, 0x2},
    {574, kHalf, 0xFFFF}, {576, kWord, 0x0, 3, 4}, {1100, kWord, 0x0}, {1108, kHalf, 0xFFFF},
    {1104, kWord, 0x2}, {1110, kHalf, 0xFFFF}, {1112, kWord, 0x0, 3, 4}, {1636, kWord, 0x0},
    {1644, kHalf, 0xFFFF}, {1640, kWord, 0x2}, {1646, kHalf, 0xFFFF}, {1648, kWord, 0x0, 3, 4},
    {2172, kWord, 0x0}, {2180, kHalf, 0xFFFF}, {2176, kWord, 0x2}, {2182, kHalf, 0xFFFF, 2, 3938},
    {6636, kHalf, 0xFFFF}, {6122, kHalf, 0xFFFF, 2, 516}, {6124, kHalf, 0xFFFF, 2, 516}, {6126, kHalf, 0xFFFF, 2, 516},
    {6128, kHalf, 0xFFFF, 2, 516}, {6130, kHalf, 0xFFFF, 2, 516}, {6132, kHalf, 0xFFFF, 2, 516}, {6134, kHalf, 0xFFFF, 2, 516},
    {6136, kHalf, 0xFFFF, 2, 516}, {6138, kHalf, 0xFFFF, 2, 516}, {6140, kHalf, 0xFFFF, 2, 516}, {6142, kHalf, 0xFFFF, 2, 516},
    {6144, kHalf, 0xFFFF, 2, 516}, {6146, kHalf, 0xFFFF, 2, 516}, {6148, kHalf, 0xFFFF, 2, 516}, {6150, kHalf, 0xFFFF, 2, 516},
    {6152, kHalf, 0xFFFF, 2, 516}, {6154, kHalf, 0xFFFF, 2, 516}, {6156, kHalf, 0xFFFF, 2, 516}, {6158, kHalf, 0xFFFF, 2, 516},
    {6160, kHalf, 0xFFFF, 2, 516}, {6162, kHalf, 0xFFFF, 2, 516}, {6164, kHalf, 0xFFFF, 2, 516}, {6166, kHalf, 0xFFFF, 2, 516},
    {6168, kHalf, 0xFFFF, 2, 516}, {6170, kHalf, 0xFFFF, 2, 516}, {6172, kHalf, 0xFFFF, 2, 516}, {6174, kHalf, 0xFFFF, 2, 516},
    {6176, kHalf, 0xFFFF, 2, 516}, {6178, kHalf, 0xFFFF, 2, 516}, {6180, kHalf, 0xFFFF, 2, 516}, {6182, kHalf, 0xFFFF, 2, 516},
    {6184, kHalf, 0xFFFF, 2, 516}, {6186, kHalf, 0xFFFF, 2, 516}, {6188, kHalf, 0xFFFF, 2, 516}, {6190, kHalf, 0xFFFF, 2, 516},
    {6192, kHalf, 0xFFFF, 2, 516}, {6194, kHalf, 0xFFFF, 2, 516}, {6196, kHalf, 0xFFFF, 2, 516}, {6198, kHalf, 0xFFFF, 2, 516},
    {6200, kHalf, 0xFFFF, 2, 516}, {6202, kHalf, 0xFFFF, 2, 516}, {6204, kHalf, 0xFFFF, 2, 516}, {6206, kHalf, 0xFFFF, 2, 516},
    {6208, kHalf, 0xFFFF, 2, 516}, {6210, kHalf, 0xFFFF, 2, 516}, {6212, kHalf, 0xFFFF, 2, 516}, {6214, kHalf, 0xFFFF, 2, 516},
    {6216, kHalf, 0xFFFF, 2, 516}, {6218, kHalf, 0xFFFF, 2, 516}, {6220, kHalf, 0xFFFF, 2, 516}, {6222, kHalf, 0xFFFF, 2, 516},
    {6224, kHalf, 0xFFFF, 2, 516}, {6226, kHalf, 0xFFFF, 2, 516}, {6228, kHalf, 0xFFFF, 2, 516}, {6230, kHalf, 0xFFFF, 2, 516},
    {6232, kHalf, 0xFFFF, 2, 516}, {6234, kHalf, 0xFFFF, 2, 516}, {6236, kHalf, 0xFFFF, 2, 516}, {6238, kHalf, 0xFFFF, 2, 516},
    {6240, kHalf, 0xFFFF, 2, 516}, {6242, kHalf, 0xFFFF, 2, 516}, {6244, kHalf, 0xFFFF, 2, 516}, {6246, kHalf, 0xFFFF, 2, 516},
    {6248, kHalf, 0xFFFF, 2, 516}, {6250, kHalf, 0xFFFF, 2, 516}, {6252, kHalf, 0xFFFF, 2, 516}, {6254, kHalf, 0xFFFF, 2, 516},
    {6256, kHalf, 0xFFFF, 2, 516}, {6258, kHalf, 0xFFFF, 2, 516}, {6260, kHalf, 0xFFFF, 2, 516}, {6262, kHalf, 0xFFFF, 2, 516},
    {6264, kHalf, 0xFFFF, 2, 516}, {6266, kHalf, 0xFFFF, 2, 516}, {6268, kHalf, 0xFFFF, 2, 516}, {6270, kHalf, 0xFFFF, 2, 516},
    {6272, kHalf, 0xFFFF, 2, 516}, {6274, kHalf, 0xFFFF, 2, 516}, {6276, kHalf, 0xFFFF, 2, 516}, {6278, kHalf, 0xFFFF, 2, 516},
    {6280, kHalf, 0xFFFF, 2, 516}, {6282, kHalf, 0xFFFF, 2, 516}, {6284, kHalf, 0xFFFF, 2, 516}, {6286, kHalf, 0xFFFF, 2, 516},
    {6288, kHalf, 0xFFFF, 2, 516}, {6290, kHalf, 0xFFFF, 2, 516}, {6292, kHalf, 0xFFFF, 2, 516}, {6294, kHalf, 0xFFFF, 2, 516},
    {6296, kHalf, 0xFFFF, 2, 516}, {6298, kHalf, 0xFFFF, 2, 516}, {6300, kHalf, 0xFFFF, 2, 516}, {6302, kHalf, 0xFFFF, 2, 516},
    {6304, kHalf, 0xFFFF, 2, 516}, {6306, kHalf, 0xFFFF, 2, 516}, {6308, kHalf, 0xFFFF, 2, 516}, {6310, kHalf, 0xFFFF, 2, 516},
    {6312, kHalf, 0xFFFF, 2, 516}, {6314, kHalf, 0xFFFF, 2, 516}, {6316, kHalf, 0xFFFF, 2, 516}, {6318, kHalf, 0xFFFF, 2, 516},
    {6320, kHalf, 0xFFFF, 2, 516}, {6322, kHalf, 0xFFFF, 2, 516}, {6324, kHalf, 0xFFFF, 2, 516}, {6326, kHalf, 0xFFFF, 2, 516},
    {6328, kHalf, 0xFFFF, 2, 516}, {6330, kHalf, 0xFFFF, 2, 516}, {6332, kHalf, 0xFFFF, 2, 516}, {6334, kHalf, 0xFFFF, 2, 516},
    {6336, kHalf, 0xFFFF, 2, 516}, {6338, kHalf, 0xFFFF, 2, 516}, {6340, kHalf, 0xFFFF, 2, 516}, {6342, kHalf, 0xFFFF, 2, 516},
    {6344, kHalf, 0xFFFF, 2, 516}, {6346, kHalf, 0xFFFF, 2, 516}, {6348, kHalf, 0xFFFF, 2, 516}, {6350, kHalf, 0xFFFF, 2, 516},
    {6352, kHalf, 0xFFFF, 2, 516}, {6354, kHalf, 0xFFFF, 2, 516}, {6356, kHalf, 0xFFFF, 2, 516}, {6358, kHalf, 0xFFFF, 2, 516},
    {6360, kHalf, 0xFFFF, 2, 516}, {6362, kHalf, 0xFFFF, 2, 516}, {6364, kHalf, 0xFFFF, 2, 516}, {6366, kHalf, 0xFFFF, 2, 516},
    {6368, kHalf, 0xFFFF, 2, 516}, {6370, kHalf, 0xFFFF, 2, 516}, {6372, kHalf, 0xFFFF, 2, 516}, {6374, kHalf, 0xFFFF, 2, 516},
    {6376, kHalf, 0xFFFF, 2, 516}, {6378, kHalf, 0xFFFF, 2, 516}, {6380, kHalf, 0xFFFF, 2, 516}, {6382, kHalf, 0xFFFF, 2, 516},
    {6384, kHalf, 0xFFFF, 2, 516}, {6386, kHalf, 0xFFFF, 2, 516}, {6388, kHalf, 0xFFFF, 2, 516}, {6390, kHalf, 0xFFFF, 2, 516},
    {6392, kHalf, 0xFFFF, 2, 516}, {6394, kHalf, 0xFFFF, 2, 516}, {6396, kHalf, 0xFFFF, 2, 516}, {6398, kHalf, 0xFFFF, 2, 516},
    {6400, kHalf, 0xFFFF, 2, 516}, {6402, kHalf, 0xFFFF, 2, 516}, {6404, kHalf, 0xFFFF, 2, 516}, {6406, kHalf, 0xFFFF, 2, 516},
    {6408, kHalf, 0xFFFF, 2, 516}, {6410, kHalf, 0xFFFF, 2, 516}, {6412, kHalf, 0xFFFF, 2, 516}, {6414, kHalf, 0xFFFF, 2, 516},
    {6416, kHalf, 0xFFFF, 2, 516}, {6418, kHalf, 0xFFFF, 2, 516}, {6420, kHalf, 0xFFFF, 2, 516}, {6422, kHalf, 0xFFFF, 2, 516},
    {6424, kHalf, 0xFFFF, 2, 516}, {6426, kHalf, 0xFFFF, 2, 516}, {6428, kHalf, 0xFFFF, 2, 516}, {6430, kHalf, 0xFFFF, 2, 516},
    {6432, kHalf, 0xFFFF, 2, 516}, {6434, kHalf, 0xFFFF, 2, 516}, {6436, kHalf, 0xFFFF, 2, 516}, {6438, kHalf, 0xFFFF, 2, 516},
    {6440, kHalf, 0xFFFF, 2, 516}, {6442, kHalf, 0xFFFF, 2, 516}, {6444, kHalf, 0xFFFF, 2, 516}, {6446, kHalf, 0xFFFF, 2, 516},
    {6448, kHalf, 0xFFFF, 2, 516}, {6450, kHalf, 0xFFFF, 2, 516}, {6452, kHalf, 0xFFFF, 2, 516}, {6454, kHalf, 0xFFFF, 2, 516},
    {6456, kHalf, 0xFFFF, 2, 516}, {6458, kHalf, 0xFFFF, 2, 516}, {6460, kHalf, 0xFFFF, 2, 516}, {6462, kHalf, 0xFFFF, 2, 516},
    {6464, kHalf, 0xFFFF, 2, 516}, {6466, kHalf, 0xFFFF, 2, 516}, {6468, kHalf, 0xFFFF, 2, 516}, {6470, kHalf, 0xFFFF, 2, 516},
    {6472, kHalf, 0xFFFF, 2, 516}, {6474, kHalf, 0xFFFF, 2, 516}, {6476, kHalf, 0xFFFF, 2, 516}, {6478, kHalf, 0xFFFF, 2, 516},
    {6480, kHalf, 0xFFFF, 2, 516}, {6482, kHalf, 0xFFFF, 2, 516}, {6484, kHalf, 0xFFFF, 2, 516}, {6486, kHalf, 0xFFFF, 2, 516},
    {6488, kHalf, 0xFFFF, 2, 516}, {6490, kHalf, 0xFFFF, 2, 516}, {6492, kHalf, 0xFFFF, 2, 516}, {6494, kHalf, 0xFFFF, 2, 516},
    {6496, kHalf, 0xFFFF, 2, 516}, {6498, kHalf, 0xFFFF, 2, 516}, {6500, kHalf, 0xFFFF, 2, 516}, {6502, kHalf, 0xFFFF, 2, 516},
    {6504, kHalf, 0xFFFF, 2, 516}, {6506, kHalf, 0xFFFF, 2, 516}, {6508, kHalf, 0xFFFF, 2, 516}, {6510, kHalf, 0xFFFF, 2, 516},
    {6512, kHalf, 0xFFFF, 2, 516}, {6514, kHalf, 0xFFFF, 2, 516}, {6516, kHalf, 0xFFFF, 2, 516}, {6518, kHalf, 0xFFFF, 2, 516},
    {6520, kHalf, 0xFFFF, 2, 516}, {6522, kHalf, 0xFFFF, 2, 516}, {6524, kHalf, 0xFFFF, 2, 516}, {6526, kHalf, 0xFFFF, 2, 516},
    {6528, kHalf, 0xFFFF, 2, 516}, {6530, kHalf, 0xFFFF, 2, 516}, {6532, kHalf, 0xFFFF, 2, 516}, {6534, kHalf, 0xFFFF, 2, 516},
    {6536, kHalf, 0xFFFF, 2, 516}, {6538, kHalf, 0xFFFF, 2, 516}, {6540, kHalf, 0xFFFF, 2, 516}, {6542, kHalf, 0xFFFF, 2, 516},
    {6544, kHalf, 0xFFFF, 2, 516}, {6546, kHalf, 0xFFFF, 2, 516}, {6548, kHalf, 0xFFFF, 2, 516}, {6550, kHalf, 0xFFFF, 2, 516},
    {6552, kHalf, 0xFFFF, 2, 516}, {6554, kHalf, 0xFFFF, 2, 516}, {6556, kHalf, 0xFFFF, 2, 516}, {6558, kHalf, 0xFFFF, 2, 516},
    {6560, kHalf, 0xFFFF, 2, 516}, {6562, kHalf, 0xFFFF, 2, 516}, {6564, kHalf, 0xFFFF, 2, 516}, {6566, kHalf, 0xFFFF, 2, 516},
    {6568, kHalf, 0xFFFF, 2, 516}, {6570, kHalf, 0xFFFF, 2, 516}, {6572, kHalf, 0xFFFF, 2, 516}, {6574, kHalf, 0xFFFF, 2, 516},
    {6576, kHalf, 0xFFFF, 2, 516}, {6578, kHalf, 0xFFFF, 2, 516}, {6580, kHalf, 0xFFFF, 2, 516}, {6582, kHalf, 0xFFFF, 2, 516},
    {6584, kHalf, 0xFFFF, 2, 516}, {6586, kHalf, 0xFFFF, 2, 516}, {6588, kHalf, 0xFFFF, 2, 516}, {6590, kHalf, 0xFFFF, 2, 516},
    {6592, kHalf, 0xFFFF, 2, 516}, {6594, kHalf, 0xFFFF, 2, 516}, {6596, kHalf, 0xFFFF, 2, 516}, {6598, kHalf, 0xFFFF, 2, 516},
    {6600, kHalf, 0xFFFF, 2, 516}, {6602, kHalf, 0xFFFF, 2, 516}, {6604, kHalf, 0xFFFF, 2, 516}, {6606, kHalf, 0xFFFF, 2, 516},
    {6608, kHalf, 0xFFFF, 2, 516}, {6610, kHalf, 0xFFFF, 2, 516}, {6612, kHalf, 0xFFFF, 2, 516}, {6614, kHalf, 0xFFFF, 2, 516},
    {6616, kHalf, 0xFFFF, 2, 516}, {6618, kHalf, 0xFFFF, 2, 516}, {6620, kHalf, 0xFFFF, 2, 516}, {6622, kHalf, 0xFFFF, 2, 516},
    {6624, kHalf, 0xFFFF, 2, 516}, {6626, kHalf, 0xFFFF, 2, 516}, {6628, kHalf, 0xFFFF, 2, 516}, {6630, kHalf, 0xFFFF, 2, 516},
    {6632, kWord, 0x0, 2, 516}, {7156, kImage, 0x82000A24}, {7160, kWord, 0x0, 15, 4}, {7228, kWord, 0x0},
    {7220, kWord, 0x0, 2, 4}, {7240, kWord, 0xFFFFFFFF}, {7232, kImage, 0x82000A20, 2, 4},
};

// RaceStates::Begin (sub_82162C90).
// 3 stores
constexpr Fill kRaceStates_Begin0[] = {
    {9616, kWord, 0x0, 2, 4}, {9612, kWord, 0x0},
};
// 4 stores
constexpr Fill kRaceStates_Begin1[] = {
    {9628, kWord, 0x0, 4, 4},
};

bool On() { return REXCVAR_GET(native_race_states); }

using guest::LoadU32;
using guest::StoreU32;

constexpr uint32_t kRoute = 0x826B57F0;  // [it] + 12: the sections

// A section's record.
uint32_t Section(uint8_t* base, uint32_t section) {
  return LoadU32(base, LoadU32(base, LoadU32(base, kRoute) + 12) + ((section & 0xFFFF) << 2));
}

// A section's link (a signed halfword).
int32_t Link(uint8_t* base, uint32_t record, uint32_t link) {
  return int16_t(guest::LoadU16(base, record + 4 * (link + 7)));
}

constexpr uint32_t kAhead[] = {0, 1, 2};
constexpr uint32_t kBeside[] = {6, 7, 8, 9};

// A time word's milliseconds: bit 31 the sign, then the minutes (15 bits
// at 16), the seconds (6 bits at 10) and the milliseconds (10 bits).
int32_t Milliseconds(uint32_t time) {
  const int32_t total = int32_t((((time >> 16) & 0x7FFF) * 60 + ((time >> 10) & 0x3F)) * 1000 +
                                (time & 0x3FF));
  return (time & 0x80000000u) ? -total : total;
}

}  // namespace

// RaceStates::ClearStep.
extern "C" REX_FUNC(sub_82161218) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82161218(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kRaceStates_ClearStep0, args);
  ctx.r3.u64 = object;
}

// RaceStates::OnQuit.
extern "C" REX_FUNC(sub_82161238) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82161238(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kRaceStates_OnQuit0, args);
  ctx.r3.u64 = object;
}

// RaceStates::FadeInFree.
extern "C" REX_FUNC(sub_82161248) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82161248(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kRaceStates_FadeInFree0, args);
  ctx.r3.u64 = guest::LoadU32(base, 0x82001BE4);
  ctx.r4.u64 = 0x0u;
  ctx.r5.u64 = 0x0u;
  ctx.r6.u64 = 0x0u;
  ctx.r7.u64 = 0x0u;
  sub_8209D158(ctx, base);
}

// RaceStates::FadeOut.
extern "C" REX_FUNC(sub_82161278) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82161278(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = guest::LoadU32(base, 0x82001BE8);
  ctx.r4.u64 = 0x0u;
  ctx.r5.u64 = 0x0u;
  ctx.r6.u64 = 0x0u;
  ctx.r7.u64 = 0x0u;
  sub_8209D1E8(ctx, base);
}

// RaceStates::Unload.
extern "C" REX_FUNC(sub_821617F0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821617F0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = guest::LoadU32(base, 0x826C7D90);
  sub_8258C868(ctx, base);
}

// RaceStates::OnStandingStart.
extern "C" REX_FUNC(sub_82162280) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82162280(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kRaceStates_OnStandingStart0, args);
  ctx.r3.u64 = object + 0;
  sub_82139350(ctx, base);
}

// RaceProgress::Reset.
extern "C" REX_FUNC(sub_821627B0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821627B0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 2184;
  sub_821020F0(ctx, base);
  guest_memory::Apply(base, object, kRaceProgress_Reset0, args);
}

// RaceStates::Begin.
extern "C" REX_FUNC(sub_82162C90) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82162C90(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kRaceStates_Begin0, args);
  ctx.r3.u64 = guest::LoadU32(base, object + 9624);
  sub_821627B0(ctx, base);
  guest_memory::Apply(base, object, kRaceStates_Begin1, args);
}


// Time::Sub: [r3] - [r4] in milliseconds, made a time word again
// (sub_82101850, a tail call; r4 as the original leaves it: -1 for a
// negative [r4], else 0).
extern "C" REX_FUNC(sub_821600F8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821600F8(ctx, base);
    return;
  }
  const uint32_t a = LoadU32(base, ctx.r3.u32), b = LoadU32(base, ctx.r4.u32);
  ctx.r3.u64 = uint64_t(int64_t(Milliseconds(a) - Milliseconds(b)));
  ctx.r4.u64 = (b & 0x80000000u) ? 0xFFFFFFFFFFFFFFFFull : 0;
  sub_82101850(ctx, base);
}

// RoutePosition::NotAfter: 1 when r3's position (+0 the section, +2 the
// offset, signed halfwords) isn't after r4's.
extern "C" REX_FUNC(sub_82160170) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82160170(ctx, base);
    return;
  }
  const int16_t a = int16_t(guest::LoadU16(base, ctx.r3.u32));
  const int16_t b = int16_t(guest::LoadU16(base, ctx.r4.u32));
  const int16_t a_offset = int16_t(guest::LoadU16(base, ctx.r3.u32 + 2));
  const int16_t b_offset = int16_t(guest::LoadU16(base, ctx.r4.u32 + 2));
  ctx.r3.u64 = (a < b || (a == b && a_offset <= b_offset)) ? 1 : 0;
}

// RaceProgress::CountTime: r3 the result (two words cleared, returned), r4
// the progress, r5 the racer: the racer's count (+2200 of its 972-byte
// record) up by one and its time (+2204) from it (sub_821018B0).
extern "C" REX_FUNC(sub_82160CB0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82160CB0(ctx, base);
    return;
  }
  const uint32_t out = ctx.r3.u32;
  const uint32_t record = ctx.r4.u32 + 972 * ctx.r5.u32;
  const uint32_t count = LoadU32(base, record + 2200) + 1;
  StoreU32(base, record + 2200, count);
  ctx.r3.u64 = count;
  sub_821018B0(ctx, base);
  StoreU32(base, record + 2204, ctx.r3.u32);
  StoreU32(base, out + 0, 0);
  StoreU32(base, out + 4, 0);
  ctx.r3.u64 = out;
}

// RouteSections::Before: r3 a section, r4 another. r3 when one of r3's
// links ahead (0 to 2) is r4; else the first of its links beside (6 to 9)
// whose own links ahead hold r4; else -1.
extern "C" REX_FUNC(sub_82160D18) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82160D18(ctx, base);
    return;
  }
  const int32_t target = int32_t(ctx.r4.u32);
  const uint32_t record = Section(base, ctx.r3.u32);
  for (uint32_t link : kAhead) {
    if (Link(base, record, link) == target) {
      return;
    }
  }
  for (uint32_t beside : kBeside) {
    const int32_t next = Link(base, record, beside);
    if (next == -1) {
      continue;
    }
    const uint32_t next_record = Section(base, uint32_t(next));
    for (uint32_t link : kAhead) {
      if (Link(base, next_record, link) == target) {
        ctx.r3.u64 = uint64_t(int64_t(next));
        return;
      }
    }
  }
  ctx.r3.u64 = 0xFFFFFFFFFFFFFFFFull;
}

// RouteSections::Beside: 1 when r3 is r4 or one of r3's links beside (6 to
// 9) is r4.
extern "C" REX_FUNC(sub_82160E20) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82160E20(ctx, base);
    return;
  }
  const int32_t target = int32_t(ctx.r4.u32);
  if (int32_t(ctx.r3.u32) == target) {
    ctx.r3.u64 = 1;
    return;
  }
  const uint32_t record = Section(base, ctx.r3.u32);
  for (uint32_t beside : kBeside) {
    if (Link(base, record, beside) == target) {
      ctx.r3.u64 = 1;
      return;
    }
  }
  ctx.r3.u64 = 0;
}
