# Modding plan

A plan for mod support beyond today's file replacement, kept for later
reference. Nothing here is built yet except the mod loader. Function names
and data formats are in `docs/functions.md` and `docs/game_data.md`.

## Two loaders

Replacing something that exists and adding something new are different
problems, so they get separate loaders on a shared base.

**Patch loader** (today's `src/hooks/mod_loader.cpp`, renamed later):
replaces existing archive ids with loose files. It doesn't need to know
what the files mean, and it already works for anything from one file to
the whole Restored patch. It uses the game's own loose-file support: the
TOC entry is changed in memory after `Archive::Init`, and
`Archive::FindLooseFile` returns the file's path.

**Addon loader**: new content loaded beside the base game, like DLC. Two
layers:

1. **Archive layer.** Each addon's files get ids in a virtual range above
   `BUILD_CRS` (24,368 and up), served by hooking `Archive::Load` and
   `Archive::GetToc` for ids past the end. The XEX's archive counts and
   the `.TOC` files don't change.
2. **Registries.** New ids do nothing until the game asks for them. A
   new car only shows up if the car table, the shop lists, the name
   strings and the model lookup all know about it. Each kind of content
   (cars first; wheels and paints later) gets its own registry, filled
   from the addon's manifest.

Both loaders share one overlay layer that serves archive entries from
loose files (most of `mod_loader.cpp` today), with their own on/off
setting each.

## Design rules

- **Addons name their content; the loader assigns ids.** Ids are given
  out at load, so two addons can't both claim car 72 or the same archive
  id, and any set of addons can be installed together.
- **Load order: base game, addons, patches.** Patches win, so a patch can
  also fix an addon's files. Addons only add; replacing base files is
  what patches are for.
- **Addon state goes in a sidecar save**, keyed by addon name: owned
  addon cars, their unlocks, anything that doesn't fit the 129,200-byte
  save. The stock save stays untouched and compatible with the original
  game and Import Tuner Challenge. A save that references a missing addon
  must still load (skip the car, or fall back to a stock one).
- **Folder layout** to settle before the patch loader has many users.
  Keeping `MOD_DATA/BUILD/TXR/BUILD/` for patches moves nothing (the
  Restored submodule lives there). `MOD_DATA/PATCH/...` and
  `MOD_DATA/ADDONS/<name>/` read more clearly but move the submodule.

## Example: more cars

The car spec loader isn't the limit: `sub_820CBFE8` works out the car
count from the entry's size (size / 8,432, stored at params+56), so a
bigger `00000072.bin` loads through the patch loader. The limits are in
the code that uses car ids:

| Limit | Where | What breaks |
|---|---|---|
| 72 cars | About 15 functions compare against 72, including the starter/shop list builders (`sub_8217F2A0`, `sub_82272370`, `sub_8224A0E8`); not all of them are about cars. | Cars past 72 are never listed. |
| 96 cars | The save's car unlock bits are three words (`0x1985C`). | Ids 72-95 fit; 96 and up need more save space. |
| 23 road cars | `Params::GetCarParts` indexes the aero tables (74-82) by car id; they hold 23 records. | A new road car (car list 0) reads past the tables. |
| 45 models | Car models are fixed id runs in `BUILD` (45 cars x 226/197/55 ids). | New models need new archive ids. |
| Names, pictures, sounds | String table 22637, the menu XMDs, the engine sound byte. | Each needs an entry for the new car; how the game finds them isn't traced yet. |

In stages:

1. **Reuse empty slots (no code).** 11 unused BOSS slots (12-19, 21-23)
   and the 4 cut cars already have records, lists and unlock bits. A mod
   edits a record with `mod_tools/car_specs.py` and ships it through the
   patch loader. Only 45 cars have models, and the unused BOSS slots
   aren't among them, so a slot borrows or replaces an existing model
   (once the car id to model mapping is known).
2. **Car registry in the addon loader.** A mod car is a folder, e.g.
   `MOD_DATA/ADDONS/<name>/CARS/<car>/`, with a manifest (TOML or JSON),
   its spec record, models, textures and name strings. The recomp gives it
   an id from 72 up and serves it through hooks:
   - **Specs:** extend the table in memory after load.
   - **Lists and counts:** patch or hook the comparisons against 72 that
     mean "car count".
   - **Parts:** hook `Params::GetCarParts` so a mod car gets aero records
     from its folder, or a stock car's.
   - **Archive ids:** the virtual id range above.
   - **Save:** the sidecar save.

## What has to be traced first

- How a car id leads to its model block, name string, menu picture and
  engine sound.
- Which of the comparisons against 72 mean the car count.
- How a garage save slot (1,964 bytes) stores its car and tuning.

Logging hooks are the way in: hook the getters (`Params::GetCarSpec`,
`Car::Refresh`, `Archive::Load`), log every car id and archive id that
goes through them, and play through the menus and a race.

## Order of work

1. The patch/addon split as a refactor of `mod_loader.cpp` (shared
   overlay layer, two settings), with the folder layout decided.
2. The addon archive layer (virtual ids) with an empty registry, tested
   with a file no stock code asks for.
3. The tracing above.
4. The car registry.
