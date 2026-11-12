// The menu sounds as our own C++ (--native_menu_sounds, on by default): the
// helpers the screens play their sounds and set the volume options through.
// All four call the sound manager through its vtable, which stays
// recompiled.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_menu_sounds, true, "SBX",
                    "Our own C++ for the menu sounds and the volume options (MenuSound::Play, "
                    "Sound::Play, SoundOptions::SetEffectsVolume and SetMusicVolume) instead of "
                    "the recompiled functions; the sound manager's calls stay recompiled");

namespace {

using guest::At;
using guest::f32_be;
using guest::Guest;
using guest::i32_be;
using guest::u32_be;

// The sound manager (an AudioManager) and the virtual functions used here.
constexpr uint32_t kSoundManager = 0x826B5960;  // Holds its address.
constexpr uint32_t kPlayCue2DSlot = 10;         // (bank*, cue*, volume*)
constexpr uint32_t kGetCategorySlot = 29;       // (name) -> category, -1 for none
constexpr uint32_t kSetCategoryVolumeSlot = 30;  // (category*, volume*)

// The sound categories' names in the game image.
constexpr uint32_t kDefaultCategory = 0x8200A5C4;  // "Default": the effects
constexpr uint32_t kMusicCategory = 0x820082DC;    // "Music"

// Where the game keeps the volumes it last set.
constexpr uint32_t kEffectsVolume = 0x82606BC0;
constexpr uint32_t kEffectsVolume2 = 0x82606BC4;
constexpr uint32_t kMusicVolume = 0x82606B74;

constexpr int kMaxVolumeSetting = 15;

bool On() { return REXCVAR_GET(native_menu_sounds); }

void* SoundManager() { return At<void>(*At<u32_be>(kSoundManager)); }

// A volume option (0 to 15) as a gain from 0.0 to 1.0.
float GainOf(int setting) {
  if (setting == 0) {
    return 0.0f;
  }
  if (setting == kMaxVolumeSetting) {
    return 1.0f;
  }
  return float(setting) * (1.0f / kMaxVolumeSetting);
}

// Sets a category's volume from an option's setting. Returns the gain.
// The manager takes the category and the gain by address, so both are
// locals on the guest stack.
float SetCategoryVolume(Guest& g, uint32_t category_name, int setting, uint32_t get_return,
                        uint32_t set_return) {
  guest::StackFrame frame(g.ctx, 112);
  f32_be* gain = At<f32_be>(frame.Local(80));
  i32_be* category = At<i32_be>(frame.Local(88));
  *gain = GainOf(setting);
  {
    guest::ReturnAddress from(g, get_return);
    *category = int32_t(g.CallVirtual(SoundManager(), kGetCategorySlot, category_name));
  }
  if (*category != -1) {
    guest::ReturnAddress from(g, set_return);
    g.CallVirtual(SoundManager(), kSetCategoryVolumeSlot, category, gain);
  }
  return *gain;
}

}  // namespace

// MenuSound::Play: r3 = the cue, played from bank 0 at full volume.
extern "C" REX_FUNC(sub_82110318) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82110318(ctx, base);
    return;
  }
  Guest g(ctx, base);
  guest::StackFrame frame(ctx, 112);
  f32_be* volume = At<f32_be>(frame.Local(80));
  u32_be* cue = At<u32_be>(frame.Local(84));
  u32_be* bank = At<u32_be>(frame.Local(88));
  *cue = ctx.r3.u32;
  *volume = 1.0f;
  *bank = 0;
  guest::ReturnAddress from(g, 0x82110360);
  g.CallVirtual(SoundManager(), kPlayCue2DSlot, bank, cue, volume);
}

// Sound::Play: r3 = the bank's address, r4 = the cue's; full volume.
extern "C" REX_FUNC(sub_82173710) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82173710(ctx, base);
    return;
  }
  Guest g(ctx, base);
  const uint32_t bank = ctx.r3.u32, cue = ctx.r4.u32;
  guest::StackFrame frame(ctx, 96);
  f32_be* volume = At<f32_be>(frame.Local(80));
  *volume = 1.0f;
  guest::ReturnAddress from(g, 0x8217374C);
  g.CallVirtual(SoundManager(), kPlayCue2DSlot, bank, cue, volume);
}

// SoundOptions::SetEffectsVolume: r3 = the setting (0 to 15).
extern "C" REX_FUNC(sub_821B80C8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821B80C8(ctx, base);
    return;
  }
  Guest g(ctx, base);
  const float gain =
      SetCategoryVolume(g, kDefaultCategory, int8_t(ctx.r3.u32), 0x821B8144, 0x821B816C);
  *At<f32_be>(kEffectsVolume) = gain;
  *At<f32_be>(kEffectsVolume2) = gain;
}

// SoundOptions::SetMusicVolume: r3 = the setting (0 to 15).
extern "C" REX_FUNC(sub_821B8198) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821B8198(ctx, base);
    return;
  }
  Guest g(ctx, base);
  *At<f32_be>(kMusicVolume) =
      SetCategoryVolume(g, kMusicCategory, int8_t(ctx.r3.u32), 0x821B8214, 0x821B823C);
}
