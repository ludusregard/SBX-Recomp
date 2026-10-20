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
- Mod loader: archive entries are read from loose files in `assets/MOD_DATA/BUILD/TXR/BUILD/`
  and `assets/MOD_DATA/BUILD/TXR/BUILD_CRS/` named by id with any extension.
  Example: `00000072.bin` and `00000072.dat` are both id `00000072` and would both be valid. 
  (`src/hooks/mod_loader.cpp`, `--mod_loader`, on by default; see
  `docs/game_data.md`, "Mod data"). Not yet tested.

## Docs

- `docs/functions.md`: named functions, mostly matched from ITC-Recomp's
  findings (`docs/porting-from-itc.md`), plus the data loading code.
- `docs/game_data.md`: the archives and data formats.

## Not done yet

- Port more of the fixes and features ITC-Recomp has, starting with game
  compatibility. Its native Vulkan renderer, native audio and frame
  interpolation are on hold.
- A CI build.
