# Status

The state of `main` (2026-11-09). Update this when a fix, experiment or
plan changes.

## Working

Boots and runs on Windows with the native Vulkan renderer ported from
ITC-Recomp (`build_vulkan.bat`, `run_vulkan.bat`, `docs/native-renderer-port.md`).
It's the only build since 2026-10-26: the emulated GPU build was removed (the
SDK's Xenos plugin is no longer built). It also plays the movies
through FFmpeg (`src/movies/`). It uses our own audio output (`src/audio/`, and its own XMA decoding, `src/hooks/xma.cpp`; neither has a flag since 2026-11-09). The Vulkan build can
also anti-alias the race's 3D view with CMAA2 (`--cmaa2`,
`--cmaa2_threshold`) or FXAA (`--fxaa`), both working (the user's test,
2026-10-25), and blur it by blending frames away from the screen's centre
(`--blur_frame_blend`, off by default, with `--subtick` and a high `--max_fps`; not yet tested). Codegen is clean: 15,963
functions, no unresolved branches or calls. What else came over from
ITC-Recomp, and what's next: `docs/itc-parity.md`.

Native game code (`docs/native-code.md`): the game's own subsystems
rewritten as our C++, each behind a `--native_<name>` flag (F4, the Overrides
tab, grouped by subsystem), tested by playing with the flag on and off; every flag is on
by default. Since 2026-11-09 the generated ones (set-ups, resets, setters,
copies, getters: 46 more files, 301 functions across 36 new flags and 10
earlier ones, not yet played) are checked against the recompiled functions
by `tools/native_gen/difftest` before they go in; 1,038 overrides in all.
In and tested (2026-11-09): `native_menu_sounds`,
`native_data_files`, `native_framework`, `native_car_specs`,
`native_menu_helpers`, `native_race_hud`, `native_data_tables`,
`native_file_layer`, `native_track_zones` and `native_race_scene`. Written
2026-11-09, to test: batch 3 (`native_shader_objects`,
`native_screen_objects`, `native_scene_models`, `native_save_data`,
`native_accessories`, `native_strings_and_font`,
`native_screen_constructors`, `native_game_modes`) and batch 4
(`native_collision`, `native_car_physics`, `native_vehicle`,
`native_race_states`, `native_material_passes`,
`native_replay_snapshots`) and batch 5 (`native_ui_movies`,
`native_data_load`, `native_race_sounds`, `native_race_screens`,
`native_race_hud_object`, `native_course_scenery`) and batch 6
(`native_scene_loaders`, `native_scene`, `native_second_set`,
`native_load_poll`, `native_car_makers`, `native_texture_compression`,
`native_save_interface`). Every override in
`src/` has a row in `docs/functions.md`
(`sbx_recomp/tools/check_function_statuses.py`: no errors).

## Fixes

