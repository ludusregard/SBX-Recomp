// The graphics library's shader objects as our own C++
// (--native_shader_objects, on by default): a shader file's record, the
// vertex and pixel shader objects' set-up, their creates, and the two
// loaders that make them (a mesh's shader pairs, the shared pair tables).
//
// Written from SBX's generated code. The objects are guest data the
// renderer reads, so the stores are the originals', in their order. The
// allocations, XMemCpy (sub_822932D0) and memcpy (sub_8258CB90, wrapped by
// the native renderer) stay recompiled; memset to 0 is done here.
//
// A shader file: +0 the magic (an object of 592 bytes when its low 7 bits
// are set, else 56), +4 the header's size, +8 the microcode's; the header
// is the file's first bytes, the microcode right after it.
// A vertex shader object (592 bytes, the file's header copied after them):
// +0 6 (| 0x100000 once made by Create), +4 1, +20 0xFFFF0000, +24 -1, +48
// the microcode, +60 the fetch count, +68 the fetch words, +196 their
// 12-byte constants. A pixel shader (56 bytes): +0 7, +4 1, +20 0xFFFF0000,
// +24 the microcode.

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_shader_objects, true, "SBX",
                    "Our own C++ for the shader objects (a shader file's record, the vertex "
                    "and pixel shaders' set-up and creates, the mesh and pair loaders) instead "
                    "of the recompiled functions; the allocations and copies stay recompiled");

