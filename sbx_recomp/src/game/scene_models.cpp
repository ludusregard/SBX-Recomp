// The scene models' records and links as our own C++
// (--native_scene_models, on by default): the parts' and interior's links,
// the records' defaults and copies, the accessories' getter, the sound
// loads' callback, the car model's pages, SceneModel::Update (every frame)
// and the Apply steps a model's record goes through.
//
// The records' floats are copied as be<float> fields, so their bits are
// kept as the originals' lfs/stfs keep them. memcpy (sub_8258CB90, wrapped
// by the native renderer) and the calls into other parts of the game stay
// recompiled.
//
// SBX's layouts differ from ITC-Recomp's here: CarRecord::Copy copies the
// part order with a 44-byte memcpy (ITC 48) and its other fields at SBX's
// own offsets; SceneModel::Update tests the scene's +5000.

#include "game/scene_models.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_scene_models, true, "SBX",
                    "Our own C++ for the scene models (the records' defaults and copies, the "
                    "parts' links, SceneModel::Update and the Apply steps) instead of the "
                    "recompiled functions; memcpy and the models' builds stay recompiled");

namespace {

using game::AccessorySet;
using game::CarModel;
using game::CarModelOwner;
using game::CockpitRecord;
using game::InteriorRecord;
using game::InteriorRecordCopy;
using game::Matrix;
using game::PartsRecord;
using game::Pool4Record;
using game::Record12;
using game::RecordLayer;
using game::SceneCarRecord;
using game::SceneInterior;
using game::SceneModel;
using game::SceneModelRecord;
using game::SceneParts;
using game::SceneWheels;
using game::Vector;
using game::WheelsRecord;
using guest::At;
using guest::Guest;
using guest::f32_be;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_scene_models); }

constexpr uint32_t kAccessories = 0x826B51F0;     // Holds the accessories' set.
constexpr uint32_t kSoundLoadsDone = 0x826C7DF8;  // A byte a slot.
constexpr uint32_t kSoundLoaded = 0x826C7E54;     // A word a slot.
constexpr int32_t kNoAccessory = 3;
constexpr uint32_t kMemCommit = 0x1000;
constexpr uint32_t kPageReadWrite = 4;

void Memcpy(Guest& g, const void* to, const void* from, uint32_t bytes) {
  g.Call(sub_8258CB90, to, from, bytes);
}

template <typename T>
void Zero(T& object) {
  for (u32_be& word : reinterpret_cast<u32_be(&)[sizeof(T) / 4]>(object)) {
    word = 0;
  }
}

void Set(Vector& v, float x, float y, float z, float w) {
  v.x = x;
  v.y = y;
  v.z = z;
  v.w = w;
}

// Record12::Reset (sub_820D3780): no index, both floats -1.0.
void ResetRecord12(Record12& record) {
  record.unknown_4 = -1.0f;
  record.unknown_0 = -1;
  record.unknown_8 = -1.0f;
}

// Layer::Reset (sub_820D07D0).
void ResetLayer(RecordLayer& layer) {
  layer.unknown_4 = 0;
  layer.kind = 8;
  layer.unknown_0 = -1;
  Set(layer.unknown_16, 0.0f, 0.0f, 0.0f, 1.0f);
  layer.unknown_32 = 0.0f;
  Set(layer.unknown_48, 1.0f, 1.0f, 1.0f, 1.0f);
}

