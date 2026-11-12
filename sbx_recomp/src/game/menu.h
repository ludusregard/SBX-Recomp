// The front end's menu pieces worked out so far (docs/menus.md).

#pragma once

#include "game/guest_types.h"

namespace game {

using guest::i32_be;
using guest::u32_be;

// The row cursor a menu screen steps once a tick (MenuCursor::Update) and
// reads its buttons through (MenuCursor::Pressed).
struct MenuCursor {
  i32_be row;
  i32_be previous_row;  // The row before this tick's update.
  i32_be moved;         // 1 down, -1 up, 0 not at all this tick.
  i32_be first_row;
  i32_be last_row;
  i32_be wraps;                 // Past either end goes to the other.
  i32_be up_down_locked;        // Up and down do nothing.
  i32_be left_right_step_rows;  // Left and right step the row.
  i32_be shoulders_step_rows;   // The shoulder buttons step the row.
  i32_be side;                  // Left (-1) or right (1) asked this tick.
  i32_be wrapped;               // The row went past an end this tick.
  uint8_t unknown_44[20];
  u32_be player;
};
GUEST_SIZE(MenuCursor, 68);
GUEST_OFFSET(MenuCursor, player, 64);

// The menu actions a cursor reads from the player's action mapper.
enum MenuAction : uint32_t {
  kMenuUp = 153,
  kMenuDown = 154,
  kMenuLeft = 155,
  kMenuRight = 156,
  kMenuCancel = 158,
  kMenuAccept = 159,
  kMenuLeftShoulder = 161,
  kMenuRightShoulder = 162,
  kMenuUpAlternate = 167,
  kMenuDownAlternate = 168,
  kMenuLeftAlternate = 169,
  kMenuRightAlternate = 170,
};

// A player's action mapper, and whether its input is blocked for now.
struct MapperEntry {
  u32_be mapper;
  i32_be blocked;
};
constexpr uint32_t kMapperEntries = 0x826AA860;  // Four, one a player.
constexpr int kPlayers = 4;
// Each player's index into the mapper entries.
constexpr uint32_t kPlayerEntryIndex = 0x826AA884;

}  // namespace game