namespace {

using guest::LoadU32;
using guest::StoreU32;

constexpr uint32_t kVertexBytes = 592;
constexpr uint32_t kPixelBytes = 56;
constexpr uint32_t kCreated = 0x00100000;  // +0's flag once Create made it
constexpr uint32_t kGuard = 0x0BADF00D;    // the word after a made microcode

// XMemAlloc's attributes in the creates.
constexpr uint32_t kObjectAttributes = 0x24800000;
constexpr uint32_t kVertexCodeAttributes = 0xA5800000;
constexpr uint32_t kPixelCodeAttributes = 0xB5800000;

// The shared pair tables (OnPairLoaded): 4 bytes a pair.
constexpr uint32_t kVertexTable = 0x826AF998;
constexpr uint32_t kPixelTable = 0x826AD978;

// Mesh::LoadShaders' debug messages for a microcode it couldn't allocate.
constexpr uint32_t kVertexAllocError = 0x82069B5C;
constexpr uint32_t kPixelAllocError = 0x82069B80;

// ShaderFile::Parse's 16-byte record.
struct Record {
  uint32_t file, object_bytes, code, code_bytes;
};

// ShaderFile::Parse (sub_822A5B08): the record at `record` cleared, then
// +0 the file, +8 the microcode, +4 the object's size (with the header),
// +12 the microcode's size, in the original's order.
Record Parse(uint8_t* base, uint32_t file, uint32_t record) {
  StoreU32(base, record + 0, 0);
  StoreU32(base, record + 4, 0);
  StoreU32(base, record + 8, 0);
  StoreU32(base, record + 12, 0);
  const uint32_t magic = LoadU32(base, file + 0);
  const uint32_t header = LoadU32(base, file + 4);
  StoreU32(base, record + 0, file);
  StoreU32(base, record + 8, header + file);
  const uint32_t fields = (magic & 0x7F) != 0 ? kVertexBytes : kPixelBytes;
  StoreU32(base, record + 4, fields + header);
  StoreU32(base, record + 12, LoadU32(base, file + 8));
  return {file, fields + header, header + file, LoadU32(base, record + 12)};
}

// The same, kept on the host (the loaders' records live in their frames
// and nothing else reads them).
Record Read(uint8_t* base, uint32_t file) {
  const uint32_t magic = LoadU32(base, file + 0);
  const uint32_t header = LoadU32(base, file + 4);
  const uint32_t fields = (magic & 0x7F) != 0 ? kVertexBytes : kPixelBytes;
  return {file, fields + header, header + file, LoadU32(base, file + 8)};
}

// VertexShader::Init (sub_822A5CD0).
void VertexInit(uint8_t* base, uint32_t object, uint32_t code) {
  guest_memory::Clear(base, object, kVertexBytes);
  const uint32_t header = LoadU32(base, object + 612) + object + kVertexBytes;
  StoreU32(base, object + 48, code);
  StoreU32(base, object + 0, 6);
  StoreU32(base, object + 4, 1);
  StoreU32(base, object + 20, 0xFFFF0000u);
  StoreU32(base, object + 24, 0xFFFFFFFFu);
  uint32_t count = LoadU32(base, header + 28);
  StoreU32(base, object + 60, count);
  uint32_t word = header + 4 * (LoadU32(base, header + 24) + 10);
  uint32_t to = object + 196;
  for (uint32_t i = 0; count != 0; count--, i++) {
    StoreU32(base, object + 68 + 4 * i, LoadU32(base, word));
    const uint32_t from = code + 12 * (LoadU32(base, word) & 0xFFF);
    word += 4;
    StoreU32(base, to + 0, LoadU32(base, from + 0));
    StoreU32(base, to + 4, LoadU32(base, from + 4));
    StoreU32(base, to + 8, LoadU32(base, from + 8));
    to += 12;
  }
}

// PixelShader::Init (sub_822A5B58).
void PixelInit(uint8_t* base, uint32_t object, uint32_t code) {
  guest_memory::Clear(base, object, kPixelBytes);
  StoreU32(base, object + 24, code);
  StoreU32(base, object + 0, 7);
  StoreU32(base, object + 4, 1);
  StoreU32(base, object + 20, 0xFFFF0000u);
}

// VertexShader::CopyHeader (sub_822A7348), PixelShader::CopyHeader
// (sub_822A7210): the object's fields cleared, the file's header copied
// after them (XMemCpy); `record` is Parse's.
void CopyHeader(PPCContext& ctx, uint8_t* base, uint32_t object, uint32_t record,
                uint32_t fields) {
  const uint32_t bytes = LoadU32(base, LoadU32(base, record) + 4);
  guest_memory::Clear(base, object, fields);
  guest_memory::XMemCpy(ctx, base, object + fields, LoadU32(base, record), bytes);
}

// The same from a file (the loaders' host records).
void CopyHeaderFrom(PPCContext& ctx, uint8_t* base, uint32_t object, uint32_t file,
                    uint32_t fields) {
  const uint32_t bytes = LoadU32(base, file + 4);
  guest_memory::Clear(base, object, fields);
  guest_memory::XMemCpy(ctx, base, object + fields, file, bytes);
}

uint32_t XMemAlloc(PPCContext& ctx, uint8_t* base, uint32_t bytes, uint32_t attributes) {
  ctx.r3.u64 = bytes;
  ctx.r4.u64 = attributes;
  sub_82295118(ctx, base);
  return ctx.r3.u32;
}

void XMemFree(PPCContext& ctx, uint8_t* base, uint32_t block, uint32_t attributes) {
  ctx.r3.u64 = block;
  ctx.r4.u64 = attributes;
  sub_822951B0(ctx, base);
}

// Memory::Alloc (sub_82573B80).
uint32_t Alloc(PPCContext& ctx, uint8_t* base, uint32_t bytes) {
  ctx.r3.u64 = bytes;
  sub_82573B80(ctx, base);
  return ctx.r3.u32;
}

// Memory::AllocPhysicalEx (sub_82573C28): bytes, 0, -1, 32 and `flags`.
uint32_t AllocPhysical(PPCContext& ctx, uint8_t* base, uint32_t bytes, uint32_t flags) {
  ctx.r3.u64 = bytes;
  ctx.r4.u64 = 0;
  ctx.r5.u64 = 0xFFFFFFFFFFFFFFFFull;
  ctx.r6.u64 = 32;
  ctx.r7.u64 = flags;
  sub_82573C28(ctx, base);
  return ctx.r3.u32;
}

// The debug print (sub_82586A00) of the string at `text`.
void DebugPrint(PPCContext& ctx, uint8_t* base, uint32_t text) {
  ctx.r3.u64 = text;
  sub_82586A00(ctx, base);
}

// A microcode made by a create: `bytes` + 4 from XMemAlloc, the file's
// microcode copied in (memcpy) and the guard word after it. 0 when the
// allocation fails.
uint32_t MakeCode(PPCContext& ctx, uint8_t* base, uint32_t from, uint32_t bytes,
                  uint32_t attributes) {
  const uint32_t code = XMemAlloc(ctx, base, bytes + 4, attributes);
  if (code == 0) {
    return 0;
  }
  guest_memory::Memcpy(ctx, base, code, from, bytes);
  StoreU32(base, code + bytes, kGuard);
  return code;
}

// VertexShader::Create (sub_822A7388): r3 the file; the object, or 0.
uint32_t VertexCreate(PPCContext& ctx, uint8_t* base, uint32_t file) {
  const Record record = Read(base, file);
  const uint32_t object = XMemAlloc(ctx, base, record.object_bytes, kObjectAttributes);
  if (object == 0) {
    return 0;
  }
  CopyHeaderFrom(ctx, base, object, file, kVertexBytes);
  const uint32_t code =
      MakeCode(ctx, base, record.code, record.code_bytes, kVertexCodeAttributes);
  if (code == 0) {
    XMemFree(ctx, base, object, kObjectAttributes);
    return 0;
  }
  // The cache flush of the microcode and its guard word (sub_822A95C8).
  ctx.r3.u64 = code;
  ctx.r4.u64 = code + record.code_bytes + 4;
  sub_822A95C8(ctx, base);
  VertexInit(base, object, code);
  StoreU32(base, object, LoadU32(base, object) | kCreated);
  return object;
}

// PixelShader::Create (sub_822A7250): r3 the file; the object, or 0.
uint32_t PixelCreate(PPCContext& ctx, uint8_t* base, uint32_t file) {
  const Record record = Read(base, file);
  const uint32_t object = XMemAlloc(ctx, base, record.object_bytes, kObjectAttributes);
  if (object == 0) {
    return 0;
  }
  CopyHeaderFrom(ctx, base, object, file, kPixelBytes);
  const uint32_t code =
      MakeCode(ctx, base, record.code, record.code_bytes, kPixelCodeAttributes);
  if (code == 0) {
    XMemFree(ctx, base, object, kObjectAttributes);
    return 0;
  }
  guest_memory::Clear(base, object, kPixelBytes);
  StoreU32(base, object + 24, code);
  StoreU32(base, object + 0, 7);
  StoreU32(base, object + 4, 1);
  StoreU32(base, object + 20, 0xFFFF0000u);
  StoreU32(base, object + 0, 7 | kCreated);
  return object;
}

// One shader of a loader: its object from Memory::Alloc stored to `slot`
// (read back from there, as the original does), its header copied after
// the fields, its microcode from AllocPhysicalEx (flags 4 for a vertex
// shader, 0x404 for a pixel shader) stored to `code_slot` (0: none) and
// copied in (XMemCpy for a vertex shader, memcpy for a pixel shader), then
// its Init. Returns the microcode.
uint32_t LoadShader(PPCContext& ctx, uint8_t* base, uint32_t file, uint32_t slot,
                    uint32_t code_slot, bool vertex) {
  const Record record = Read(base, file);
  StoreU32(base, slot, Alloc(ctx, base, record.object_bytes));
  CopyHeaderFrom(ctx, base, LoadU32(base, slot), file, vertex ? kVertexBytes : kPixelBytes);
  const uint32_t code = AllocPhysical(ctx, base, record.code_bytes, vertex ? 4 : 0x404);
  if (code_slot != 0) {
    StoreU32(base, code_slot, code);
  }
  if (vertex) {
    guest_memory::XMemCpy(ctx, base, code, record.code, record.code_bytes);
    VertexInit(base, LoadU32(base, slot), code_slot != 0 ? LoadU32(base, code_slot) : code);
  } else {
    guest_memory::Memcpy(ctx, base, code, record.code, record.code_bytes);
    PixelInit(base, LoadU32(base, slot), code_slot != 0 ? LoadU32(base, code_slot) : code);
  }
  return code;
}

// Mesh::LoadShaders (sub_8257A620): r3 the model, r4 its shader section:
// +0 the pair count (to the model's +420); the pairs' 16-byte entries
// (vertex object, pixel object, their microcodes) from Memory::Alloc to the
// model's +24; from +32 each pair (+4 the vertex file's size, +8 the pixel
// file's; the vertex file at +32, the pixel file after it, the next pair
// after that).
void LoadShaders(PPCContext& ctx, uint8_t* base, uint32_t model, uint32_t section) {
  const uint32_t count = LoadU32(base, section);
  StoreU32(base, model + 420, count);
  if (count == 0) {
    return;
  }
  StoreU32(base, model + 24, Alloc(ctx, base, count << 4));
  uint32_t pair = section + 32;
  for (uint32_t i = 0; i < LoadU32(base, model + 420); i++) {
    const uint32_t vertex = pair + 32;
    const uint32_t pixel = LoadU32(base, pair + 4) + vertex;
    const uint32_t next = LoadU32(base, pair + 8) + pixel;
    const uint32_t entry = LoadU32(base, model + 24) + 16 * i;
    LoadShader(ctx, base, vertex, entry + 0, entry + 8, true);
    LoadShader(ctx, base, pixel, entry + 4, entry + 12, false);
    if (LoadU32(base, entry + 8) == 0) {
      DebugPrint(ctx, base, kVertexAllocError);
    }
    if (LoadU32(base, entry + 12) == 0) {
      DebugPrint(ctx, base, kPixelAllocError);
    }
    pair = next;
  }
}

// Shaders::OnPairLoaded (sub_820A08B8): the archive's callback for a shared
// shader pair; r6 its 12-byte context (+0 the vertex file, +4 the pixel
// file, +8 the pair), freed at the end (operator delete, sub_8258C868).
void PairLoaded(PPCContext& ctx, uint8_t* base, uint32_t context) {
  LoadShader(ctx, base, LoadU32(base, context + 0),
             kVertexTable + (LoadU32(base, context + 8) << 2), 0, true);
  LoadShader(ctx, base, LoadU32(base, context + 4),
             kPixelTable + (LoadU32(base, context + 8) << 2), 0, false);
  ctx.r3.u64 = context;
  sub_8258C868(ctx, base);
}

bool On() { return REXCVAR_GET(native_shader_objects); }

}  // namespace

