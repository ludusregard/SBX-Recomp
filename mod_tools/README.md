# Mod tools

Tools for reading and editing the game's data files. Run with
`python <tool>` from `mod_tools/` (the Blender add-on's tools from
`mod_tools/blender_plugin/`). The formats are in
`docs/game_data.md`; edited files are loaded through `assets/PATCH_DATA/`
(`docs/game_data.md`, "Patch data").

The part and node code lives in the Blender add-on's `sbx_formats/`
(plain Python and numpy; usable without Blender).

| Tool | Needs | What |
|---|---|---|
| `blender_plugin/` | Blender 5.2 | The Blender add-on for part files (car models): imports cars and parts, exports the visible parts back (untouched parts come out identical, edited shapes and materials are encoded). Has everything for it: the format code (`sbx_formats/`), tests, `build.py` (makes the zip to install), `car_models.py` / `car_models.xlsx` (every car's nodes and parts, every wheel), and the XPR2 texture lister (`python -m sbx_formats.xpr_textures FILE.xpr`, run from `blender_plugin/`). See `blender_plugin/README.md`. |
| `cut_car_wheels.py` | numpy | Applies the cut cars' wheel baselines (`docs/cut_car_wheels.md`): `cut_car_wheels.py BUILD_OUT OUT [--specs 00000072.bin]` writes the car specs with the new tire sizes and the ECLI, L86, S30 and T86's set-a and set-b node files with the tire nodes moved. Uses `blender_plugin/sbx_formats`. |
| `car_specs.py` | openpyxl | Exports the car specs (BUILD 72, `00000072.bin`) to an Excel workbook, one sheet per car, and builds them back: `export 00000072.bin car_specs.xlsx`, `build car_specs.xlsx 00000072.bin`. Fields not worked out yet are UNLABELED. The Bytes column updates as a value is edited, and each engine stage has a torque (kgm) and power (PS) chart that follows its curve. An unedited workbook builds back byte for byte. |
