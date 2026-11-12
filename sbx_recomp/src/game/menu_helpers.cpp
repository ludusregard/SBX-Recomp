// The front end's menu cursors as our own C++ (--native_menu_helpers, on
// by default): MenuCursor::Update, which the screens step their cursors with
// once a tick, and MenuCursor::Pressed, which they read their buttons with
// (docs/menus.md; game/menu.h).
//
// The player's action mapper is asked through its vtable as the original
// does, so the mapper itself stays recompiled (and subtick's once-a-tick
// mapper update still applies).

#include "game/menu.h"
#include "hooks/guest_call.h"

#include <rex/cvar.h>

#include <algorithm>

REXCVAR_DEFINE_BOOL(native_menu_helpers, true, "SBX",
                    "Our own C++ for the menu cursors (MenuCursor::Update and Pressed) instead "
                    "of the recompiled functions; the action mapper stays recompiled");

namespace {

using game::MapperEntry;
using game::MenuCursor;
using guest::At;
using guest::Guest;
using guest::i32_be;
using guest::u32_be;

// The mapper's virtual functions the cursor reads.
constexpr uint32_t kPressedSlot = 10;  // An action pressed this tick.
constexpr uint32_t kFiredSlot = 12;    // Pressed, or repeating while held.

// The original's frames and return addresses, which each mapper call is
// made from (guest_call.h).
constexpr uint32_t kUpdateFrame = 128, kPressedFrame = 96;

MapperEntry* PlayerMapper(const MenuCursor* cursor) {
  const uint32_t index = *guest::Element<u32_be>(game::kPlayerEntryIndex, cursor->player);
  return guest::Element<MapperEntry>(game::kMapperEntries, index);
}

// Asks the player's mapper about an action; false while it's blocked.
bool Ask(Guest& g, const MenuCursor* cursor, uint32_t slot, uint32_t action, uint32_t frame,
         uint32_t return_address) {
  MapperEntry* entry = PlayerMapper(cursor);
  if (entry->blocked == 1) {
    return false;
  }
  void* mapper = At<void>(entry->mapper);
  guest_call::InFrame(g.ctx, g.base, frame, return_address,
                      [&](uint32_t) { g.CallVirtual(mapper, slot, action); });
  return (g.ctx.r3.u32 & 0xFF) != 0;
}

struct Question {
  uint32_t slot;
  uint32_t action;
  uint32_t return_address;
  int step;  // What a yes does to the row.
};

// The first of the questions the mapper says yes to, or nullptr.
template <size_t N>
const Question* FirstYes(Guest& g, const MenuCursor* cursor, const Question (&questions)[N]) {
  for (const Question& q : questions) {
    if (Ask(g, cursor, q.slot, q.action, kUpdateFrame, q.return_address)) {
      return &q;
    }
  }
  return nullptr;
}

constexpr Question kUpDown[] = {
    {kFiredSlot, game::kMenuUp, 0x821844E4, -1},
    {kFiredSlot, game::kMenuUpAlternate, 0x82184510, -1},
    {kFiredSlot, game::kMenuDown, 0x8218453C, 1},
    {kFiredSlot, game::kMenuDownAlternate, 0x82184568, 1},
};
constexpr Question kSides[] = {
    {kFiredSlot, game::kMenuLeft, 0x82184594, -1},
    {kFiredSlot, game::kMenuLeftAlternate, 0x821845C0, -1},
    {kFiredSlot, game::kMenuRight, 0x821845EC, 1},
    {kFiredSlot, game::kMenuRightAlternate, 0x82184618, 1},
};
constexpr Question kLeftRightRows[] = {
    {kFiredSlot, game::kMenuLeft, 0x82184678, -1},
    {kFiredSlot, game::kMenuLeftAlternate, 0x821846A4, -1},
    {kFiredSlot, game::kMenuRight, 0x821846D0, 1},
    {kFiredSlot, game::kMenuRightAlternate, 0x821846FC, 1},
};
constexpr Question kShoulderRows[] = {
    {kPressedSlot, game::kMenuLeftShoulder, 0x8218474C, -1},
    {kFiredSlot, game::kMenuLeftShoulder, 0x82184778, -1},
    {kPressedSlot, game::kMenuRightShoulder, 0x821847A4, 1},
    {kFiredSlot, game::kMenuRightShoulder, 0x821847D0, 1},
};

// Steps the cursor by this tick's input. Returns which way the row moved.
int32_t Update(Guest& g, MenuCursor* cursor) {
  cursor->previous_row = cursor->row;
  cursor->side = 0;

  // Up or down moves the row; failing those, left or right is noted as the
  // side asked.
  if (cursor->up_down_locked == 0) {
    if (const Question* q = FirstYes(g, cursor, kUpDown)) {
      cursor->row += q->step;
    } else if (const Question* side = FirstYes(g, cursor, kSides)) {
      cursor->side = side->step;
    }
  }
  if (cursor->left_right_step_rows != 0) {
    if (const Question* q = FirstYes(g, cursor, kLeftRightRows)) {
      cursor->row += q->step;
    }
  }
  if (cursor->shoulders_step_rows != 0) {
    if (const Question* q = FirstYes(g, cursor, kShoulderRows)) {
      cursor->row += q->step;
    }
  }

  // Past an end: around to the other one, or held at it.
  const int32_t row = cursor->row, first = cursor->first_row, last = cursor->last_row;
  if (cursor->wraps != 0) {
    cursor->wrapped = row < first || row > last;
    cursor->row = row < first ? last : row > last ? first : row;
  } else {
    cursor->wrapped = 0;
    cursor->row = row < first ? first : row > last ? last : row;
  }

  const int32_t now = cursor->row, before = cursor->previous_row;
  cursor->moved = now > before ? 1 : now < before ? -1 : 0;
  return cursor->moved;
}

// Whether an action (accept, cancel, ...) was pressed on a cursor that
// didn't move or ask a side this tick.
bool Pressed(Guest& g, const MenuCursor* cursor, uint32_t action) {
  if (cursor->moved != 0 || cursor->side != 0) {
    return false;
  }
  return Ask(g, cursor, kPressedSlot, action, kPressedFrame, 0x82184440);
}

}  // namespace

// MenuCursor::Update: r3 = the cursor. Returns which way it moved.
extern "C" REX_FUNC(sub_82184468) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_menu_helpers)) {
    __imp__sub_82184468(ctx, base);
    return;
  }
  Guest g(ctx, base);
  ctx.r3.u64 = uint32_t(Update(g, At<MenuCursor>(ctx.r3.u32)));
}

// MenuCursor::Pressed: r3 = the cursor, r4 = the action. Returns 1 or 0.
extern "C" REX_FUNC(sub_821843C8) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_menu_helpers)) {
    __imp__sub_821843C8(ctx, base);
    return;
  }
  Guest g(ctx, base);
  ctx.r3.u64 = Pressed(g, At<MenuCursor>(ctx.r3.u32), ctx.r4.u32);
}