// CarRecord::Reset (sub_820D0848): nine vectors in three groups of three,
// its 54 layers reset, then the layers' kinds in nine groups of six, and
// the part order.
void ResetCarRecord(SceneCarRecord& record) {
  record.unknown_0 = -1;
  constexpr float kZ[9] = {1.0f, 1.0f, 0.4f, 1.0f, 1.0f, 0.125f, 1.0f, 1.0f, 0.125f};
  for (int i = 0; i < 9; i++) {
    Set(record.unknown_16[i], 0.0f, 0.0f, kZ[i], 1.0f);
  }
  record.unknown_160[0] = 0;
  record.unknown_160[1] = 0;
  for (RecordLayer& layer : record.layers) {
    ResetLayer(layer);
  }
  // Each group of six layers' kind; the first group's last three are 0.
  constexpr uint32_t kGroupKinds[9] = {8, 8, 2, 1, 3, 4, 7, 6, 5};
  for (int group = 0; group < 9; group++) {
    for (int i = 0; i < 6; i++) {
      RecordLayer& layer = record.layers[6 * group + i];
      layer.kind = (group == 0 && i >= 3) ? 0 : kGroupKinds[group];
      if (kGroupKinds[group] == 1 || kGroupKinds[group] == 5) {
        layer.unknown_4 = 1;
      }
    }
  }
  constexpr uint32_t kPartOrder[11] = {0, 0, 0, 1, 2, 3, 4, 5, 0, 6, 7};
  for (int i = 0; i < 11; i++) {
    record.part_order[i] = kPartOrder[i];
  }
  for (u32_be& word : record.unknown_3676) {
    word = 0;
  }
}

// CarRecord::SetW (sub_820D4250): 1.0 into the w of its vectors and its
// layers'.
void SetCarRecordW(SceneCarRecord& record) {
  for (Vector& v : record.unknown_16) {
    v.w = 1.0f;
  }
  for (RecordLayer& layer : record.layers) {
    layer.unknown_16.w = 1.0f;
    layer.unknown_48.w = 1.0f;
  }
}

// PartsRecord::Reset (sub_820D31A0).
void ResetPartsRecord(PartsRecord& record) {
  Zero(record);
  record.unknown_52 = 3;
}

// InteriorRecord::Reset (sub_820D3488).
void ResetInteriorRecord(InteriorRecord& record) {
  Zero(record);
  u32_be* w = record.unknown_0;
  w[7] = 1;   // +28
  w[10] = 2;  // +40
  w[13] = 3;  // +52
  w[16] = 4;  // +64
  w[18] = 1;  // +72
  w[21] = 1;  // +84
  w[22] = 1;  // +88
  record.accessory = kNoAccessory;
}

// CockpitRecord::Reset (sub_820D3100).
void ResetCockpitRecord(CockpitRecord& record) {
  for (u32_be& word : record.unknown_0) {
    word = 0;
  }
  ResetInteriorRecord(record.interior);
}

// WheelsRecord::Reset (sub_820D3548).
void ResetWheelsRecord(WheelsRecord& record) {
  record.unknown_0 = 1;
  for (u32_be& word : record.unknown_4) {
    word = 0;
  }
  for (Record12* r : {&record.unknown_16, &record.unknown_32}) {
    r->unknown_0 = -1;
    r->unknown_4 = 20.0f;
    r->unknown_8 = 0.5f;
  }
  record.unknown_28 = 0;
  ResetRecord12(record.unknown_44);
  ResetRecord12(record.unknown_56);
  for (u32_be& word : record.unknown_68) {
    word = 0;
  }
  record.caliper_texture = 0;
  record.unknown_92[0] = 0;
  record.unknown_92[1] = 0;
  record.unknown_100[0] = 0.0f;
  record.unknown_100[1] = 0.0f;
  record.unknown_108[0] = 11;
  record.unknown_108[1] = 11;
  record.unknown_116[0] = 0;
  record.unknown_116[1] = 0;
  record.unknown_124 = -1;
  for (f32_be& f : record.unknown_128) {
    f = 0.0f;
  }
}

// Pool4Record::Reset (sub_820D3128).
void ResetPool4Record(Pool4Record& record) {
  Zero(record);
  for (Pool4Record::Color& color : record.colors) {
    for (f32_be& f : color.rgb) {
      f = 1.0f;
    }
  }
}

// SceneModelRecord::Reset (sub_820D94D8).
void ResetSceneModelRecord(SceneModelRecord& record) {
  record.kind = 1;
  ResetCarRecord(record.car);
  ResetPartsRecord(record.parts);
  ResetInteriorRecord(record.interior);
  ResetCockpitRecord(record.cockpit);
  ResetWheelsRecord(record.wheels);
  ResetPool4Record(record.pool4);
}

// A layer's copied fields: its first three words, its first vector and the
// float after it, and its last vector.
void CopyLayer(RecordLayer& to, const RecordLayer& from) {
  to.unknown_0 = from.unknown_0;
  to.unknown_4 = from.unknown_4;
  to.kind = from.kind;
  to.unknown_16 = from.unknown_16;
  to.unknown_32 = from.unknown_32;
  to.unknown_48 = from.unknown_48;
}

