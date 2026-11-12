// The graphics library's shader objects as our own C++
// (--native_shader_objects, on by default): a shader file's record, the
// vertex and pixel shader objects' set-up, their creates, and the two
// loaders that make them (a model's shader pairs, the shared pair tables).
// The objects are guest data the renderer reads. The allocations, XMemCpy
// (sub_822932D0) and memcpy (sub_8258CB90, wrapped by the native renderer)
// stay recompiled, called from here; memset to 0 is done here.

#include "game/model.h"
#include "game/shader_objects.h"

#include <rex/cvar.h>

#include <cstring>

REXCVAR_DEFINE_BOOL(native_shader_objects, true, "SBX",
                    "Our own C++ for the shader objects (a shader file's record, the vertex "
                    "and pixel shaders' set-up and creates, the mesh and pair loaders) instead "
                    "of the recompiled functions; the allocations and copies stay recompiled");

namespace {

using game::FetchConstant;
using game::Model;
using game::PixelShader;
using game::ShaderFile;
using game::ShaderPairEntry;
using game::ShaderPairLoad;
using game::ShaderRecord;
using game::ShaderSection;
using game::ShaderSectionPair;
using game::VertexShader;
using game::VertexShaderHeader;
using game::VertexShaderInfo;
using guest::AddressOf;
using guest::At;
using guest::Element;
using guest::Guest;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_shader_objects); }

constexpr uint32_t kCreated = 0x00100000;  // flags' bit once Create made it
constexpr uint32_t kGuard = 0x0BADF00D;    // the word after a made microcode
constexpr uint32_t kUnknown20 = 0xFFFF0000;

// XMemAlloc's attributes in the creates.
constexpr uint32_t kObjectAttributes = 0x24800000;
constexpr uint32_t kVertexCodeAttributes = 0xA5800000;
constexpr uint32_t kPixelCodeAttributes = 0xB5800000;

// The shared pair tables (writable; OnPairLoaded): an object a pair.
constexpr uint32_t kVertexTable = 0x826AF998;
constexpr uint32_t kPixelTable = 0x826AD978;

// Mesh::LoadShaders' debug messages for a microcode it couldn't allocate
// (read-only strings).
constexpr uint32_t kVertexAllocError = 0x82069B5C;
constexpr uint32_t kPixelAllocError = 0x82069B80;

// What a shader file makes: its object's size (the fields and the file's
// header), the microcode and its size.
struct Shader {
  uint32_t object_bytes, code, code_bytes;
};

uint32_t FieldBytes(const ShaderFile* file) {
  return (file->magic & 0x7F) != 0 ? sizeof(VertexShader) : sizeof(PixelShader);
}

Shader Read(const ShaderFile* file) {
  return {FieldBytes(file) + file->header_bytes, AddressOf(file) + file->header_bytes,
          file->code_bytes};
}

// VertexShader::Init (sub_822A5CD0): the fields cleared and set from the
// file's header (copied after them) and the microcode: each fetch word, and
// its 12-byte constant from the microcode.
void VertexInit(VertexShader* shader, uint32_t code) {
  std::memset(shader, 0, sizeof(VertexShader));
  const uint32_t header = AddressOf(shader) + sizeof(VertexShader);
  const uint32_t info_address = header + At<VertexShaderHeader>(header)->info_offset;
  const VertexShaderInfo* info = At<VertexShaderInfo>(info_address);
  shader->code = code;
  shader->flags = VertexShader::kFlags;
  shader->unknown_4 = 1;
  shader->unknown_20 = kUnknown20;
  shader->unknown_24 = -1;
  const uint32_t count = info->fetch_count;
  shader->fetch_count = count;
  const uint32_t words = info_address + 4 * (info->fetch_offset + 10);
  for (uint32_t i = 0; i < count; i++) {
    const uint32_t word = *Element<u32_be>(words, i);
    *Element<u32_be>(AddressOf(shader->fetches), i) = word;
    *Element<FetchConstant>(AddressOf(shader->constants), i) =
        *Element<FetchConstant>(code, word & 0xFFF);
  }
}

// PixelShader::Init (sub_822A5B58).
void PixelInit(PixelShader* shader, uint32_t code) {
  std::memset(shader, 0, sizeof(PixelShader));
  shader->code = code;
  shader->flags = PixelShader::kFlags;
  shader->unknown_4 = 1;
  shader->unknown_20 = kUnknown20;
}

void XMemCpy(Guest& g, uint32_t to, uint32_t from, uint32_t bytes) {
  g.Call(sub_822932D0, to, from, bytes);
}

void Memcpy(Guest& g, uint32_t to, uint32_t from, uint32_t bytes) {
  g.Call(sub_8258CB90, to, from, bytes);
}

