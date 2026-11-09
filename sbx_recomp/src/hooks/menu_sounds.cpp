// The menu sounds as our own C++ (--native_menu_sounds, on by default): the
// helpers the screens play their sounds and set the volume options through.
// All four call the sound manager (the AudioManager at [0x826B5960]) through
// its vtable, which stays recompiled:
//
//   MenuSound::Play           sub_82110318  slot 10 (+40) PlayCue2D, bank 0
//   Sound::Play               sub_82173710  slot 10, bank and cue given
//   SoundOptions::SetEffectsVolume  sub_821B80C8  slots 29 (+116) GetCategory
//   SoundOptions::SetMusicVolume    sub_821B8198  and 30 (+120) SetCategoryVolume
//
// Written from SBX's generated code, with ITC-Recomp's menu_sounds (graded B
// here: the same code but for one return address) as the reference. ITC's
// Sound::Play returns from its slot call to 0x82173750; SBX's is 0x8217374C
// (one instruction shorter: no `mr r3,r11` before the load).
//
// Each one leaves guest memory as the original does: its frame (the back
// chain, the saved LR at -8 and r31 at -16 where the original saves them) and
// the words it passes by address. Registers other than r3 after the call are
// whatever the slot left; the callers don't read them (they're volatile).

#include "guest.h"

#include <rex/cvar.h>

#include <cstring>

REXCVAR_DEFINE_BOOL(native_menu_sounds, true, "SBX",
                    "Our own C++ for the menu sounds and the volume options (MenuSound::Play, "
                    "Sound::Play, SoundOptions::SetEffectsVolume and SetMusicVolume) instead of "
                    "the recompiled functions; the sound manager's calls stay recompiled");

namespace {

constexpr uint32_t kSoundManager = 0x826B5960;  // [it] the AudioManager
constexpr uint32_t kPlayCue2DSlot = 40;
constexpr uint32_t kGetCategorySlot = 116;
constexpr uint32_t kSetCategoryVolumeSlot = 120;
constexpr uint32_t kOne = 0x82000A20;        // 1.0f
constexpr uint32_t kZero = 0x82000A24;       // 0.0f
constexpr uint32_t kFifteenth = 0x82008000;  // 1/15
constexpr uint32_t kDefaultName = 0x8200A5C4;  // "Default"
constexpr uint32_t kMusicName = 0x820082DC;    // "Music"
constexpr uint32_t kEffectsVolume = 0x82606BC0;
constexpr uint32_t kEffectsVolume2 = 0x82606BC4;
constexpr uint32_t kMusicVolume = 0x82606B74;

// The frame the original makes: `bytes` below r1, its back chain, and the
// saved LR at r1 - 8.
uint32_t OpenFrame(PPCContext& ctx, uint8_t* base, uint32_t bytes) {
  const uint32_t sp = ctx.r1.u32;
  guest::StoreU32(base, sp - 8, uint32_t(ctx.lr));
  const uint32_t frame = sp - bytes;
  guest::StoreU32(base, frame, sp);
  return frame;
}

// The manager's vtable slot `slot` (bytes), called from `frame` with r3 =
// the manager and r4 to r6 as given; returns to `lr` (the original's address
// after its bctrl). r1 and the caller's LR are put back.
void CallSlot(PPCContext& ctx, uint8_t* base, uint32_t frame, uint32_t lr, uint32_t manager,
              uint32_t slot) {
  const uint64_t sp = ctx.r1.u64;
  const uint64_t caller_lr = ctx.lr;
  ctx.r1.u64 = frame;
  ctx.r3.u64 = manager;
  ctx.lr = lr;
  REX_CALL_INDIRECT_FUNC(guest::LoadU32(base, guest::LoadU32(base, manager) + slot));
  ctx.r1.u64 = sp;
  ctx.lr = caller_lr;
}

// The two volume setters: r3's low byte (sign-extended) as a gain, 0.0 at 0,
// 1.0 at 15, else times 1/15 (fcfid, frsp, fmuls); the category `name`, its
// volume set unless GetCategory answered -1; the gain stored to `globals`.
void SetVolume(PPCContext& ctx, uint8_t* base, uint32_t name, const uint32_t* globals,
               int count, uint32_t get_lr, uint32_t set_lr) {
  const uint32_t sp = ctx.r1.u32;
  REX_STORE_U64(sp - 16, ctx.r31.u64);  // std r31,-16(r1)
  const uint32_t frame = OpenFrame(ctx, base, 112);
  const int64_t value = int8_t(uint8_t(ctx.r3.u32));
  uint32_t gain;
  if (value == 0) {
    gain = guest::LoadU32(base, kZero);
  } else if (value == 15) {
    gain = guest::LoadU32(base, kOne);
  } else {
    // std r11,88(r1): its low word stays at +92 after GetCategory's result
    // goes to +88.
    REX_STORE_U64(frame + 88, uint64_t(value));
    const float product = float(double(float(double(value))) *
                                double(guest::LoadFloat(base, kFifteenth)));
    std::memcpy(&gain, &product, 4);
  }
  guest::StoreU32(base, frame + 80, gain);

  ctx.r4.u64 = name;
  CallSlot(ctx, base, frame, get_lr, guest::LoadU32(base, kSoundManager), kGetCategorySlot);
  guest::StoreU32(base, frame + 88, ctx.r3.u32);
  if (ctx.r3.s32 != -1) {
    ctx.r4.u64 = frame + 88;
    ctx.r5.u64 = frame + 80;
    CallSlot(ctx, base, frame, set_lr, guest::LoadU32(base, kSoundManager),
             kSetCategoryVolumeSlot);
  }
  const uint32_t stored = guest::LoadU32(base, frame + 80);
  for (int i = 0; i < count; i++) {
    guest::StoreU32(base, globals[i], stored);
  }
}

}  // namespace

