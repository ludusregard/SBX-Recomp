# Status

The state of `main` (2026-10-19). Update this when a fix, experiment or
plan changes.

## Working

Boots and runs on Windows on the emulated GPU (`--gpu_plugin=xenos`).
Codegen is clean: 15,963 functions, no unresolved branches or calls.

## Fixes

- The black screen after quest races: `sub_8218C240`, the post-race
  check, waits 1 cycle instead of 60 (`src/hooks/post_race.cpp`,
  `--post_race_check_fix`, on by default).

## Experiments

- The cut cars (ECLI, S30, L86, T86) are offered from the start of a new
  game: their bits in the save's car unlock bits are set after
  `sub_82113FB0` and `sub_82114278` (`src/hooks/unlock_cut_cars.cpp`,
  `--unlock_cut_cars`, on by default). Their prices are still 999999.
- Patch loader: archive entries are read from loose files in `assets/PATCH_DATA/BUILD/TXR/BUILD/`
  and `assets/PATCH_DATA/BUILD/TXR/BUILD_CRS/` named by id with any extension.
  Example: `00000072.bin` and `00000072.dat` are both id `00000072` and would both be valid. 
  (`src/hooks/patch_loader.cpp`, `--patch_loader`, on by default; see
  `docs/game_data.md`, "Patch data"). Not yet tested.
- Cut car wheels: the ECLI gets its own stock wheel instead of AD00, and
  the L86, S30 and T86 the BOSS00's wheel (`--cut_car_wheels`), or the
  LORRY's steel wheels (`--cut_car_steelies`); both on by default
  (`src/hooks/cut_car_wheels.cpp`). Working in the garage. The wheel shop
  also offers them inch-ups (ECLI 3, the others 4); not yet tested.

## Mod tools

- `mod_tools/car_specs.py`: the car specs to an Excel workbook and back,
  byte for byte.
- The Blender add-on (`mod_tools/blender_plugin/`, 1.0.0): imports cars
  and parts, exports edited parts for `assets/PATCH_DATA/`
  (`mod_tools/blender_plugin/README.md`).
- `mod_tools/cut_car_wheels.py`: the cut cars' wheel baselines
  (`docs/cut_car_wheels.md`). Not yet tested in game.

## Docs

- `docs/functions.md`: named functions, mostly matched from ITC-Recomp's
  findings (`docs/porting-from-itc.md`), plus the data loading code.
- `docs/game_data.md`: the archives and data formats.
- `docs/modding-plan.md`: the patch and addon loaders, adding cars.
- `docs/cut_car_wheels.md`: the cut cars' wheel baselines.

## Not done yet

- Port more of the fixes and features ITC-Recomp has, starting with game
  compatibility. Its native Vulkan renderer, native audio and frame
  interpolation are on hold.
- A CI build.
