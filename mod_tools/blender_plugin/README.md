# SBX Part Tools (Blender add-on)

Imports Shutoko Battle X part files (car models) into Blender 5.2, and
exports them back as part files the game loads through the patch loader
(`assets/PATCH_DATA/`). Untouched parts come out byte for byte the same as
the game's; edited shapes and materials are re-encoded. The plan and how
it was built: `PLAN.md`. The file formats: `docs/game_data.md` ("Part
files", "Node files", "Car models").

Everything for the add-on is in this folder: the add-on (`sbx_blender/`),
its format code (`sbx_formats/`, plain Python and numpy), the tests
(`tests/`) and the tools (`build.py`, `car_models.py`).

## Build and install

```
python build.py
```

makes `build/sbx_parts-<version>.zip`. In Blender: Edit > Preferences >
Get Extensions, the menu at the top right, Install from Disk, pick the
zip. A newer zip replaces the installed add-on. The log file's location
is shown in the add-on's preferences.

For materials with shaders no imported part has, build the shader
library once: in the add-on's preferences set BUILD_OUT and press Build
Shader Library (about a minute for all 6,430 parts). It's written to the
add-on's own folder (or the file set as Shader library) and read from
there; it's game data, so keep it out of the repo.

## Import

- **File > Import > SBX Car (.node):** a whole car from its node file in
  the dumped `BUILD_OUT` folder. Which file is which car:
  `car_models.xlsx`, sheet Cars (the SKY35's set a is `00002700.node`).
  - Set: worked out from the id; a is the full car, b the outside model,
    c the cockpit view.
  - Stock: only the first part of each slot group.
  - Wheels: a wheel by name, `SKY35` (its stock wheel) or `AD03` (an
    aftermarket one), optionally with a tire, `AD03 583`; or part ids,
    `587 588 581`. Names: `car_models.xlsx`, sheet Wheels.
- **File > Import > SBX Part (.part):** single parts (or drag them into
  the viewport). Textures are found from the part's id.

Each part becomes an empty with one mesh per shape and an empty per
marker. The meter needles hang from the marker they turn about
(`sm_hari00_Shape` under `sm_hari`): rotate the marker to see the needle
swing (the export writes the needle at rest). Each part's sidecar is kept
in the .blend as two text blocks: `<part>.sbx.json` (readable: shapes,
draws, materials, shaders, markers, and what was added) and a hidden
`.<part>.sbx.raw` (the game's part, which the export builds on).

## Edit

- Geometry: move, add and delete vertices and faces; quads and ngons are
  fine. At most 65,535 vertices per shape.
- Materials: move faces between slots, or put any SBX material in a slot:
  one from an imported part, or one made with Make SBX Material. If the
  part doesn't have the material's shader pair, the export brings it in
  (from the part the material came from, or the shader library).
- Material values of every draw: edit them in the readable sidecar
  (`unknown_fields` can't be edited): colors, power, params, flags,
  blend, shared shader, textures by index or name, shader by id
  (`vs_02 + ps_03`).
- Markers (exhaust tips, light glow points): move them.
- Tires (581, 583): edit the mesh and its shape keys; keep every key.

Hide a part to leave it out of the export. Deleting a shape object
doesn't change the part (the game's shape is written).

## Add to a part (the SBX tab)

The 3D view's sidebar (N) has an SBX tab. It shows what the active object
is (part, shape, marker, new shape) and has the buttons:

- **Add to Part:** select meshes or empties, then a part's object (its
  root, a shape, a marker) last so it's active, and press it. Each mesh
  becomes a new shape of that part, named after the object without
  Blender's `.001` (e.g. `ssmg02_Shape` next to `ssmg01_Shape` under
  `00005757_ss`); each empty a new marker. They're parented to the part.
  A copy made with Shift+D works too (rename it first). The name can be
  changed in the tab (at most 31 characters, new to the part). Every
  material slot with faces needs an SBX material.
- **Remove from Part:** the selected new shapes and markers stop being
  part of it (they stay in the scene).
- **Make SBX Material:** makes the active material an SBX material. Pick
  the shader pair (listed with what its constants say it does: lit,
  specular, cube map, textures), the game record it starts from (its
  fields not worked out are kept as they are), and its textures by name
  from the part's XPR2 (as many as that record binds; the last entry is
  the car's paint or vinyl sheet). An imported material is copied first,
  so the shapes using it don't change. Its values start as the record's
  and are edited per draw in the sidecar (below).
- **Refresh Sidecar:** updates the readable sidecar from the scene (the
  buttons and the export do this too).

The readable sidecar shows the part as the export will write it. New
shapes are added to `shapes` with the same entries as the game's (streams,
every draw in full), marked `"new": true`; new markers and their nodes are
in `markers` and `nodes`, an added shader pair in `shader_pairs`, all
marked new. Each draw says its material `slot` and `material`; a draw
whose record comes from a material (a swapped or added slot, a new shape)
also says `record_from`. Every draw's values can be edited there: colors,
power, params, flags, blend, shared shader, the shader by id (any pair:
one the part lacks is brought in), and textures by `index` or by `name`
(a name from the part's XPR2 wins; `paint` is the car's paint or vinyl
sheet). An edit to a draw from a material holds while the entry names
that slot's current material; put another material in the slot and the
entry starts again from it. Refresh Sidecar after editing to see the
values as they'll be written. If the part can't be built, the reason is
in `export_problem` at the end.

Edit Mode is fine: the export and the buttons leave it and come back.

The export writes new shapes in the stream layout of a shape their
materials came from, at their sorted place with their nodes, and new
markers at their position (`docs/game_data.md`, "Part files", "Node and
shape order"). Not tried in the game yet.

## Export

File > Export > SBX Parts (visible, .part): pick a folder, e.g.
`assets/PATCH_DATA/BUILD/TXR/BUILD/`. Every part with a visible shape is
written as `<id>.part`. Options: Skip unchanged parts, Apply modifiers.
An edit the export can't write (the two shapes with stream type 0x0D, a
material that isn't an SBX material, a shader pair in no imported part
and not in the library, a missing tire shape key) stops it with a
message naming the shape.

## Tests

They need the dumped game data (`BUILD_OUT`: the `.part` and `.node`
files, and the XPR2s for textures).

```
python tests/roundtrip.py BUILD_OUT --mode write     # every part writes back byte for byte
python tests/roundtrip.py BUILD_OUT --mode relocate  # ... with every section moved
python tests/roundtrip.py BUILD_OUT --mode sidecar   # ... through the sidecar
python tests/encode.py BUILD_OUT                     # every shape re-encodes
python tests/coverage.py BUILD_OUT -o report.md      # what the reader sees
python tests/compose.py BUILD_OUT                    # new shapes, markers, shader pairs in every part
python build.py
blender -b --factory-startup --python tests/blender_import.py -- BUILD_OUT
blender -b --factory-startup --python tests/blender_export.py -- BUILD_OUT
blender -b --factory-startup --python tests/blender_add.py -- BUILD_OUT
```

## Tools

```
python car_models.py BUILD_OUT -o car_models.xlsx              # cars, nodes, parts, wheels
python -m sbx_formats.diff A.part B.part [--all]               # compare two part files
python -m sbx_formats.xpr_textures FILE.xpr [-o DIR]           # list or extract XPR2 textures
python -m sbx_formats.library BUILD_OUT -o ../../assets/shader_library.json
```

The shader library and the sidecars are made from game data: keep them
out of the repo.
