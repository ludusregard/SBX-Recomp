// The race states as our own C++ (--native_race_states, on by default):
// the race state machine's simple handlers (its step cleared, the fades,
// quit, unload, the standing start, begin), the race progress' reset and
// a racer's time count, and the route helpers every racer's progress goes
// through each tick (a time's difference, whether a route position isn't
// after another, a section before or beside another).
//
// The ticks themselves (rolling and standing starts, the countdown, the
// race, after the race, quit), the progress' route following, laps, checks
// and finish, the ranking and the machine's set-up stay recompiled.

#include "game/race_states.h"
#include "game/track_zones.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_states, true, "SBX",
                    "Our own C++ for the race states' simple handlers, the progress' reset and "
                    "the route helpers each racer's progress uses every tick, instead of the "
                    "recompiled functions; the race's ticks stay recompiled");

namespace {

using game::ProgressEntry;
using game::ProgressHalfwords;
using game::RaceEntryPart;
using game::RaceProgress;
using game::RaceStates;
using game::RoutePoint;
using game::RouteSection;
using guest::At;
using guest::Guest;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_race_states); }

// The fades' screens (read-only words at 0x82001BE4 and 0x82001BE8).
constexpr uint32_t kFadeInScreen = 5;
constexpr uint32_t kFadeOutScreen = 15;

// A word cleared when the standing start begins (writable).
constexpr uint32_t kStandingStartWord = 0x826B65D0;

// A route section by its number (the low halfword).
RouteSection* Section(uint32_t number) {
  game::Route* route = At<game::Route>(*At<u32_be>(game::kRoute));
  return *guest::Element<guest::ptr<RouteSection>>(route->sections.address, number & 0xFFFF);
}

// Whether one of a section's links ahead (0 to 2) is `target`.
bool LinksAheadTo(const RouteSection* section, int32_t target) {
  for (int link = RouteSection::kForward; link < RouteSection::kForward + 3; link++) {
    if (section->links[link].section == target) {
      return true;
    }
  }
  return false;
}

// A time word's milliseconds: bit 31 the sign, then the minutes (15 bits
// at 16), the seconds (6 bits at 10) and the milliseconds (10 bits).
int32_t Milliseconds(uint32_t time) {
  const int32_t total = int32_t((((time >> 16) & 0x7FFF) * 60 + ((time >> 10) & 0x3F)) * 1000 +
                                (time & 0x3FF));
  return (time & 0x80000000u) ? -total : total;
}

}  // namespace

#define SBX_RACE_STATES_FALLBACK(name) \
  REX_FUNC_PROLOGUE();                 \
  if (!On()) {                         \
    __imp__##name(ctx, base);          \
    return;                            \
  }                                    \
  Guest g(ctx, base);

// RaceStates::ClearStep: r3 the states, returned.
extern "C" REX_FUNC(sub_82161218) {
  SBX_RACE_STATES_FALLBACK(sub_82161218)
  At<RaceStates>(ctx.r3.u32)->step = 0;
}

// RaceStates::OnQuit: r3 the states, returned.
extern "C" REX_FUNC(sub_82161238) {
  SBX_RACE_STATES_FALLBACK(sub_82161238)
  At<RaceStates>(ctx.r3.u32)->unknown_9612 = 0;
}

// RaceStates::FadeInFree: the words cleared, then the fade in started
// (sub_8209D158).
extern "C" REX_FUNC(sub_82161248) {
  SBX_RACE_STATES_FALLBACK(sub_82161248)
  RaceStates* states = At<RaceStates>(ctx.r3.u32);
  states->unknown_9616[0] = 0;
  states->unknown_9616[1] = 0;
  states->unknown_9612 = 0;
  g.Call(sub_8209D158, kFadeInScreen, 0u, 0u, 0u, 0u);
}

// RaceStates::FadeOut: the fade out started (sub_8209D1E8).
extern "C" REX_FUNC(sub_82161278) {
  SBX_RACE_STATES_FALLBACK(sub_82161278)
  g.Call(sub_8209D1E8, kFadeOutScreen, 0u, 0u, 0u, 0u);
}

// RaceStates::Unload: the race progress deleted.
extern "C" REX_FUNC(sub_821617F0) {
  SBX_RACE_STATES_FALLBACK(sub_821617F0)
  g.Call(sub_8258C868, *At<u32_be>(game::kRaceProgress));  // operator delete
}

// RaceStates::OnStandingStart: the words cleared, then sub_82139350 on
// the states.
extern "C" REX_FUNC(sub_82162280) {
  SBX_RACE_STATES_FALLBACK(sub_82162280)
  RaceStates* states = At<RaceStates>(ctx.r3.u32);
  states->unknown_9616[1] = 0;
  states->unknown_9628[0] = 0;
  states->unknown_9628[1] = 0;
  states->unknown_9612 = 0;
  states->unknown_9616[0] = 0;
  states->unknown_9628[2] = 0;
  states->unknown_9628[3] = 0;
  *At<u32_be>(kStandingStartWord) = 0;
  g.Call(sub_82139350, states);
}

