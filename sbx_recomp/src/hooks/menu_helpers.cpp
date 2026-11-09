// The front end's menu cursors as our own C++ (--native_menu_helpers, on
// by default): MenuCursor::Update, which the screens step their cursors with
// once a tick, and MenuCursor::Pressed, which they read their buttons with
// (docs/menus.md).
//
//   MenuCursor::Update   sub_82184468  up, down, left, right and the shoulders
//   MenuCursor::Pressed  sub_821843C8  an action pressed on an unmoved cursor
//
// Written from SBX's generated code. The player's action mapper is
// asked through its vtable as the original does, so the mapper itself stays
// recompiled (and subtick's once-a-tick mapper update still applies).
//
// A cursor (68 bytes): +0 the row, +4 the row before this update, +8 which
// way the row moved (1, -1 or 0), +12 the first row, +16 the last, +20
// whether it wraps, +24 up/down locked, +28 left/right step the row, +32 the
// shoulders step the row, +36 the side asked this tick (-1, 0, 1), +40
// whether it wrapped, +64 the player.

#include "guest.h"
#include "hooks/guest_call.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_menu_helpers, true, "SBX",
                    "Our own C++ for the menu cursors (MenuCursor::Update and Pressed) instead "
                    "of the recompiled functions; the action mapper stays recompiled");

namespace {

// Each player's index into the mapper entries, and the entries (8 bytes: the
// mapper, then 1 when it's blocked).
constexpr uint32_t kPlayerEntries = 0x826AA884;
constexpr uint32_t kMapperEntries = 0x826AA860;

// The mapper's vtable slots the cursor reads: +40 an action pressed, +48 an
// action fired (pressed, or repeating while held).
constexpr uint32_t kPressedSlot = 40;
constexpr uint32_t kFiredSlot = 48;

constexpr uint32_t kRow = 0;
constexpr uint32_t kPrevious = 4;
constexpr uint32_t kMoved = 8;
constexpr uint32_t kFirst = 12;
constexpr uint32_t kLast = 16;
constexpr uint32_t kWraps = 20;
constexpr uint32_t kUpDownLocked = 24;
constexpr uint32_t kLeftRightStep = 28;
constexpr uint32_t kShoulderStep = 32;
constexpr uint32_t kSide = 36;
constexpr uint32_t kWrapped = 40;
constexpr uint32_t kPlayer = 64;

int32_t Word(uint8_t* base, uint32_t cursor, uint32_t field) {
  return int32_t(guest::LoadU32(base, cursor + field));
}

void SetWord(uint8_t* base, uint32_t cursor, uint32_t field, int32_t value) {
  guest::StoreU32(base, cursor + field, uint32_t(value));
}

// The player's mapper entry.
uint32_t Entry(uint8_t* base, uint32_t cursor) {
  const uint32_t player = guest::LoadU32(base, cursor + kPlayer);
  return guest::LoadU32(base, kPlayerEntries + 4 * player) * 8 + kMapperEntries;
}

// One read of the mapper: false when the entry is blocked (+4 is 1), else
// whether the slot's answer's low byte is nonzero. The entry is read again
// for each read, as the original does. `lr` is the original's return
// address after its bctrl; `frame_bytes` its frame.
bool Read(PPCContext& ctx, uint8_t* base, uint32_t entry, uint32_t slot, uint32_t action,
          uint32_t frame_bytes, uint32_t lr) {
  if (int32_t(guest::LoadU32(base, entry + 4)) == 1) {
    return false;
  }
  const uint32_t mapper = guest::LoadU32(base, entry);
  guest_call::InFrame(ctx, base, frame_bytes, lr, [&](uint32_t) {
    ctx.r3.u64 = mapper;
    ctx.r4.u64 = action;
    REX_CALL_INDIRECT_FUNC(guest::LoadU32(base, guest::LoadU32(base, mapper) + slot));
  });
  return (ctx.r3.u32 & 0xFF) != 0;
}

struct Step {
  uint32_t slot;
  uint32_t action;
  uint32_t lr;
};

}  // namespace