// CarRecord::Copy (sub_820D7B98): `from` into `to`, field by field, the
// part order with memcpy.
void CopyCarRecord(Guest& g, SceneCarRecord& to, const SceneCarRecord& from) {
  to.unknown_0 = from.unknown_0;
  for (int i = 0; i < 9; i++) {
    to.unknown_16[i] = from.unknown_16[i];
  }
  to.unknown_160[0] = from.unknown_160[0];
  to.unknown_160[1] = from.unknown_160[1];
  for (int i = 0; i < SceneCarRecord::kLayers; i++) {
    CopyLayer(to.layers[i], from.layers[i]);
  }
  Memcpy(g, to.part_order, from.part_order, uint32_t(sizeof(to.part_order)));
  for (int i = 0; i < 4; i++) {
    to.unknown_3676[i] = from.unknown_3676[i];
  }
}

// SceneModelRecord::Copy (sub_820DAF58): the kind, the car record, then the
// other records (memcpy).
void CopySceneModelRecord(Guest& g, SceneModelRecord& to, const SceneModelRecord& from) {
  to.kind = from.kind;
  CopyCarRecord(g, to.car, from.car);
  Memcpy(g, &to.parts, &from.parts, uint32_t(sizeof(PartsRecord)));
  Memcpy(g, &to.interior, &from.interior, uint32_t(sizeof(InteriorRecord)));
  Memcpy(g, &to.cockpit, &from.cockpit, uint32_t(sizeof(CockpitRecord)));
  Memcpy(g, &to.wheels, &from.wheels, uint32_t(sizeof(WheelsRecord)));
  Memcpy(g, &to.pool4, &from.pool4, uint32_t(sizeof(Pool4Record)));
}

// SceneModel::Accessory (sub_820D9480): the accessory the interior's copy
// picks from the set, its +48, or 0 (none, or no entry).
uint32_t Accessory(const InteriorRecordCopy* copy) {
  const uint32_t set = *At<u32_be>(kAccessories);
  const int32_t index = copy->record.accessory;
  if (index == kNoAccessory) {
    return 0;
  }
  const uint32_t entry =
      *guest::Element<u32_be>(guest::AddressOf(At<AccessorySet>(set)->entries), uint32_t(index));
  return entry != 0 ? entry + 48 : 0;
}

// SceneModel::Update (sub_820D9E48): the deferred build counted down (and
// run, SceneModel::Build, on reaching 0 when pending). Then, when the scene
// allows it and the interior is ready, the interior's accessory anchored:
// the identity with row 3 the car model's accessory node, times the
// model's transform (Matrix::Multiply), set on the accessory
// (Accessory::SetAnchor) and stepped (Accessory::Step, r4 = 0), in the
// original's 240-byte frame.
void Update(Guest& g, SceneModel* model) {
  int32_t countdown = model->build_countdown;
  if (countdown > 0) {
    countdown--;
    model->build_countdown = countdown;
    if (model->build_pending != 0 && countdown == 0) {
      g.Call(sub_820D9538, model);
      model->build_pending = 0;
    }
  }
  if (model->scene->unknown_5000 != 0) {
    return;
  }
  if (model->unknown_5072 == 0 || model->car_model.address == 0) {
    return;
  }
  const uint32_t ready =
      model->scene->unknown_4 == 0 ? model->unknown_4296 : model->unknown_5076;
  if (ready == 0) {
    return;
  }
  const uint32_t accessory = Accessory(model->unknown_5068->interior);
  if (accessory == 0) {
    return;
  }
  guest::StackFrame frame(g.ctx, 240);
  Matrix* anchor = At<Matrix>(frame.Local(144));
  Set(anchor->right, 1.0f, 0.0f, 0.0f, 0.0f);
  Set(anchor->up, 0.0f, 1.0f, 0.0f, 0.0f);
  Set(anchor->forward, 0.0f, 0.0f, 1.0f, 0.0f);
  anchor->position = model->car_model->model->accessory_node;
  g.Call(sub_8209BE70, anchor, anchor, &model->transform);  // Matrix::Multiply
  g.Call(sub_820EF288, accessory, anchor);                  // Accessory::SetAnchor
  g.Call(sub_820EF0C0, accessory, 0u);                      // Accessory::Step
}

}  // namespace

