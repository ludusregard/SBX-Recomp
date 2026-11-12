// The C++ library's narrow string as our own C++ (--native_std_string, on
// by default): its storage (tidy, a new block, grow), assign, append, the
// constructors, and the game's find and replace-all over it
// (game/std_types.h, String).
//
// Blocks come from operator new and go back with operator delete; text is
// copied with memcpy_s. Those, erase, append from a string, replace, the
// wildcard search and the length and range errors stay recompiled and are
// called.

#include "game/std_types.h"

#include <rex/cvar.h>

#include <algorithm>
#include <cstring>

REXCVAR_DEFINE_BOOL(native_std_string, true, "SBX",
                    "Our own C++ for the C++ library's string (its storage, assign, append, "
                    "the constructors, find and replace-all) instead of the recompiled "
                    "functions; operator new and delete and memcpy_s stay recompiled");

namespace {

using game::String;
using guest::AddressOf;
using guest::At;
using guest::u32_be;
using guest::Guest;

constexpr uint32_t kToEnd = 0xFFFFFFFF;  // npos

bool On() { return REXCVAR_GET(native_std_string); }

// Recompiled functions this file calls.
void OperatorDelete(Guest& g, void* block) { g.Call(sub_8258C868, block); }
void MemcpyS(Guest& g, char* to, uint32_t room, const char* from, uint32_t count) {
  g.Call(sub_8258C9F0, to, room, from, count);
}
void ThrowLengthError(Guest& g) { sub_8258C1A0(g.ctx, g.base); }
void ThrowRangeError(Guest& g) { sub_8258C2D8(g.ctx, g.base); }
void EraseText(Guest& g, String* string, uint32_t at, int32_t count) {
  g.Call(sub_82091460, string, at, count);
}

// The string's own functions below, called through their overrides so the
// flag and any hook on them still apply.
char* Allocate(Guest& g, uint32_t bytes) { return At<char>(g.Call(sub_82091378, bytes, 0u)); }
void Tidy(Guest& g, String* string, bool built, uint32_t keep) {
  g.Call(sub_820913F0, string, built, keep);
}
void Reallocate(Guest& g, String* string, uint32_t capacity, uint32_t keep) {
  g.Call(sub_82091520, string, capacity, keep);
}
bool Grow(Guest& g, String* string, uint32_t length, bool trim = false) {
  return (g.Call(sub_82091608, string, length, trim) & 0xFF) != 0;
}
void Assign(Guest& g, String* string, String* other, uint32_t at, uint32_t count) {
  g.Call(sub_820917E8, string, other, at, count);
}
void Assign(Guest& g, String* string, const char* text, uint32_t count) {
  g.Call(sub_820918D0, string, text, count);
}

}  // namespace

// string::_Tidy: r3 the string, r4 whether it's built, r5 the length to
// keep. Back to in-place text: a built string's block has its first r5
// bytes copied in and is freed.
extern "C" REX_FUNC(sub_820913F0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820913F0(ctx, base);
    return;
  }
  Guest g(ctx, base);
  String* string = At<String>(ctx.r3.u32);
  const bool built = (ctx.r4.u32 & 0xFF) != 0;
  const uint32_t keep = ctx.r5.u32;
  if (built && string->HasBlock()) {
    char* block = string->block;
    if (keep != 0) {
      MemcpyS(g, string->in_place, String::kInPlaceBytes, block, keep);
    }
    OperatorDelete(g, block);
  }
  string->capacity = String::kInPlaceBytes - 1;
  string->SetLength(keep);
}

// allocator<char>::allocate: r3 the count; that many bytes from operator
// new.
extern "C" REX_FUNC(sub_82091378) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82091378(ctx, base);
    return;
  }
  sub_8258C000(ctx, base);
}