// The object's fields cleared and the file's header copied after them.
void CopyHeader(Guest& g, uint32_t object, const ShaderFile* file, uint32_t field_bytes) {
  const uint32_t bytes = file->header_bytes;
  std::memset(At<uint8_t>(object), 0, field_bytes);
  XMemCpy(g, object + field_bytes, AddressOf(file), bytes);
}

uint32_t XMemAlloc(Guest& g, uint32_t bytes, uint32_t attributes) {
  return g.Call(sub_82295118, bytes, attributes);
}

void XMemFree(Guest& g, uint32_t block, uint32_t attributes) {
  g.Call(sub_822951B0, block, attributes);
}

uint32_t Alloc(Guest& g, uint32_t bytes) { return g.Call(sub_82573B80, bytes); }  // Memory::Alloc

// Memory::AllocPhysicalEx (sub_82573C28): bytes, 0, -1, 32 and `flags`.
uint32_t AllocPhysical(Guest& g, uint32_t bytes, uint32_t flags) {
  return g.Call(sub_82573C28, bytes, 0u, -1, 32u, flags);
}

// A microcode made by a create: `bytes` + 4 from XMemAlloc, the file's
// copied in and the guard word after it. 0 when the allocation fails.
uint32_t MakeCode(Guest& g, const Shader& shader, uint32_t attributes) {
  const uint32_t code = XMemAlloc(g, shader.code_bytes + 4, attributes);
  if (code == 0) {
    return 0;
  }
  Memcpy(g, code, shader.code, shader.code_bytes);
  *At<u32_be>(code + shader.code_bytes) = kGuard;
  return code;
}

// VertexShader::Create (sub_822A7388): the object, or 0.
uint32_t VertexCreate(Guest& g, const ShaderFile* file) {
  const Shader shader = Read(file);
  const uint32_t object = XMemAlloc(g, shader.object_bytes, kObjectAttributes);
  if (object == 0) {
    return 0;
  }
  CopyHeader(g, object, file, sizeof(VertexShader));
  const uint32_t code = MakeCode(g, shader, kVertexCodeAttributes);
  if (code == 0) {
    XMemFree(g, object, kObjectAttributes);
    return 0;
  }
  // The cache flush of the microcode and its guard word.
  g.Call(sub_822A95C8, code, code + shader.code_bytes + 4);
  VertexShader* made = At<VertexShader>(object);
  VertexInit(made, code);
  made->flags = made->flags | kCreated;
  return object;
}

// PixelShader::Create (sub_822A7250): the object, or 0.
uint32_t PixelCreate(Guest& g, const ShaderFile* file) {
  const Shader shader = Read(file);
  const uint32_t object = XMemAlloc(g, shader.object_bytes, kObjectAttributes);
  if (object == 0) {
    return 0;
  }
  CopyHeader(g, object, file, sizeof(PixelShader));
  const uint32_t code = MakeCode(g, shader, kPixelCodeAttributes);
  if (code == 0) {
    XMemFree(g, object, kObjectAttributes);
    return 0;
  }
  PixelShader* made = At<PixelShader>(object);
  PixelInit(made, code);
  made->flags = PixelShader::kFlags | kCreated;
  return object;
}

// One shader of a loader: its object from Memory::Alloc into `slot` (read
// back from there, as the original does), the file's header copied after
// its fields, its microcode from AllocPhysicalEx (flags 4 for a vertex
// shader, 0x404 for a pixel shader) into `code_slot` (none: nullptr) and
// copied in (XMemCpy for a vertex shader, memcpy for a pixel shader), then
// its Init.
void LoadShader(Guest& g, const ShaderFile* file, u32_be& slot, u32_be* code_slot, bool vertex) {
  const Shader shader = Read(file);
  slot = Alloc(g, shader.object_bytes);
  CopyHeader(g, slot, file, vertex ? sizeof(VertexShader) : sizeof(PixelShader));
  const uint32_t code = AllocPhysical(g, shader.code_bytes, vertex ? 4 : 0x404);
  if (code_slot != nullptr) {
    *code_slot = code;
  }
  const uint32_t kept = code_slot != nullptr ? uint32_t(*code_slot) : code;
  if (vertex) {
    XMemCpy(g, code, shader.code, shader.code_bytes);
    VertexInit(At<VertexShader>(slot), kept);
  } else {
    Memcpy(g, code, shader.code, shader.code_bytes);
    PixelInit(At<PixelShader>(slot), kept);
  }
}

}  // namespace

#define SBX_SHADER_OBJECTS_FALLBACK(name) \
  REX_FUNC_PROLOGUE();                    \
  if (!On()) {                            \
    __imp__##name(ctx, base);             \
    return;                               \
  }                                       \
  Guest g(ctx, base);