#define SBX_SCENE_MODELS_FALLBACK(name) \
  REX_FUNC_PROLOGUE();                  \
  if (!On()) {                          \
    __imp__##name(ctx, base);           \
    return;                             \
  }                                     \
  Guest g(ctx, base);

// Accessories::Get: the accessories' set.
extern "C" REX_FUNC(sub_820F0CE0) {
  SBX_SCENE_MODELS_FALLBACK(sub_820F0CE0)
  ctx.r3.u64 = *At<u32_be>(kAccessories);
}

// SceneModel::Accessory: r3 an interior's record copy.
extern "C" REX_FUNC(sub_820D9480) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D9480)
  ctx.r3.u64 = Accessory(At<InteriorRecordCopy>(ctx.r3.u32));
}

// SceneModel::OnSoundLoaded: Archive::Load's callback for SceneModel::
// SetUp's two sound assets, r6 the context (+0 the slot): the slot's byte
// up by one, and when it's then 2 (the slot read again) its word = 1.
extern "C" REX_FUNC(sub_82173DA0) {
  SBX_SCENE_MODELS_FALLBACK(sub_82173DA0)
  const u32_be* slot = At<u32_be>(ctx.r6.u32);
  uint8_t* done = At<uint8_t>(kSoundLoadsDone + *slot);
  *done = uint8_t(*done + 1);
  const uint32_t again = *slot;
  if (*At<uint8_t>(kSoundLoadsDone + again) == 2) {
    *guest::Element<u32_be>(kSoundLoaded, again) = 1;
  }
}

// SceneWheels::SetRecord: +16776 = r4.
extern "C" REX_FUNC(sub_820E6F60) {
  SBX_SCENE_MODELS_FALLBACK(sub_820E6F60)
  At<SceneWheels>(ctx.r3.u32)->unknown_16776 = ctx.r4.u32;
}

// SetCarModel: +24 = r4, the car model.
extern "C" REX_FUNC(sub_820DC710) {
  SBX_SCENE_MODELS_FALLBACK(sub_820DC710)
  At<CarModelOwner>(ctx.r3.u32)->model.address = ctx.r4.u32;
}

// SceneParts::Link: r4 the car model's owner; the model's skeleton too.
extern "C" REX_FUNC(sub_820DEF70) {
  SBX_SCENE_MODELS_FALLBACK(sub_820DEF70)
  SceneParts* parts = At<SceneParts>(ctx.r3.u32);
  CarModelOwner* owner = At<CarModelOwner>(ctx.r4.u32);
  parts->unknown_24 = ctx.r4.u32;
  parts->unknown_28 = owner->model.address + uint32_t(offsetof(CarModel, skeleton));
}

// SceneParts::SetRecord: +37248 = r4.
extern "C" REX_FUNC(sub_820DEF88) {
  SBX_SCENE_MODELS_FALLBACK(sub_820DEF88)
  At<SceneParts>(ctx.r3.u32)->unknown_37248 = ctx.r4.u32;
}

// SceneInterior::Link: r4 the car model's owner; the model's skeleton and
// node block too.
extern "C" REX_FUNC(sub_820E3398) {
  SBX_SCENE_MODELS_FALLBACK(sub_820E3398)
  SceneInterior* interior = At<SceneInterior>(ctx.r3.u32);
  CarModelOwner* owner = At<CarModelOwner>(ctx.r4.u32);
  interior->unknown_24[0] = ctx.r4.u32;
  interior->unknown_24[1] = owner->model.address + uint32_t(offsetof(CarModel, skeleton));
  interior->unknown_24[2] = owner->model.address + uint32_t(offsetof(CarModel, node_block));
}

// sub_820E3220: +11452 = r4.
struct Object11452 {
  uint8_t unknown_0[11452];
  u32_be unknown_11452;
};

