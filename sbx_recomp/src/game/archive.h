// The archive's table of contents (sub_8209D988 gives it for an asset): a
// 16-byte header, then a 16-byte entry per asset.

#pragma once

#include "game/guest_types.h"

namespace game {

struct TocEntry {
  uint8_t unknown_0[4];
  guest::u32_be stored_size;
  guest::u32_be unpacked_size;  // 0 for an asset stored as it is.
  uint8_t unknown_12[4];
};
GUEST_SIZE(TocEntry, 16);

// An asset's size: its unpacked size, or the stored one when that's 0.
inline uint32_t AssetSize(uint32_t toc, uint32_t id) {
  const TocEntry* entry = guest::Element<TocEntry>(toc + 16, id);
  return entry->unpacked_size != 0 ? uint32_t(entry->unpacked_size) : uint32_t(entry->stored_size);
}

}  // namespace game