extern "C" REX_FUNC(sub_822A5B08) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822A5B08(ctx, base);
    return;
  }
  Parse(base, ctx.r3.u32, ctx.r4.u32);
}

extern "C" REX_FUNC(sub_822A5CD0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822A5CD0(ctx, base);
    return;
  }
  VertexInit(base, ctx.r3.u32, ctx.r4.u32);
}

extern "C" REX_FUNC(sub_822A5B58) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822A5B58(ctx, base);
    return;
  }
  PixelInit(base, ctx.r3.u32, ctx.r4.u32);
}

extern "C" REX_FUNC(sub_822A7348) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822A7348(ctx, base);
    return;
  }
  CopyHeader(ctx, base, ctx.r3.u32, ctx.r5.u32, kVertexBytes);
}

extern "C" REX_FUNC(sub_822A7210) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822A7210(ctx, base);
    return;
  }
  CopyHeader(ctx, base, ctx.r3.u32, ctx.r5.u32, kPixelBytes);
}

extern "C" REX_FUNC(sub_822A7388) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822A7388(ctx, base);
    return;
  }
  const uint32_t object = VertexCreate(ctx, base, ctx.r3.u32);
  ctx.r3.u64 = object;
}

extern "C" REX_FUNC(sub_822A7250) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822A7250(ctx, base);
    return;
  }
  const uint32_t object = PixelCreate(ctx, base, ctx.r3.u32);
  ctx.r3.u64 = object;
}

extern "C" REX_FUNC(sub_8257A620) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8257A620(ctx, base);
    return;
  }
  LoadShaders(ctx, base, ctx.r3.u32, ctx.r4.u32);
}

extern "C" REX_FUNC(sub_820A08B8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820A08B8(ctx, base);
    return;
  }
  PairLoaded(ctx, base, ctx.r6.u32);
}