extern "C" REX_FUNC(sub_820E3220) {
  SBX_SCENE_MODELS_FALLBACK(sub_820E3220)
  At<Object11452>(ctx.r3.u32)->unknown_11452 = ctx.r4.u32;
}

// CarModel::SetRecord: r4 the car record copied in (memcpy); the copy's r3
// left.
extern "C" REX_FUNC(sub_820D3910) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D3910)
  Memcpy(g, &At<CarModel>(ctx.r3.u32)->record, At<SceneCarRecord>(ctx.r4.u32),
         uint32_t(sizeof(SceneCarRecord)));
}

// The records' defaults. Each leaves r3 as the original does.
extern "C" REX_FUNC(sub_820D3780) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D3780)
  ResetRecord12(*At<Record12>(ctx.r3.u32));
}

extern "C" REX_FUNC(sub_820D07D0) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D07D0)
  ResetLayer(*At<RecordLayer>(ctx.r3.u32));
}

// CarRecord::Reset: r3 left at its last layer.
extern "C" REX_FUNC(sub_820D0848) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D0848)
  SceneCarRecord* record = At<SceneCarRecord>(ctx.r3.u32);
  ResetCarRecord(*record);
  ctx.r3.u64 = guest::AddressOf(&record->layers[SceneCarRecord::kLayers - 1]);
}

extern "C" REX_FUNC(sub_820D31A0) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D31A0)
  ResetPartsRecord(*At<PartsRecord>(ctx.r3.u32));
}

extern "C" REX_FUNC(sub_820D3488) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D3488)
  ResetInteriorRecord(*At<InteriorRecord>(ctx.r3.u32));
}

// CockpitRecord::Reset: r3 left at its interior record.
extern "C" REX_FUNC(sub_820D3100) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D3100)
  CockpitRecord* record = At<CockpitRecord>(ctx.r3.u32);
  ResetCockpitRecord(*record);
  ctx.r3.u64 = guest::AddressOf(&record->interior);
}

extern "C" REX_FUNC(sub_820D3548) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D3548)
  ResetWheelsRecord(*At<WheelsRecord>(ctx.r3.u32));
}

extern "C" REX_FUNC(sub_820D3128) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D3128)
  ResetPool4Record(*At<Pool4Record>(ctx.r3.u32));
}

extern "C" REX_FUNC(sub_820D4250) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D4250)
  SetCarRecordW(*At<SceneCarRecord>(ctx.r3.u32));
}

// SceneModelRecord::Reset: r3 left at its pool 4 record.
extern "C" REX_FUNC(sub_820D94D8) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D94D8)
  SceneModelRecord* record = At<SceneModelRecord>(ctx.r3.u32);
  ResetSceneModelRecord(*record);
  ctx.r3.u64 = guest::AddressOf(&record->pool4);
}

// SceneModelRecord::SceneModelRecord: the car record's w set, the two
// Record12s in its wheels record reset; returns the record.
extern "C" REX_FUNC(sub_820DAF08) {
  SBX_SCENE_MODELS_FALLBACK(sub_820DAF08)
  const uint32_t address = ctx.r3.u32;
  SceneModelRecord* record = At<SceneModelRecord>(address);
  SetCarRecordW(record->car);
  ResetRecord12(record->wheels.unknown_44);
  ResetRecord12(record->wheels.unknown_56);
  ctx.r3.u64 = address;
}

// The copies: r3 to, r4 from; each returns `to`.
extern "C" REX_FUNC(sub_820D7B98) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D7B98)
  const uint32_t to = ctx.r3.u32;
  CopyCarRecord(g, *At<SceneCarRecord>(to), *At<SceneCarRecord>(ctx.r4.u32));
  ctx.r3.u64 = to;
}

extern "C" REX_FUNC(sub_820DAF58) {
  SBX_SCENE_MODELS_FALLBACK(sub_820DAF58)
  const uint32_t to = ctx.r3.u32;
  CopySceneModelRecord(g, *At<SceneModelRecord>(to), *At<SceneModelRecord>(ctx.r4.u32));
  ctx.r3.u64 = to;
}