// MenuCursor::Update: r3 = the cursor. r3 = +8, which way it moved.
extern "C" REX_FUNC(sub_82184468) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_menu_helpers)) {
    __imp__sub_82184468(ctx, base);
    return;
  }
  constexpr uint32_t kFrame = 128;
  const uint32_t cursor = ctx.r3.u32;
  // The original's order: +0 and +24 read, +4 = +0, +36 = 0.
  const int32_t row = Word(base, cursor, kRow);
  const int32_t locked = Word(base, cursor, kUpDownLocked);
  const uint32_t entry = Entry(base, cursor);
  SetWord(base, cursor, kPrevious, row);
  SetWord(base, cursor, kSide, 0);

  // Up and down (and the side asked): the first that fires wins. Actions
  // 153/167 up, 154/168 down, 155/169 left, 156/170 right.
  if (locked == 0) {
    struct Move {
      Step step;
      int kind;  // the row -1 or 1; 2 the side -1, 3 the side 1
    };
    static constexpr Move kMoves[] = {
        {{kFiredSlot, 153, 0x821844E4}, -1}, {{kFiredSlot, 167, 0x82184510}, -1},
        {{kFiredSlot, 154, 0x8218453C}, 1},  {{kFiredSlot, 168, 0x82184568}, 1},
        {{kFiredSlot, 155, 0x82184594}, 2},  {{kFiredSlot, 169, 0x821845C0}, 2},
        {{kFiredSlot, 156, 0x821845EC}, 3},  {{kFiredSlot, 170, 0x82184618}, 3},
    };
    for (const Move& move : kMoves) {
      if (Read(ctx, base, entry, move.step.slot, move.step.action, kFrame, move.step.lr)) {
        if (move.kind == 2) {
          SetWord(base, cursor, kSide, -1);
        } else if (move.kind == 3) {
          SetWord(base, cursor, kSide, 1);
        } else {
          SetWord(base, cursor, kRow, Word(base, cursor, kRow) + move.kind);
        }
        break;
      }
    }
  }

  // Left and right stepping the row, then the shoulders (161 and 162,
  // pressed or fired): -1, -1, 1, 1.
  struct Steps {
    uint32_t field;
    Step steps[4];
  };
  static constexpr Steps kSteps[] = {
      {kLeftRightStep,
       {{kFiredSlot, 155, 0x82184678},
        {kFiredSlot, 169, 0x821846A4},
        {kFiredSlot, 156, 0x821846D0},
        {kFiredSlot, 170, 0x821846FC}}},
      {kShoulderStep,
       {{kPressedSlot, 161, 0x8218474C},
        {kFiredSlot, 161, 0x82184778},
        {kPressedSlot, 162, 0x821847A4},
        {kFiredSlot, 162, 0x821847D0}}},
  };
  for (const Steps& steps : kSteps) {
    if (Word(base, cursor, steps.field) == 0) {
      continue;
    }
    for (int k = 0; k < 4; k++) {
      const Step& step = steps.steps[k];
      if (Read(ctx, base, entry, step.slot, step.action, kFrame, step.lr)) {
        SetWord(base, cursor, kRow, Word(base, cursor, kRow) + (k < 2 ? -1 : 1));
        break;
      }
    }
  }

  // Wrapped, or held, between the first and the last row.
  const int32_t now = Word(base, cursor, kRow);
  const int32_t first = Word(base, cursor, kFirst);
  if (Word(base, cursor, kWraps) != 0) {
    SetWord(base, cursor, kWrapped, now < first || now > Word(base, cursor, kLast) ? 1 : 0);
    int32_t result = now;
    if (now < first) {
      result = Word(base, cursor, kLast);
    } else if (now > Word(base, cursor, kLast)) {
      result = first;
    }
    SetWord(base, cursor, kRow, result);
  } else {
    SetWord(base, cursor, kWrapped, 0);
    int32_t result = now;
    if (now < first) {
      result = first;
    } else if (now > Word(base, cursor, kLast)) {
      result = Word(base, cursor, kLast);
    }
    SetWord(base, cursor, kRow, result);
  }

  const int32_t after = Word(base, cursor, kRow);
  const int32_t before = Word(base, cursor, kPrevious);
  SetWord(base, cursor, kMoved, after > before ? 1 : after < before ? -1 : 0);
  ctx.r3.u64 = guest::LoadU32(base, cursor + kMoved);
}

// MenuCursor::Pressed: r3 = the cursor, r4 = the action (159 accept, 158
// cancel, ...). 1 when the cursor didn't move or ask a side this tick, the
// player's entry isn't blocked and the mapper's +40 answers nonzero; else 0.
extern "C" REX_FUNC(sub_821843C8) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_menu_helpers)) {
    __imp__sub_821843C8(ctx, base);
    return;
  }
  const uint32_t cursor = ctx.r3.u32;
  const uint32_t action = ctx.r4.u32;
  bool pressed = false;
  if (Word(base, cursor, kMoved) == 0 && Word(base, cursor, kSide) == 0) {
    pressed = Read(ctx, base, Entry(base, cursor), kPressedSlot, action, 96, 0x82184440);
  }
  ctx.r3.u64 = pressed ? 1 : 0;
}
