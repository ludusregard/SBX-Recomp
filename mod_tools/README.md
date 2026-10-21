# Mod tools

Tools for reading and editing the game's data files. Run with
`python <tool>` from `mod_tools/`. The formats are in
`docs/game_data.md`; edited files are loaded through `assets/MOD_DATA/`
(`docs/game_data.md`, "Mod data").

| Tool | Needs | What |
|---|---|---|
| `car_specs.py` | openpyxl | Exports the car specs (BUILD 72, `00000072.bin`) to an Excel workbook, one sheet per car, and builds them back: `export 00000072.bin car_specs.xlsx`, `build car_specs.xlsx 00000072.bin`. Fields not worked out yet are UNLABELED. The Bytes column updates as a value is edited, and each engine stage has a torque (kgm) and power (PS) chart that follows its curve. An unedited workbook builds back byte for byte. |
| `part_to_gltf.py` | numpy, Pillow | Converts part files (car models) to glTF binary (`.glb`) for Blender: single parts (`--xpr`, `--nodes` for skinned models), or a whole car block (`--car 892 --set a`, `--stock`, `--wheels 587 588 581`) with its node hierarchy, markers, textures, the tires' shape keys and the wheels at the tire nodes. See `docs/game_data.md`, "Part files". |
| `SBX_model.py` | numpy | The shared reader for node and part files, used by `part_to_gltf.py`. |
| `xpr_textures.py` | numpy | Lists the textures in an XPR2 and decodes them to PNG (DXT1/3/5, A8R8G8B8, tiled). |
