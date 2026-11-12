// The app's start-up as our own C++ (--native_app_start_up, on by
// default): the app's globals set, an app camera entry's constructor, two
// words cleared and two getters.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_app_start_up, true, "SBX",
                    "Our own C++ for the app start-up's globals, the app camera entry's "
                    "constructor and two getters instead of the recompiled functions");

namespace {

using guest::At;
using guest::Guest;
using guest::u32_be;

// Globals the app's objects are kept in.
constexpr uint32_t kAppInterface84 = 0x8265CD64;
constexpr uint32_t kReadErrorApp = 0x825F032C;
constexpr uint32_t kRenderApp = 0x826AA880;
constexpr uint32_t kWords8110 = 0x8280F118;  // Two words.

constexpr uint32_t kAppCameraEntryVtable = 0x82002538;

struct AppCameraEntry {
  u32_be vtable;
  u32_be unknown_4[3];
};

// An app object's two words read at start-up.
struct AppObject {
  uint8_t unknown_0[108];
  u32_be unknown_108;
  u32_be unknown_112;
};

bool On() { return REXCVAR_GET(native_app_start_up); }

}  // namespace

#define SBX_START_UP_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                \
  if (!On()) {                        \
    __imp__##name(ctx, base);         \
    return;                           \
  }                                   \
  Guest g(ctx, base);

// The setters: r3 the object, kept in its global. Return r3.
extern "C" REX_FUNC(sub_8209AD00) {  // SetAppInterface84
  SBX_START_UP_FALLBACK(sub_8209AD00)
  *At<u32_be>(kAppInterface84) = ctx.r3.u32;
}

extern "C" REX_FUNC(sub_8209AD20) {  // ReadError::SetApp
  SBX_START_UP_FALLBACK(sub_8209AD20)
  *At<u32_be>(kReadErrorApp) = ctx.r3.u32;
}

extern "C" REX_FUNC(sub_8209F6B8) {  // SetRenderApp
  SBX_START_UP_FALLBACK(sub_8209F6B8)
  *At<u32_be>(kRenderApp) = ctx.r3.u32;
}

// AppCameraEntry::AppCameraEntry: r3 the entry. Returns it.
extern "C" REX_FUNC(sub_820A7568) {
  SBX_START_UP_FALLBACK(sub_820A7568)
  AppCameraEntry* entry = At<AppCameraEntry>(ctx.r3.u32);
  entry->vtable = kAppCameraEntryVtable;
  for (u32_be& word : entry->unknown_4) {
    word = 0;
  }
}

// ClearWords8110: the two words cleared. Returns r3.
extern "C" REX_FUNC(sub_8225D588) {
  SBX_START_UP_FALLBACK(sub_8225D588)
  At<u32_be>(kWords8110)[0] = 0;
  At<u32_be>(kWords8110)[1] = 0;
}

extern "C" REX_FUNC(sub_82094A30) {  // Sub82094A30: the word at +112
  SBX_START_UP_FALLBACK(sub_82094A30)
  ctx.r3.u64 = At<AppObject>(ctx.r3.u32)->unknown_112;
}

extern "C" REX_FUNC(sub_82094A38) {  // Sub82094A38: the word at +108
  SBX_START_UP_FALLBACK(sub_82094A38)
  ctx.r3.u64 = At<AppObject>(ctx.r3.u32)->unknown_108;
}
