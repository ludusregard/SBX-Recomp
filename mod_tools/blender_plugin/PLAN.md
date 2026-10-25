# Blender plugin plan

A Blender add-on to import SBX part files, edit them, and export them back
for the patch loader (`assets/PATCH_DATA/`, `docs/game_data.md`, "Patch data").
This is the plan and the record of how it was built (see "Status").
Version 1.0.0 is the first complete import and export; how to build, use
and test it is in `README.md` here.

**Target: Blender 5.2** (the user's version). 4.2 and later use the
extension format: a `blender_manifest.toml` instead of `bl_info`, and the
add-on is installed from a zip. Blender ships numpy, so `sbx_formats`
runs inside it with nothing extra to install.

Read first: `README.md` here, then `docs/game_data.md` ("Models", "Node
files", "Part files", "Car models", "Shaders") and `sbx_formats/` (the
format code).

## Scope

In scope now: **patching existing parts.** Import a part (or a car's
parts), edit geometry, UVs, normals and vertex colors, assign materials,
add shapes, markers and materials to a part (1.1), export a part file the
game loads in place of the original.

Not now: editing or adding textures (XPR2 writing; new materials bind
the XPR2's existing textures), new parts or archive ids (that's the addon
loader, `docs/modding-plan.md`), skinned and morphing shapes beyond
carrying them through untouched, removing shapes.

## Ground rules

These keep the work from going in circles:

1. **Byte-exact round trip first.** An untouched part must export to the
   identical file. Every milestone has this as its test, checked on many
   files, before anything else is added.
2. **The format code doesn't need Blender.** Reading, writing and the
   sidecar are plain Python + numpy, tested from the command line.
   Blender code only converts between that and Blender objects.
3. **Carry what isn't understood.** Bytes we can't explain go into the
   sidecar as they are and come back out unchanged. Don't stop to decode
   them; note them in `docs/game_data.md` as not worked out.
4. **One milestone at a time,** each with its own test passing before
   the next starts.
5. **Find mismatches with a diff, not guesses.** A small tool that
   compares two part files field by field (header, each shape, each
   material record, each stream) and names the first difference.
6. **Repo rules apply:** American English, plain wording, relative paths,
   and names that refer to a file type use it in caps (`XPR`, `TGA`,
   `PART`, `NODE`): `read_PART`, `PARTFile`, `read_NODE`.
7. **Modules, not one big script.** Each module does one job (read,
   write, sidecar and property tags, import, export, logging), so a
   problem can be traced to one file and tested on its own. No module
   reaches into another's internals; they talk through the model.
8. **Logs for troubleshooting.** The add-on logs every step (file read,
   shape counts, what was written, why a shape was re-encoded) to a log
   file through `log.py`, so a problem can be found from the log instead
   of guessed at.

## Layout

Everything the add-on needs, and everything for working on it, is in
`mod_tools/blender_plugin/`:

```
blender_plugin/
  README.md               building, installing, using and testing it
  PLAN.md                 this file
  build.py                copies sbx_formats into the add-on and zips it
  car_models.py           makes car_models.xlsx: every car's nodes and parts,
                          every wheel (needs openpyxl)
  car_models.xlsx         its output
  sbx_blender/            the add-on (Blender extension)
    blender_manifest.toml the add-on's id, version and Blender version
    __init__.py           registration and menus only
    log.py                logging to a file, used by every module
    frames.py             game spaces to Blender's
    properties.py         the sbx_* tags on objects and materials: one
                          place that names, reads and writes them
    materials.py          Blender materials for display (XPR2 textures)
    import_part.py        game files -> Blender objects
    export_part.py        Blender objects -> sidecar + geometry -> part_write
    edit_part.py          adding shapes, markers, materials; sidecar sync
    operators.py          the SBX tab's buttons; ui.py its panels
    sources.py            records, layouts, shader pairs (sidecars, library)
    part_tree.py          a part's objects, transforms in part space
  sbx_formats/            the format code: plain Python and numpy, no Blender
    binary.py             big-endian helpers, Record (raw bytes + named fields)
    layout.py             field tables of the PART records (every byte named)
    node.py               NODE read, marker moves
    streams.py            vertex streams: decode known types, keep the rest raw
    strips.py             triangle strips
    part_model.py         the in-memory PART model (below)
    part_read.py          the reader: bytes -> model
    part_write.py         the writer: model -> bytes
    decode.py             the decoded view: positions, normals, UVs, triangles
    encode.py             edited shapes: vertex data and triangles -> streams, strips
    compose.py            adding shapes, markers, shader pairs to a part; its rules
    library.py            the shader library: every pair, with examples
    cars.py               car blocks, part slots, wheels, the node frame
    sidecar.py            the sidecar: model <-> JSON
    shaders.py            the shader section and the shader library
    diff.py               field-by-field comparison of two part files
    xpr_textures.py       XPR2 textures: list and decode
  tests/
    roundtrip.py          part -> model -> bytes over a folder of parts
                          (modes: rebuild, write, relocate, sidecar)
    coverage.py           reader coverage report: stream types, sections,
                          values of the fields not worked out
    encode.py             re-encodes every shape and checks it reads back the same
    blender_import.py     installs the add-on zip in Blender and checks its
                          import against the decoded parts
    blender_export.py     imports cars, exports them, compares with the
                          game's files; edits, hiding, refusals
    compose.py            adds shapes, markers, shader pairs to every part
    blender_add.py        Add to Part, Make SBX Material, needles
  build/                  build.py's output (not committed)
```

The add-on has to carry its own copy of `sbx_formats` (Blender loads an
add-on as one folder). `build.py` makes that copy; the source lives in one
place only.

## The model, the sidecar and Blender

**The model** (`sbx_formats.part_model`): a part read into Python
objects: the header, the node list, the embedded node file, the small
sections, the shader section, and each shape (its record, its streams as
numpy arrays, its draws with their material records and index buffers,
its skin data). Every record is kept as raw bytes with named fields
decoded on top (`layout.py`), so writing it back is exact.

**The sidecar** (JSON, one per part, in two halves): the ground truth
for everything Blender can't hold. The readable half
(`00002882.sbx.json`) is the part as a tree for people: shapes with
their Blender object, streams in words, and draws with their Blender
material, textures by name, shader, colors, flags and the values of the
fields not worked out; then the markers, nodes and shader pairs. The
writer never reads it. The raw half holds the bytes below,
zlib-compressed: a binary `00002882.sbx.raw` on disk, base64 in a
hidden text block in a .blend. Keyed by stable
things (part id, shape index, original shape name, draw index), never by
Blender names:

- the source part's id; a hash and the size of the game's file (in the
  raw half since 1.1: only Skip unchanged parts uses them, and they
  describe the game's file, not the sidecar, so editing the sidecar
  doesn't change them);
- the header and every section not decoded, as raw bytes (base64 or hex);
- the node list and the embedded node file;
- the shader section (raw, plus each pair's hash in the shader library);
- per shape: original name, matrix, stream layout (types, strides), the
  original index buffer and vertex data hashes (to tell whether it was
  edited), skin and morph data raw;
- per draw: the full 0x200-byte material record, its decoded fields, and
  its texture references (XPR list index, texture index, and the
  texture's name in the car's body XPR2);
- the original geometry itself (streams and index buffers, zlib and
  base64): Blender can't hold the strip order, the half-float normals or
  the stream types not worked out exactly, so an unedited shape is
  written from these.

The import keeps each part's sidecar in the .blend as two text blocks,
`<part>.sbx.json` and `.<part>.sbx.raw` (the leading dot hides it from
Blender's lists), so a saved .blend has everything the export needs.
0.1.2 and 0.1.3 stored the raw half as plain JSON in
`<part>.sbx.raw.json`; the export reads both. A .blend made before 0.1.2
has the old one-block sidecar: import those parts again.

**Blender** holds geometry and material assignment, plus custom
properties that point back into the sidecar:

- on each object: `sbx_part_id`, `sbx_shape_index`, `sbx_shape_name`;
- on each material: `sbx_record` (which material record it came from),
  `sbx_shader` (the shader pair's hash), `sbx_shared_shader`,
  `sbx_textures` (XPR list index and texture index for each bound
  texture).

A material made with Make SBX Material (1.1) has `sbx_new`,
`sbx_template` (the game record it starts from), `sbx_shader` and
`sbx_textures`, and its own values (`sbx_diffuse` ...), which the export
writes over the template's record. New shapes have `sbx_new_shape`, new
markers `sbx_new_marker` (properties.py lists every tag).

The readable half shows the part as it will be written (1.2;
edit_part.sync_sidecar builds the part as the export does and writes the
readable half from it, run by the buttons and the export): the game's
shapes in order, then the new ones, each with its streams and every draw
in full; markers, nodes and shader pairs as written; additions marked
`new`. Each draw has its `slot` and `material`, and `record_from` when
its record comes from a material. Every draw is edited there: a game
draw's edits are applied to the game's record, a draw from a material
gets the material's record with the entry's edits on top, while the
entry names that slot's current material. This keeps it stateless: the
export reapplies what the entry says. The raw half is never changed: it
is the game's part, and additions are made from the scene at each
export.

**Materials are shared, records stay per draw.** Two draws share one
Blender material when their records match except for where their indices
sit and the part-local shader index (the shader pair must match by its
library hash); fields not worked out count as differences. Materials and
textures already in the .blend are reused by later imports. Every shape
keeps one material slot per draw, in draw order, so export takes each
face's record from its shape's original draw (slot k = draw k) in the
sidecar, never from the shared material. A face put in a slot that isn't
an original draw (a new slot) takes the record its material was made
from (`sbx_record`).

**Names:** the game finds shapes by name (the XEX has patterns like
`fb**wink_L_shape`, `hl**cover_shape`, `door_p_L_**_shape` for
indicators, lights and doors), so export must write the original names.
Export uses `sbx_shape_name`; for an object without it, the object's name
with any Blender `.001`-style suffix cut off. Shape names are unique
within a part (checked on 42 parts), so cutting the suffix is safe within
a part; across a whole car's parts names repeat, which is where Blender
adds the suffixes.

## Export rules

- **Which parts:** every part with at least one visible shape object
  (not hidden, in a visible collection). A part goes out whole: its
  hidden shapes are written too, since the game finds shapes by name.
  So import a car, hide all but the bonnet, export: only the bonnet's
  part file is written.
- **Unchanged shape** (Blender's vertices, UVs, colors and triangles
  match the sidecar's original geometry decoded, within a small
  tolerance, so Blender's float rounding never counts as an edit): write
  its original bytes. This is what makes an untouched part come out
  identical, which is how the export is tested: any difference there is
  a bug, not rounding.
- **Edited shape:** encoded from Blender (`sbx_formats/encode.py`).
  Its bytes differ from the original; the test is that it reads back
  with the edit in it, and that the game loads it (not tried yet).
  Corners of a vertex stay one game vertex when their UVs and colors
  match and their normals are within 1 degree. Each material slot is a
  draw: slot k keeps draw k's record unless another material was put in
  it; a new slot takes the record its material was made from, whose
  shader pair must be one of this part's own. Slots without faces are
  dropped. Can't be written yet (the export stops and says why): the two
  shapes with stream type 0x0D (one bonnet model), shape keys other than
  the tire morphs, skinned shapes, more than 65,535 vertices.
- **Tire morphs** (parts 581 and 583: `base_Shape` and `target_00`-`04`,
  the same mesh but for positions and normals): imported as shape keys
  of `base_Shape`. An edited tire writes `base_Shape` from the mesh and
  each target from its shape key, all with the same vertex split; every
  target needs its key.
- **Markers:** a moved marker empty is written into the part's embedded
  node file: its translation, its world matrix's translation row and its
  inverse (every marker in the game's parts is top-level, with world =
  rotation plus that translation). Position only; a rotation is ignored
  with a warning.
- **What counts as an edit:** vertex count, positions (beyond 1e-5 m),
  the triangles and their order, a face's material slot, UVs, vertex
  colors, the shape object's own transform, the normals (a fingerprint
  of the corner normals taken at import, `sbx_normals`; shapes imported
  before 0.3.2 don't have it) and the tire morphs' shape keys.
  Modifiers are ignored, or applied with the export's Apply modifiers
  option (then the shape counts as edited if they change it).
- **A part imported more than once** (a wheel at each tire) is written
  once.
- **Readable sidecar edits:** the known fields of a draw in the
  readable half (blend, diffuse, ambient, specular, power, params,
  flags, shared_shader, texture indices, shader) are applied to its
  record on export (`sidecar.apply_readable_edits`), so materials can be
  adjusted without touching the raw half; only those bytes change. Not
  yet: changing how many textures are bound (+0x06C and +0x0D8 follow
  the count), and a shader id the part's own shader section doesn't
  have (a material with that pair brings it in, see Additions).
- **Additions (1.1):** new shapes (meshes joined with Add to Part) are
  encoded in the stream layout of a shape their materials came from (or
  of one of the part's own; `encode.choose_template`) and inserted at
  their sorted place with a shape node and an embedded node; new markers
  get a node and an embedded node at their position; a material whose
  shader pair the part lacks appends the pair to the shader section
  (`compose.py`; the rules are in `docs/game_data.md`, "Node and shape
  order"). The written part must still follow the game's rules
  (`check_rules`), or the export stops.
- **A slot's material swapped** on an otherwise untouched shape counts as
  an edit (1.1; before, it was ignored).
- **Shapes under markers:** the meter needles hang from their pivot
  marker; a marker between a shape and its part root counts with its
  position only (`part_tree.to_root`), so rotating it to preview the
  needle doesn't change the export.
- **Edited shape:** encode from Blender: positions (float3), normals
  (half4), UVs (half2), colors, in the shape's original stream layout;
  triangles as strips with 0xFFFF restarts; recompute the bounding box
  and sphere, stream sizes and the Xenos vertex fetch constants. Limit:
  65,535 vertices per shape (16-bit indices).
- **Material records** come from the sidecar; only their index buffer
  offsets and counts change.
- **Offsets** in the header and shapes are recomputed when sizes change.

Import keeps one Blender vertex per game vertex, in order (no merging),
with the normals as custom split normals, so an untouched import exports
the same vertices. It also keeps every game triangle, one face each: a
few shapes repeat a triangle or draw it both ways (the RX7 roll cage's
`tower01_Shape`). Blender's glTF importer, Merge by Distance and Clean
Up remove those; Blender also can't give a vertex shared by faces facing
opposite ways one custom normal for both, which only affects how such a
shape shades in Blender. On export, a vertex whose loops disagree on UV
or normal (after an edit) is split.

Import details found while testing in Blender 5.2: new shape keys start
at value 1, so the tire targets are set to 0; shapes without stored
normals (lights, LEDs, needles) shade flat, as glTF does.

- Milestone 6: done (add-on 0.2.0). File > Export > SBX Parts writes
  every part with a visible shape into a folder as `<id>.part`, with an
  option to skip parts identical to the game's. Test
  (`tests/blender_export.py`, headless): the RX7's sets a (every slot,
  with wheels: 121 parts), b (103) and c (27) and the stock SKY35 (19)
  export byte-identical to the game's files, also after saving and
  reopening the .blend; with all but the bonnet hidden only the bonnet
  is written; a moved vertex, a changed face slot and a moved object are
  each refused, naming the shape.

- Milestone 7: done in the tests (add-on 0.3.0), not yet tried in the
  game. Edited shapes are encoded (`sbx_formats/encode.py`): the streams
  in their original layout, strips (about 9% fewer indices than the
  game's), the bounds by the game's rule, the skin counts. Test
  (`tests/encode.py`): all 19,131 shapes without unknown stream types
  re-encode with streams and bounds byte-identical to the game's and the
  same triangles. Test (`tests/blender_export.py`): on the SKY35 bonnet,
  moving every vertex, subdividing, deleting half the faces, another
  part's material on some faces, a specular edit in the readable
  sidecar and a Subdivision Surface modifier (ignored, then applied)
  each export and read back with the edit and nothing else changed;
  edits to a 0x10 window, a tire with shape keys and a material whose
  shader the part lacks are refused.
  In game (the user, through the patch loader): an unedited bonnet and a
  bonnet with moved vertices load; a bonnet with Suzanne joined in
  loaded broken, because the header's +0x44 (then kept as stored) is the
  vertex data size and the game copies that many bytes. The writer now
  computes it (0.3.1); not yet tried in game again.
  0.3.2: tire morphs (shape keys) export, normals-only edits are
  detected, moved markers are written. Tested in `blender_export.py`:
  tire 583 with its base and a key edited, and subdivided (all five
  targets follow); a deleted key is refused; flat normals on the bonnet;
  a headlight glow marker moved 10 cm (only that node changes).
  0.3.3: stream type 0x10 worked out (color + 3 UVs, the third a copy of
  the first): those windows and light covers import with their UVs and
  can be edited. `tests/encode.py` now re-encodes 19,394 of the 19,396
  shapes byte-identical (all but the two 0x0D ones).

## Versions

The version is in `sbx_blender/blender_manifest.toml`. 1.0.0 is the first
complete import and export (what milestones 1-7 built). From there:
the last number for fixes, the middle one for new features, each with
the tests passing. Installing a newer zip replaces the older add-on.

## Status

The early milestones were also checked against `mod_tools/part_to_gltf.py`
and its reader `SBX_model.py` (a glTF converter); both were removed in
1.0.0, and the import test now checks against the decoded parts.

- Milestone 1: done. `sbx_formats` reads every part into the model;
  `tests/roundtrip.py` rebuilds all 6,430 parts byte for byte from the
  model; `part_to_gltf.py` output is identical (RX7 sets a, b, c, stock
  with wheels, single parts).
- Milestone 2: done. All 6,430 parts read with `sbx_formats` and with
  `SBX_model.py` (228 failed before: stream types 0x10 and 0x0D, now
  kept raw). Report: `tests/coverage.py`.
- Milestone 3: done. `part_write.py` lays the file out from the model:
  all 6,430 parts write byte-exact (`roundtrip.py --mode write`), and
  written with every section moved, they read back and write again
  identical (`--mode relocate`). Header +0x44 turned out to be the
  vertex data size (found after an in-game test, see milestone 7); the
  writer computes it.
- Milestone 4: done. `sidecar.py`: part -> model -> sidecar -> JSON ->
  model -> bytes is exact for all 6,430 parts (`--mode sidecar`), and
  `edited_shapes` tells edited geometry by hash. `shaders.py` splits the
  shader section into pairs (exact for all parts) and builds the library:
  39 shaders (11 vertex, 28 pixel), 28 pairs. The library and sidecars
  are made from game data, so they never go in the repo.
- Milestone 5: done. The add-on (`sbx_blender`, built by `build.py`)
  installs in Blender 5.2 and imports single parts (File > Import > SBX
  Part, or drag and drop) and car blocks (File > Import > SBX Car: the
  node file, set from its id, stock, wheels). Test
  (`tests/blender_import.py`, headless): the RX7's sets a (stock with
  wheels, and every slot), b and c match the glTF route shape by shape:
  positions within 1e-7 m, UVs exact, normals within 0.04 degrees. 166
  sample parts import without errors, and a saved .blend keeps the
  textures and sidecars.
  Since then: the sidecar is split into a readable half and a raw half
  (0.1.2); the readable half names shaders by id (`vs_02 + ps_03`, from
  the table in `sbx_formats/shaders.py`) and lists a field not worked
  out (`unknown_fields`) only when its value isn't the usual one (0.1.3);
  the raw half is compressed and hidden (0.1.4); glow planes show
  additive in Blender (0.2.0, `materials.is_glow`); materials are shared across parts and imports (RX7 stock
  61 -> 40 materials, SKY35 stock 32) and named by role (`sky35_ex
  trim`, `tex49 paint`).

- 1.2.0: fixes from the first test of 1.1, where additions didn't reach
  the file: a Shift+D copy keeps the original's tags, so Add to Part took
  it for the original and did nothing (it now tells a copy by its name);
  exporting in Edit Mode read stale meshes (the export now leaves Edit
  Mode and returns). The readable sidecar is now generated from the built
  part (above): new shapes, markers and pairs as full entries, every draw
  editable, textures by name, any shader by id; values of new materials
  are edited there, not in the material panel. The export's message lists
  what was added. `blender_add.py` adds: a Shift+D copy, an export from
  Edit Mode, the full sidecar entries, a new draw's color and texture
  edited by name.
- 1.1.0: adding to parts. Add to Part (new shapes and markers), Remove
  from Part, Make SBX Material (any shader pair, from the imported parts
  or the shader library; a game record to start from; textures by name),
  Refresh Sidecar, in the 3D view's SBX tab; Material Properties > SBX
  for a new material's values; the shader library (v2, with example
  records and shape layouts) built from the add-on's preferences; meter
  needles under their markers; a slot's swapped material is exported;
  the game file's hash and size moved to the raw half. Tests:
  `compose.py` (390 parts take two new shapes, a marker and a foreign
  shader pair, and read back with the rules intact), `blender_add.py`
  (S30 side skirt 5757 with a second shape, a shape sorting first, a
  marker, a library material, a swapped material, needles, Remove from
  Part). Not tried in the game yet.
- 1.0.0: everything above, with the add-on, its format code, tests and
  tools in this folder; car roots are named by car (`SKY35_a`), the
  Wheels field takes wheel names (`SKY35`, `AD03 583`), and
  `car_models.xlsx` lists every car's nodes and parts and every wheel.
  The import test checks against the decoded parts (about 0.4% of the
  game's vertices have a zero normal, and Blender can't hold the normal
  where a vertex touches a zero-area triangle; both are left out of the
  normal check).

## Milestones

1. **Set up.** `sbx_formats` with the reader moved in from
   `SBX_model.py` (which becomes a wrapper), `tests/roundtrip.py`,
   `diff.py`. Test: `part_to_gltf.py` still produces the same output.
2. **Reader coverage.** Read all 6,430 part files in `BUILD_OUT`
   without errors. Known gap: stream type 0x10 (stride 16) in part 6280
   (Devil Z). Test: a report listing every stream type, shader count and
   section seen.
3. **Writer.** Model to bytes. Test: all 6,430 parts rewrite
   byte-exact from the model.
4. **Sidecar and shader library.** Model to sidecar and back. Test: part
   -> model -> sidecar -> model -> bytes is exact for all parts. Library:
   all parts hold 39 different shaders (11 vertex, 28 pixel) in 28
   pairs; 7,199 of 27,402 draws use the shared material shaders
   (1000-1004) instead.
5. **Blender import.** One part, then a whole car block (reuse
   `part_to_gltf.py`'s car logic: node hierarchy, markers as empties,
   textures from the body XPR2 for display). Test: looks the same as the
   glTF import.
6. **Blender export, untouched.** Export works on part roots: the
   selected ones, or every part under a selected car root, one .part
   each, named by part id, each from its sidecar and its shapes. A part
   that wasn't edited comes out identical; an option skips those, so the
   output folder (`assets/PATCH_DATA/`) only gets real overrides. Test,
   headless (`blender -b --python ...`): import then export the RX7's
   parts and compare: byte-exact.
7. **Blender export, edited.** Move vertices, add and delete geometry,
   reassign materials. Test: the file reads back with the reader, and
   the game loads it through the patch loader (log line and a look in
   game).

Later: skinned and morphing shapes (tires, omamori), new materials from
the shader library, XPR2 writing, whole-car export, new parts for the
addon loader.

## Open questions

- License: the repo has none. The manifest says GPL-3.0-or-later, which
  Blender's terms require of add-ons using its Python API.

- Stream type 0x0D (two shapes, one bonnet model): the second vector's
  use isn't known, so those shapes export only untouched.
- Whether the game builds each shape's vertex declaration from its
  stream types (inferred: one vertex shader runs with several layouts).
  An in-game test of a new shape with a material from another shape
  will tell.
- Whether the game finds shapes or nodes by binary search (they're
  sorted in every part); new ones are inserted in order either way.