// CarModel::AllocPages: r5 bytes (VirtualAlloc, sub_82292DD8) into [r4];
// the pages left in r3.
extern "C" REX_FUNC(sub_820C3AB0) {
  SBX_SCENE_MODELS_FALLBACK(sub_820C3AB0)
  u32_be* out = At<u32_be>(ctx.r4.u32);
  *out = g.Call(sub_82292DD8, 0u, ctx.r5.u32, kMemCommit, kPageReadWrite);
}

extern "C" REX_FUNC(sub_820D9E48) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D9E48)
  Update(g, At<SceneModel>(ctx.r3.u32));
}

// SceneParts::Apply: unless its model record is kind 1, r4 copied to its
// record (60 bytes) and the parts rebuilt (sub_820E0188).
extern "C" REX_FUNC(sub_820E0458) {
  SBX_SCENE_MODELS_FALLBACK(sub_820E0458)
  SceneParts* parts = At<SceneParts>(ctx.r3.u32);
  const uint32_t kind = *At<u32_be>(parts->unknown_37284 + 4);
  if (int32_t(kind) == 1) {
    return;
  }
  Memcpy(g, &parts->record, At<PartsRecord>(ctx.r4.u32), uint32_t(sizeof(PartsRecord)));
  g.Call(sub_820E0188, parts);
}

// SceneInterior::Apply: r4 (120 bytes) copied to its record and to its
// copy's; with an index, its group's +8 set from [+11668]; each of the six
// lights' +452 = sub_820D4228 of the car model; then sub_820E3500.
extern "C" REX_FUNC(sub_820E3E20) {
  SBX_SCENE_MODELS_FALLBACK(sub_820E3E20)
  SceneInterior* interior = At<SceneInterior>(ctx.r3.u32);
  const InteriorRecord* record = At<InteriorRecord>(ctx.r4.u32);
  Memcpy(g, &interior->record, record, uint32_t(sizeof(InteriorRecord)));
  Memcpy(g, &At<InteriorRecordCopy>(interior->unknown_24[3])->record, record,
         uint32_t(sizeof(InteriorRecord)));
  const int32_t index = interior->record.unknown_100;
  if (index != 0) {
    interior->points.group_9488.unknown_8 =
        interior->unknown_11668 + uint32_t(index * 464) + 784;
  }
  for (const u32_be& light : interior->unknown_11632) {
    const uint32_t model = At<CarModelOwner>(interior->unknown_24[0])->model.address;
    const uint32_t value = g.Call(sub_820D4228, model);
    *At<u32_be>(light + 452) = value;
  }
  g.Call(sub_820E3500, interior);
}

// SceneModel::ReloadDecals: when marked (and not kind 3), the car model's
// record set from the model's (CarModel::SetRecord) and its decals rebuilt
// (sub_820D59C8).
extern "C" REX_FUNC(sub_820D9A10) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D9A10)
  SceneModel* model = At<SceneModel>(ctx.r3.u32);
  if (model->reload_decals == 0 || int32_t(uint32_t(model->record.kind)) == 3) {
    return;
  }
  model->reload_decals = 0;
  model->unknown_5056 = 0;
  g.Call(sub_820D3910, model->car_model->model, &model->record.car);
  g.Call(sub_820D59C8, model->car_model->model);
}

// SceneModel::AddSecondWheels: once, the scene's pool 5 entry made
// (sub_820C5868, sub_820C59D8), linked to the car model (SetCarModel) and
// its record set from the model's wheels record (sub_820C58E0); 1, or 0
// when done before.
extern "C" REX_FUNC(sub_820D9AC8) {
  SBX_SCENE_MODELS_FALLBACK(sub_820D9AC8)
  SceneModel* model = At<SceneModel>(ctx.r3.u32);
  if (model->second_wheels_added != 0) {
    ctx.r3.u64 = 0;
    return;
  }
  model->second_wheels_added = 1;
  model->unknown_5112 = 0;
  const uint32_t entry = g.Call(sub_820C5868, model->scene, 5u, 0u);
  model->second_wheels = g.Call(sub_820C59D8, model->scene, 5u, entry);
  g.Call(sub_820DC710, model->second_wheels, model->car_model);  // SetCarModel
  g.Call(sub_820C58E0, model->scene, 5u, entry, &model->record.wheels);
  ctx.r3.u64 = 1;
}
