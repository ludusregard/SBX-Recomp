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
marker. Each part's sidecar is kept in the .blend as two text blocks:
`<part>.sbx.json` (readable: shapes, draws, materials, shaders, markers)
and a hidden `.<part>.sbx.raw` (the original bytes the export needs).

## Edit

- Geometry: move, add and delete vertices and faces; quads and ngons are
  fine. At most 65,535 vertices per shape.
- Materials: move faces between slots, or put a material from another
  imported part in a slot (its shader must be one the part already has).
- Material values: edit them in the readable sidecar (`unknown_fields`
  can't be edited): colors, power, params, flags, blend, shared shader,
  texture indices, shader by id (`vs_02 + ps_03`, one the part has).
- Markers (exhaust tips, light glow points): move them.
- Tires (581, 583): edit the mesh and its shape keys; keep every key.

Hide a part to leave it out of the export. Deleting a shape object or
adding a new one doesn't change the part.

## Export

File > Export > SBX Parts (visible, .part): pick a folder, e.g.
`assets/PATCH_DATA/BUILD/TXR/BUILD/`. Every part with a visible shape is
written as `<id>.part`. Options: Skip unchanged parts, Apply modifiers.
An edit the export can't write (the two shapes with stream type 0x0D, a
shader the part doesn't have, a missing tire shape key) stops it with a
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
python build.py
blender -b --factory-startup --python tests/blender_import.py -- BUILD_OUT
blender -b --factory-startup --python tests/blender_export.py -- BUILD_OUT
```

## Tools

```
python car_models.py BUILD_OUT -o car_models.xlsx              # cars, nodes, parts, wheels
python -m sbx_formats.diff A.part B.part [--all]               # compare two part files
python -m sbx_formats.xpr_textures FILE.xpr [-o DIR]           # list or extract XPR2 textures
python -m sbx_formats.shaders BUILD_OUT -o ../../assets/shader_library.json
```

The shader library and the sidecars are made from game data: keep them
out of the repo.