// MenuSound::Play: r3 = the cue; bank 0, volume 1.0, all three by address
// (+88 bank, +84 cue, +80 volume in a 112-byte frame).
extern "C" REX_FUNC(sub_82110318) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_menu_sounds)) {
    __imp__sub_82110318(ctx, base);
    return;
  }
  ctx.fpscr.disableFlushMode();
  const uint32_t frame = OpenFrame(ctx, base, 112);
  guest::StoreU32(base, frame + 84, ctx.r3.u32);
  guest::StoreU32(base, frame + 80, guest::LoadU32(base, kOne));
  guest::StoreU32(base, frame + 88, 0);
  ctx.r4.u64 = frame + 88;
  ctx.r5.u64 = frame + 84;
  ctx.r6.u64 = frame + 80;
  CallSlot(ctx, base, frame, 0x82110360, guest::LoadU32(base, kSoundManager), kPlayCue2DSlot);
}

// Sound::Play: r3 = the bank's address, r4 = the cue's; volume 1.0 at +80
// of a 96-byte frame.
extern "C" REX_FUNC(sub_82173710) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_menu_sounds)) {
    __imp__sub_82173710(ctx, base);
    return;
  }
  ctx.fpscr.disableFlushMode();
  const uint32_t frame = OpenFrame(ctx, base, 96);
  guest::StoreU32(base, frame + 80, guest::LoadU32(base, kOne));
  ctx.r5.u64 = ctx.r4.u64;
  ctx.r4.u64 = ctx.r3.u64;
  ctx.r6.u64 = frame + 80;
  CallSlot(ctx, base, frame, 0x8217374C, guest::LoadU32(base, kSoundManager), kPlayCue2DSlot);
}

// SoundOptions::SetEffectsVolume: r3 = the setting (0 to 15).
extern "C" REX_FUNC(sub_821B80C8) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_menu_sounds)) {
    __imp__sub_821B80C8(ctx, base);
    return;
  }
  ctx.fpscr.disableFlushMode();
  static constexpr uint32_t kGlobals[] = {kEffectsVolume, kEffectsVolume2};
  SetVolume(ctx, base, kDefaultName, kGlobals, 2, 0x821B8144, 0x821B816C);
}

// SoundOptions::SetMusicVolume: r3 = the setting (0 to 15).
extern "C" REX_FUNC(sub_821B8198) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_menu_sounds)) {
    __imp__sub_821B8198(ctx, base);
    return;
  }
  ctx.fpscr.disableFlushMode();
  static constexpr uint32_t kGlobals[] = {kMusicVolume};
  SetVolume(ctx, base, kMusicName, kGlobals, 1, 0x821B8214, 0x821B823C);
}
