# Status

The state of `main` (2026-10-25). Update this when a fix, experiment or
plan changes.

## Working

Boots and runs on Windows in two builds: the emulated GPU (`build.bat`,
`run.bat`; the SDK's Xenos emulation, the debugging reference) and the
native Vulkan renderer ported from ITC-Recomp (`build_vulkan.bat`,
`run_vulkan.bat`; `SBX_NATIVE_RENDERER=ON`, `docs/native-renderer-port.md`),
which also plays the movies through FFmpeg (`src/movies/`). Both use our
own audio output (`src/audio/`, `--native_audio`). The Vulkan build can
also anti-alias the race's 3D view with CMAA2 (`--cmaa2`,
`--cmaa2_threshold`) or FXAA (`--fxaa`), both working (the user's test,
2026-10-25), and blur it by blending frames away from the screen's centre
(`--blur_frame_blend`, off by default, with `--subtick` and a high `--max_fps`; not yet tested). Codegen is clean: 15,963
functions, no unresolved branches or calls. What else came over from
ITC-Recomp, and what's next: `docs/itc-parity.md`.

## Fixes

- The black screen after quest races: `sub_8218C240`, the post-race
  check, waits 1 cycle instead of 60 (`src/hooks/post_race.cpp`,
  `--post_race_check_cycles`, 1 by default; 60 is the game's).
- The race-end music (the tutorial's `bgm_tmp_08`) cut off with
  `--subtick`: the forced exit waits for the screen fades, which stepped
  once a frame; they step once a tick now (`src/subtick/per_frame_logic.cpp`).

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

- Subtick (the `60fps` branch): frames between the game's 30 Hz ticks in
  races (`--subtick`, `--max_fps`), the cars' controls fed input sampled
  each frame and weighted per tick (`--subtick_input`,
  `--subtick_input_sensitivity`), written into the control record so
  replays play back 1:1. Car physics stays at 30 Hz and two steps a tick.
  Cars, the camera, paint reflections, light glows, light trails and the
  opponent marker are blended between ticks. The race overlays (opponent
  marker, tire smoke) draw on every frame, paused or not. Open: wheel spin
  and steering step per tick; check the light sources on car models for the
  same camera-position shake the paint reflections had. Off by default (`src/subtick/`; "Subtick" in
  `docs/functions.md`).
- Tick rate experiments (`--tick_rate_60`, `--tick_rate_half_step`): removed
  2026-10-25; `src/tick_rate/` is to be deleted by hand. Their findings are in
  "Tick rate experiments" in `docs/functions.md`.

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

- The rest of ITC-Recomp's parity plan (`docs/itc-parity.md`): XACT
  natively, the SDK's kernel calls, the game's code natively. (XMA decoding
  through FFmpeg is in, both builds.)
- `--skip_cutscenes`'s wait once the fast-forward ends: the pacer's next tick
  is now clamped to one tick ahead (`subtick/frame_pacer.cpp`); to confirm.
- `--camera_fov` (the main camera's vertical FOV, Graphics/Experimental): works in the game (the user's test, 2026-10-25). Safe range 30 to 60 degrees (the game is about 47). Limitation: it changes every use of camera pass 0, so the replay and wanderer/boss cinematic camera tracks are affected too. Limiting it to racing is planned (`docs/functions.md`, "Main camera"), and the `--log_record_matrix` diagnostic is still in the code (off).
- A CI build.