- The black screen after quest races: `sub_8218C240`, the post-race
  check, waits 1 cycle instead of 60 (`src/hooks/post_race.cpp`,
  `--post_race_check_cycles`, 1 by default; 60 is the game's).
- The race-end music (the tutorial's `bgm_tmp_08`) cut off with
  `--subtick`: the forced exit waits for the screen fades, which stepped
  once a frame; they step once a tick now (`src/subtick/per_frame_logic.cpp`).
- A menu bitmap showing an old frame (one frame of the garage menu's
  animation showing the last race's view, or the garage's background),
  now and then: the native renderer kept sampling a resolve destination's
  image at an address after the game had freed it and put a texture
  there. A memcpy write over the start of a resolve destination not
  resolved into since drops it from the addresses draws sample
  (`TextureCache::State::MarkWritten`, `src/native/textures.cpp`;
  2026-11-09, not yet confirmed in play: the bug is rare). A button
  bitmap sometimes drawn as a faint quad in the sub-menus may be the same
  thing (its memory where a glow or blur target was); to watch.

## Experiments

- Built 2026-11-06, in the Windows builds since 2026-11-08; autosave and
  the G-buffer confirmed in play (the user, 2026-11-09); the language toggle
  works as far as it can without its files (to test once the patch's
  language files are in `assets/ADDON_DATA/LANGUAGES/`):
  - Language toggle, phase 1: `--language` (EN, JP, ...) and
    `--font_quality` (hd, sd) pick the files in `assets/ADDON_DATA/LANGUAGES/`
    at startup; language ids ignore `PATCH_DATA` (`src/hooks/patch_loader.cpp`,
    "Languages" in `docs/modding-plan.md`). Nothing changes until that folder
    holds files.
  - Autosave: `--autosave` (off by default) saves through the game's own save
    calls when back at the garage top after a shop or car menu, or after
    racing; a "Saved" notice on the overlay. `--log_autosave` logs the menu
    messages and save status values to confirm the ids
    (`src/hooks/autosave.cpp`, "Saving" in `docs/menus.md`). It watches the
    menus through the boot mode log's override of `sub_82106410`
    (`src/hooks/title_screen.cpp`): the first version overrode it too.
  - G-buffer lite: `--gbuffer` (off by default) builds the 3D view's depth
    (as drawn, and view distance) and the camera's motion vectors into our own
    buffers at its opaque resolve (`--gbuffer_resolve`, 1), from camera pass
    0's view-projection handed over with each command list.
    `--gbuffer_debug` 1-4 draws one over the frame (distance, motion, the
    game's own velocity buffer, raw depth). Nothing else uses the buffers yet
    (`BuildGBuffer` in `src/native/passes.cpp`, `glsl/gbuffer*.frag`).
- Saved settings: F4's Save button writes the settings changed in the menu to
  `sbx.toml` next to `sbx.exe`, which loads at every start; launch options
  still win over it (`src/base/settings_file.cpp`, "Settings" in
  `docs/build_run.md`). Not yet tested.
- The cut cars (ECLI, S30, L86, T86) are offered from the start of a new
  game: their bits in the save's car unlock bits are set after
  `sub_82113FB0` and `sub_82114278` (`src/hooks/unlock_cut_cars.cpp`,
  `--unlock_cut_cars`, on by default). Their prices are still 999999.
- Patch loader: archive entries are read from loose files in `assets/PATCH_DATA/BUILD/TXR/BUILD/`
  and `assets/PATCH_DATA/BUILD/TXR/BUILD_CRS/` named by id with any extension.
  Example: `00000072.bin` and `00000072.dat` are both id `00000072` and would both be valid. 
  (`src/hooks/patch_loader.cpp`, `--patch_loader`, on by default; see
  `docs/game_data.md`, "Patch data"). Not yet tested.
- Custom music: tracks from `assets/ADDON_DATA/CUSTOM_MUSIC/<CATEGORY>/`
  (ogg, opus, flac, mp3, wav) play in the place of the game's music by a
  roll at each music start (`--new_music_roll`, 0 by default, or a folder's
  `metadata.xml`), with weights and loop regions. Each folder's
  `metadata.xml` also weighs the game's own songs: weight 0 blocks one, and
  another song of the folder plays in its place. `--custom_music` (on by
  default), `--custom_music_log`, `--custom_music_shuffle` (on by default),
  `--custom_music_adaptive_volume` (on by default, loudness to -11.2 LUFS; restart to change),
  and `Volume` in `metadata.xml` (track levels) (`src/hooks/custom_music.cpp`, `src/audio/music/`,
  `docs/custom_music.md`). FFmpeg now also has the
  ogg, flac, mp3 and wav demuxers, their decoders and libswresample.
  Built on Windows and working in the game (2026-10-26): the menu songs,
  roaming, looping and shuffle. The category folders and a user guide
  (`CUSTOM_MUSIC_README.md`) are in `assets/ADDON_DATA/CUSTOM_MUSIC/`.
  The categories (MENU_COURSE, MENU_MAIN, MENU_GENERAL, MENU_STORY, ROAMING, BATTLE, BOSS, RACE_WIN, RACE_LOSE) are from the code
  ("Music" in `docs/audio.md`); BATTLE, BOSS, RACE_WIN and RACE_LOSE are
  still to confirm in play (each music start is logged with its caller).
- Cut car wheels: the ECLI gets its own stock wheel instead of AD00, and
  the L86, S30 and T86 the BOSS00's wheel (`--cut_car_wheels`), or the
  LORRY's steel wheels (`--cut_car_steelies`); both on by default
  (`src/hooks/cut_car_wheels.cpp`). Working in the garage. The wheel shop
  also offers them inch-ups (ECLI 3, the others 4); not yet tested.

- Subtick: frames between the game's 30 Hz ticks in
  races (`--subtick`, `--max_fps`), the cars' controls fed input sampled
  each frame and weighted per tick (`--subtick_input`,
  `--subtick_input_sensitivity`), written into the control record so
  replays play back 1:1. Car physics stays at 30 Hz and two steps a tick.
  Cars, the camera, paint reflections, light glows, light trails and the
  opponent marker are blended between ticks. The race overlays (opponent
  marker, tire smoke) draw on every frame, paused or not, in the rear-view
  mirror with the mirror's blended camera (fix for their judder there,
  2026-11-10; not yet confirmed in game). Open: wheel spin
  and steering step per tick; check the light sources on car models for the
  same camera-position shake the paint reflections had. On by default (`src/subtick/`; "Subtick" in
  `docs/functions.md`). The menus too (frames between ticks: no message
  delivered on them, the UI movies, the garage's car and camera, the
  parking area's 3D and the scrolling text drawn between ticks and blended;
  30 Hz for the tick a closing menu waits for its "closed" message; works,
  John's tests 2026-11-11; the `--subtick_menus` flag is gone). The UI
  movies' animations are drawn between ticks (works). The course's
  scrolling textures step once a tick and move smoothly between (works).
  Turning `--subtick` off mid-race works (it froze the game before).
  Camera cuts: tabled (2026-11-11). A camera breaking from its motion
  cuts instead of blending: about as good as the old 8 m test for the
  demo's cuts, but the garage's and the view button's switches are still
  blended between ticks (John's test). The HUD drawn for a frame after a
  race's result took it down: a race's frames between ticks now draw only
  the UI movies the tick drew; works (John's test, 2026-11-11).
- Tick rate experiments (`--tick_rate_60`, `--tick_rate_half_step`): removed
  2026-10-25; `cull_emulated_build.ps1` deletes `src/tick_rate/`. Their findings are in
  "Tick rate experiments" in `docs/functions.md`.

## Mod tools

- `sbxr_tools/car_specs.py`: the car specs to an Excel workbook and back,
  byte for byte.
- The Blender add-on (`sbxr_tools/blender_plugin/`, 1.0.0): imports cars
  and parts, exports edited parts for `assets/PATCH_DATA/`
  (`sbxr_tools/blender_plugin/README.md`).
- `sbxr_tools/cut_car_wheels.py`: the cut cars' wheel baselines
  (`docs/cut_car_wheels.md`). Not yet tested in game.

## Docs

- `docs/functions.md`: named functions, mostly matched from ITC-Recomp's
  findings (`docs/porting-from-itc.md`), plus the data loading code, and a
  row for every override in `src/`.
- `docs/native-code.md`: the native game code, by flag, and how to test it.
- `docs/menus.md`: the front end's screens and menu messages.
- `docs/audio.md` and `docs/custom_music.md`: sound, and custom music.
- `docs/itc-parity.md`: what came over from ITC-Recomp, and what hasn't.
- `docs/native-renderer-port.md`: the native renderer's port.
- `docs/game_data.md`: the archives and data formats.
- `docs/modding-plan.md`: the patch and addon loaders, adding cars.
- `docs/cut_car_wheels.md`: the cut cars' wheel baselines.

## Not done yet

- The rest of ITC-Recomp's parity plan (`docs/itc-parity.md`): XACT
  natively, the SDK's kernel calls. (Our own XMA decoding is in.)
- Native game code (`docs/native-code.md`): test batches 3 to 6 in play
  (each flag on and off); then the merge step (the addresses SBX's own
  overrides already hold, SBX's version the base), the held folders (the
  heap's, the renderer's, the kernel calls), and the larger functions each
  subsystem left recompiled (the physics' step and solver, the race's ticks,
  the movies' update and drawing).
- `--skip_cutscenes`'s wait once the fast-forward ends: the pacer's next tick
  is now clamped to one tick ahead (`subtick/frame_pacer.cpp`); to confirm.
- `--camera_fov` (the main camera's vertical FOV, Graphics/Experimental): works in the game (the user's test, 2026-10-25). Safe range 30 to 60 degrees (the game is about 47). Limitation: it changes every use of camera pass 0, so the replay and wanderer/boss cinematic camera tracks are affected too. Limiting it to racing is planned (`docs/functions.md`, "Main camera"), and the `--log_record_matrix` diagnostic is still in the code (off).
- Shader table: `--dump_shader_compiles` (the dump that wrote the table
  `graphics/compiled_shaders.h` is built from) was removed. The table stays;
  a shader it doesn't hold still goes to the real compiler and is logged.
  To remake the table, restore the dump branch from git history
  (`src/hooks/shader_compiles.cpp`, before its removal) first.
- A CI build.
