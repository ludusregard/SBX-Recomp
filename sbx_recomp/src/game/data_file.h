// The game's container files: a count, then each entry's offset from the
// container's start, both little-endian. The light sets, the course and car
// data and the font are containers.

#pragma once

#include "game/guest_types.h"

namespace game {

struct DataFile {
  guest::u32_le count;
  guest::u32_le offsets[1];  // The first of `count`.

  // The entry, or nullptr past the end. With `skip_empty`, an entry whose
  // first word is 0 counts as none.
  uint8_t* Entry(uint32_t index, bool skip_empty) {
    if (index >= count) {
      return nullptr;
    }
    const uint32_t file = guest::AddressOf(this);
    const uint32_t offset =
        *guest::Element<guest::u32_le>(file + offsetof(DataFile, offsets), index);
    uint8_t* entry = guest::At<uint8_t>(file + offset);
    if (skip_empty && *reinterpret_cast<uint32_t*>(entry) == 0) {
      return nullptr;
    }
    return entry;
  }
};

}  // namespace game