// string::_Copy: r3 the string, r4 the capacity wanted, r5 how much of the
// text to keep. Moves the text to a new block: the capacity rounded up to
// 16n - 1, or half as much again as the old one when that's more.
extern "C" REX_FUNC(sub_82091520) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82091520(ctx, base);
    return;
  }
  Guest g(ctx, base);
  String* string = At<String>(ctx.r3.u32);
  const uint32_t wanted = ctx.r4.u32, keep = ctx.r5.u32;
  uint32_t capacity = wanted | 15;
  if (capacity > String::kMaxLength) {
    capacity = wanted;
  } else {
    const uint32_t old = string->capacity;
    if (capacity / 3 < old / 2 && old <= String::kMaxLength - old / 2) {
      capacity = old + old / 2;
    }
  }
  char* block = Allocate(g, capacity + 1);
  if (keep != 0) {
    MemcpyS(g, block, capacity + 1, string->Text(), keep);
  }
  Tidy(g, string, true, 0);
  string->capacity = capacity;
  string->block = block;
  string->SetLength(keep);
}

// string::_Grow: r3 the string, r4 the length wanted, r5 whether to trim.
// Makes room for the length (a new block when it's over the capacity; back
// in place when trimming to under 16). Returns whether the length wanted is
// above 0.
extern "C" REX_FUNC(sub_82091608) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82091608(ctx, base);
    return;
  }
  Guest g(ctx, base);
  String* string = At<String>(ctx.r3.u32);
  const uint32_t wanted = ctx.r4.u32;
  const bool trim = (ctx.r5.u32 & 0xFF) != 0;
  if (wanted > String::kMaxLength) {
    ThrowLengthError(g);
  }
  if (string->capacity < wanted) {
    Reallocate(g, string, wanted, string->length);
  } else if (trim && wanted < String::kInPlaceBytes) {
    Tidy(g, string, true, std::min<uint32_t>(wanted, string->length));
  } else if (wanted == 0) {
    string->SetLength(0);
  }
  ctx.r3.u64 = wanted != 0;
}

// string::assign(const string&, pos, count): r3 the string, r4 the other,
// r5 where in it, r6 how much. Returns the string.
extern "C" REX_FUNC(sub_820917E8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820917E8(ctx, base);
    return;
  }
  Guest g(ctx, base);
  String* string = At<String>(ctx.r3.u32);
  String* other = At<String>(ctx.r4.u32);
  const uint32_t at = ctx.r5.u32, asked = ctx.r6.u32;
  if (other->length < at) {
    ThrowRangeError(g);
  }
  const uint32_t count = std::min<uint32_t>(asked, other->length - at);
  if (string == other) {
    // A part of itself: drop the tail, then the head.
    EraseText(g, string, at + count, -1);
    g.Call(sub_82091460, string, 0u, at);
  } else if (Grow(g, string, count)) {
    MemcpyS(g, string->Text(), string->capacity, other->Text() + at, count);
    string->SetLength(count);
  }
  ctx.r3.u64 = AddressOf(string);
}

// string::assign(const char*, count): r3 the string, r4 the text, r5 how
// much. Returns the string.
extern "C" REX_FUNC(sub_820918D0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820918D0(ctx, base);
    return;
  }
  Guest g(ctx, base);
  String* string = At<String>(ctx.r3.u32);
  const char* text = At<char>(ctx.r4.u32);
  const uint32_t count = ctx.r5.u32;
  if (string->Contains(text)) {
    Assign(g, string, string, uint32_t(text - string->Text()), count);
    return;
  }
  if (Grow(g, string, count)) {
    MemcpyS(g, string->Text(), string->capacity, text, count);
    string->SetLength(count);
  }
  ctx.r3.u64 = AddressOf(string);
}

// string::append(const char*, count): r3 the string, r4 the text, r5 how
// much. Returns the string.
extern "C" REX_FUNC(sub_820A6F18) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820A6F18(ctx, base);
    return;
  }
  Guest g(ctx, base);
  String* string = At<String>(ctx.r3.u32);
  const char* text = At<char>(ctx.r4.u32);
  const uint32_t count = ctx.r5.u32;
  if (string->Contains(text)) {
    // append(const string&, pos, count), recompiled.
    g.Call(sub_820A6E18, string, string, uint32_t(text - string->Text()), count);
    return;
  }
  const uint32_t old_length = string->length;
  if (kToEnd - old_length <= count) {
    ThrowLengthError(g);
  }
  if (count != 0 && Grow(g, string, old_length + count)) {
    MemcpyS(g, string->Text() + old_length, string->capacity - old_length, text, count);
    string->SetLength(old_length + count);
  }
  ctx.r3.u64 = AddressOf(string);
}