// ShaderFile::Parse: r3 the file, r4 the record; cleared, then set in the
// original's order.
extern "C" REX_FUNC(sub_822A5B08) {
  SBX_SHADER_OBJECTS_FALLBACK(sub_822A5B08)
  const ShaderFile* file = At<ShaderFile>(ctx.r3.u32);
  ShaderRecord* record = At<ShaderRecord>(ctx.r4.u32);
  record->file = 0;
  record->object_bytes = 0;
  record->code = 0;
  record->code_bytes = 0;
  const uint32_t header = file->header_bytes;
  record->file = AddressOf(file);
  record->code = header + AddressOf(file);
  record->object_bytes = FieldBytes(file) + header;
  record->code_bytes = file->code_bytes;
}

// VertexShader::Init and PixelShader::Init: r3 the object, r4 the microcode.
extern "C" REX_FUNC(sub_822A5CD0) {
  SBX_SHADER_OBJECTS_FALLBACK(sub_822A5CD0)
  VertexInit(At<VertexShader>(ctx.r3.u32), ctx.r4.u32);
}

extern "C" REX_FUNC(sub_822A5B58) {
  SBX_SHADER_OBJECTS_FALLBACK(sub_822A5B58)
  PixelInit(At<PixelShader>(ctx.r3.u32), ctx.r4.u32);
}

// VertexShader::CopyHeader and PixelShader::CopyHeader: r3 the object, r5
// Parse's record.
extern "C" REX_FUNC(sub_822A7348) {
  SBX_SHADER_OBJECTS_FALLBACK(sub_822A7348)
  const ShaderRecord* record = At<ShaderRecord>(ctx.r5.u32);
  CopyHeader(g, ctx.r3.u32, At<ShaderFile>(record->file), sizeof(VertexShader));
}

extern "C" REX_FUNC(sub_822A7210) {
  SBX_SHADER_OBJECTS_FALLBACK(sub_822A7210)
  const ShaderRecord* record = At<ShaderRecord>(ctx.r5.u32);
  CopyHeader(g, ctx.r3.u32, At<ShaderFile>(record->file), sizeof(PixelShader));
}

// VertexShader::Create and PixelShader::Create: r3 the file; the object, or
// 0.
extern "C" REX_FUNC(sub_822A7388) {
  SBX_SHADER_OBJECTS_FALLBACK(sub_822A7388)
  ctx.r3.u64 = VertexCreate(g, At<ShaderFile>(ctx.r3.u32));
}

extern "C" REX_FUNC(sub_822A7250) {
  SBX_SHADER_OBJECTS_FALLBACK(sub_822A7250)
  ctx.r3.u64 = PixelCreate(g, At<ShaderFile>(ctx.r3.u32));
}

// Mesh::LoadShaders: r3 the model, r4 its shader section. The pair count to
// the model, its pair entries from Memory::Alloc, then each pair's two
// shaders loaded (with a debug print for a microcode it couldn't allocate).
extern "C" REX_FUNC(sub_8257A620) {
  SBX_SHADER_OBJECTS_FALLBACK(sub_8257A620)
  Model* model = At<Model>(ctx.r3.u32);
  const ShaderSection* section = At<ShaderSection>(ctx.r4.u32);
  const uint32_t count = section->pair_count;
  model->shader_pair_count = count;
  if (count == 0) {
    return;
  }
  model->shader_pairs = Alloc(g, count * uint32_t(sizeof(ShaderPairEntry)));
  uint32_t pair = AddressOf(section) + sizeof(ShaderSection);
  for (uint32_t i = 0; i < model->shader_pair_count; i++) {
    const ShaderSectionPair* entry_in = At<ShaderSectionPair>(pair);
    const uint32_t vertex = pair + sizeof(ShaderSectionPair);
    const uint32_t pixel = entry_in->vertex_bytes + vertex;
    const uint32_t next = entry_in->pixel_bytes + pixel;
    ShaderPairEntry* entry = Element<ShaderPairEntry>(model->shader_pairs, i);
    LoadShader(g, At<ShaderFile>(vertex), entry->vertex, &entry->vertex_code, true);
    LoadShader(g, At<ShaderFile>(pixel), entry->pixel, &entry->pixel_code, false);
    if (entry->vertex_code == 0) {
      g.Call(sub_82586A00, kVertexAllocError);  // the debug print
    }
    if (entry->pixel_code == 0) {
      g.Call(sub_82586A00, kPixelAllocError);
    }
    pair = next;
  }
}

// Shaders::OnPairLoaded: the archive's callback for a shared shader pair;
// r6 its context, freed at the end (operator delete).
extern "C" REX_FUNC(sub_820A08B8) {
  SBX_SHADER_OBJECTS_FALLBACK(sub_820A08B8)
  const ShaderPairLoad* load = At<ShaderPairLoad>(ctx.r6.u32);
  LoadShader(g, At<ShaderFile>(load->vertex_file), *Element<u32_be>(kVertexTable, load->pair),
             nullptr, true);
  LoadShader(g, At<ShaderFile>(load->pixel_file), *Element<u32_be>(kPixelTable, load->pair),
             nullptr, false);
  g.Call(sub_8258C868, load);
}