// RaceProgress::Reset: r3 the progress. Its race entry reset
// (RaceEntry::Reset), then its own fields.
extern "C" REX_FUNC(sub_821627B0) {
  SBX_RACE_STATES_FALLBACK(sub_821627B0)
  RaceProgress* progress = At<RaceProgress>(ctx.r3.u32);
  g.Call(sub_821020F0, &progress->entry);
  for (ProgressEntry& entry : progress->entries) {
    for (u32_be& word : entry.unknown_0) {
      word = 0;
    }
    entry.unknown_524 = 0;
    entry.unknown_528 = 2;
    entry.unknown_532[0] = -1;
    entry.unknown_532[1] = -1;
  }
  for (ProgressHalfwords& table : progress->unknown_6120) {
    for (auto& value : table.values) {
      value = -1;
    }
    table.unknown_512 = 0;
  }
  progress->unknown_7156 = 0.0f;
  for (u32_be& word : progress->unknown_7160) {
    word = 0;
  }
  for (u32_be& word : progress->unknown_7220) {
    word = 0;
  }
  progress->item = 0xFFFFFFFFu;
  progress->unknown_7232[0] = 1.0f;
  progress->unknown_7232[1] = 1.0f;
}

// RaceStates::Begin: the words cleared, the progress reset, then the last
// four words cleared.
extern "C" REX_FUNC(sub_82162C90) {
  SBX_RACE_STATES_FALLBACK(sub_82162C90)
  RaceStates* states = At<RaceStates>(ctx.r3.u32);
  states->unknown_9616[0] = 0;
  states->unknown_9616[1] = 0;
  states->unknown_9612 = 0;
  g.Call(sub_821627B0, states->progress);
  for (u32_be& word : states->unknown_9628) {
    word = 0;
  }
}

// Time::Sub: [r3] - [r4] in milliseconds, made a time word again
// (sub_82101850; r4 as the original leaves it: -1 for a negative [r4],
// else 0).
extern "C" REX_FUNC(sub_821600F8) {
  SBX_RACE_STATES_FALLBACK(sub_821600F8)
  const uint32_t a = *At<u32_be>(ctx.r3.u32);
  const uint32_t b = *At<u32_be>(ctx.r4.u32);
  g.Call(sub_82101850, Milliseconds(a) - Milliseconds(b), (b & 0x80000000u) ? -1 : 0);
}

// RoutePosition::NotAfter: 1 when r3's point isn't after r4's.
extern "C" REX_FUNC(sub_82160170) {
  SBX_RACE_STATES_FALLBACK(sub_82160170)
  const RoutePoint* a = At<RoutePoint>(ctx.r3.u32);
  const RoutePoint* b = At<RoutePoint>(ctx.r4.u32);
  const bool not_after =
      a->section < b->section || (a->section == b->section && a->slice <= b->slice);
  ctx.r3.u64 = not_after ? 1 : 0;
}

// RaceProgress::CountTime: r3 the result (two words cleared, returned), r4
// the progress, r5 the racer. The racer's count up by one, and its time
// made from it (sub_821018B0).
extern "C" REX_FUNC(sub_82160CB0) {
  SBX_RACE_STATES_FALLBACK(sub_82160CB0)
  const uint32_t out = ctx.r3.u32;
  // The racers' parts by guest address, so any progress address (0 too)
  // works as the original's arithmetic does.
  const uint32_t parts = ctx.r4.u32 + uint32_t(offsetof(RaceProgress, entry.parts));
  RaceEntryPart* racer = guest::Element<RaceEntryPart>(parts, ctx.r5.u32);
  racer->time_count += 1;
  racer->time = g.Call(sub_821018B0, uint32_t(racer->time_count));
  u32_be* result = At<u32_be>(out);
  result[0] = 0;
  result[1] = 0;
  ctx.r3.u64 = out;
}

// RouteSections::Before: r3 a section, r4 another. r3 when one of r3's
// links ahead is r4; else the first of its links beside whose own links
// ahead hold r4; else -1.
extern "C" REX_FUNC(sub_82160D18) {
  SBX_RACE_STATES_FALLBACK(sub_82160D18)
  const int32_t target = int32_t(ctx.r4.u32);
  const RouteSection* section = Section(ctx.r3.u32);
  if (LinksAheadTo(section, target)) {
    return;
  }
  for (int link = RouteSection::kBeside; link < RouteSection::kBeside + 4; link++) {
    const int32_t beside = section->links[link].section;
    if (beside != -1 && LinksAheadTo(Section(uint32_t(beside)), target)) {
      ctx.r3.u64 = uint64_t(int64_t(beside));
      return;
    }
  }
  ctx.r3.u64 = uint64_t(int64_t(-1));
}

// RouteSections::Beside: 1 when r3 is r4 or one of r3's links beside is r4.
extern "C" REX_FUNC(sub_82160E20) {
  SBX_RACE_STATES_FALLBACK(sub_82160E20)
  const int32_t target = int32_t(ctx.r4.u32);
  if (int32_t(ctx.r3.u32) == target) {
    ctx.r3.u64 = 1;
    return;
  }
  const RouteSection* section = Section(ctx.r3.u32);
  for (int link = RouteSection::kBeside; link < RouteSection::kBeside + 4; link++) {
    if (section->links[link].section == target) {
      ctx.r3.u64 = 1;
      return;
    }
  }
  ctx.r3.u64 = 0;
}