// string::string(): r3 the string, made empty. Returns it.
extern "C" REX_FUNC(sub_82578268) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82578268(ctx, base);
    return;
  }
  Guest g(ctx, base);
  String* string = At<String>(ctx.r3.u32);
  Tidy(g, string, false, 0);
  ctx.r3.u64 = AddressOf(string);
}

// string::string(const char*): r3 the string, r4 the text. Returns the
// string.
extern "C" REX_FUNC(sub_82091AD8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82091AD8(ctx, base);
    return;
  }
  Guest g(ctx, base);
  String* string = At<String>(ctx.r3.u32);
  const char* text = At<char>(ctx.r4.u32);
  Tidy(g, string, false, 0);
  Assign(g, string, text, uint32_t(std::strlen(text)));
  ctx.r3.u64 = AddressOf(string);
}

// string::string(const char*, const allocator&): the one above.
extern "C" REX_FUNC(sub_82578A50) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82578A50(ctx, base);
    return;
  }
  const uint32_t string = ctx.r3.u32;
  sub_82091AD8(ctx, base);
  ctx.r3.u64 = string;
}

// string::string(const string&, pos, count): r3 the string, r4 the other,
// r5 where in it, r6 how much. Returns the string.
extern "C" REX_FUNC(sub_825788C0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_825788C0(ctx, base);
    return;
  }
  Guest g(ctx, base);
  String* string = At<String>(ctx.r3.u32);
  String* other = At<String>(ctx.r4.u32);
  const uint32_t at = ctx.r5.u32, count = ctx.r6.u32;
  Tidy(g, string, false, 0);
  Assign(g, string, other, at, count);
  ctx.r3.u64 = AddressOf(string);
}

// String::Find: r3 the string, r4 the pattern, r5 where the match's
// position goes (may be 0), r6 where its length goes, r7 where to start.
// The wildcard search over the text from there; a match's position is
// counted from the start of the text. Returns the match's address, 0 for
// none or an empty string.
extern "C" REX_FUNC(sub_825781F0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_825781F0(ctx, base);
    return;
  }
  Guest g(ctx, base);
  String* string = At<String>(ctx.r3.u32);
  const uint32_t start = ctx.r7.u32;
  if (string->length == 0) {
    ctx.r3.u64 = 0;
    return;
  }
  // The search takes r4 to r6 as they came.
  ctx.r3.u64 = AddressOf(string->Text() + start);
  sub_82577FC0(ctx, base);
  if (ctx.r3.u32 == 0) {
    return;
  }
  // The search leaves the position's address in r5, and the caller relies
  // on that.
  if (u32_be* position = At<u32_be>(ctx.r5.u32)) {
    *position += start;
  }
}

// String::ReplaceAll: r3 the string, r4 the pattern, r5 the text to put
// in, r6 how many matches at most (0: all). Returns the string's text.
extern "C" REX_FUNC(sub_82578908) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82578908(ctx, base);
    return;
  }
  Guest g(ctx, base);
  String* string = At<String>(ctx.r3.u32);
  const char* pattern = At<char>(ctx.r4.u32);
  const char* replacement = At<char>(ctx.r5.u32);
  uint32_t matches_left = ctx.r6.u32;
  const bool all = matches_left == 0;
  const uint32_t replacement_length = uint32_t(std::strlen(replacement));

  // The match's position and length are locals the search writes to.
  guest::StackFrame frame(ctx, 144);
  u32_be* position = At<u32_be>(frame.Local(80));
  u32_be* length = At<u32_be>(frame.Local(84));
  *position = 0;
  *length = 0;

  uint32_t start = 0;
  // String::Find, then string::replace(pos, count, text, text length).
  while (g.Call(sub_825781F0, string, pattern, position, length, start) != 0) {
    const uint32_t at = *position;
    g.Call(sub_82578698, string, at, *length, replacement, replacement_length);
    start = at + replacement_length;
    *position = start;
    if (!all && --matches_left == 0) {
      break;
    }
  }
  ctx.r3.u64 = AddressOf(string->Text());
}
