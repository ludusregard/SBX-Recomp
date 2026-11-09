# Parity with ITC-Recomp

What ITC-Recomp (the US release, Import Tuner Challenge, recompiled with the
same SDK) changed, whether it applies to SBX, and in what order to bring it
over. Written 2026-10-24 from ITC-Recomp's `main` checkout
(`<ITC-Recomp>`: its `src/`, its docs and
`itc.toml`), with ITC addresses mapped to ours through
`docs/itc_to_sbx_functions.tsv` (method and score given; "exact" and
"calls 1.00" are safe, anything else is checked in `generated/` before use).

The two games are the same code built twice, so ITC's game-logic fixes
apply here. The one deliberate difference is interpolation: ITC blends draw
state in its Vulkan renderer (`src/interpolation/`), SBX interpolates in
game logic (`src/subtick/`), so ours works with any renderer (the emulated
GPU, the native Vulkan one, or a future D3D one). Fixes that live in ITC's
renderer are taken only where they're renderer-agnostic in substance.

## Already in SBX

| ITC | SBX | Where |
|---|---|---|
| Per-frame logic: blink timers, texture animations, minimap blink, play time, UI movie replay, race overlays and their sprite recorders | `sub_820DC5C8`, `820F1038`, `820F5610`, `82107A38`, `8211D4B8`, `8215BE00`, `82127C08`, `821281B0` | `src/subtick/per_frame_logic.cpp` |
| Action mappers once a tick (`--input_mappers_by_tick`, the pause menu's repeat; `pause-menu-fix.md`) | `sub_8210B140` on logic passes only | `src/subtick/input.cpp` |
| Native Vulkan renderer, unlocked resolution (`--width`/`--height`), `--xbox_gamma`, `--motion_blur` | | `src/native/`, the only build (`build_vulkan.bat`; `docs/native-renderer-port.md`) |
| The per-frame state finder (`--find_per_frame_state`) | | `src/debug/frame_state_diff.cpp` (Windows memory map, race-end mode) |

## Phase 1: game-logic fixes and tools (renderer-agnostic)

| ITC | What | SBX function(s) | Status |
|---|---|---|---|
| `per_frame_logic.cpp` `sub_825A0548` (`--per_tick_ui_movie_tracks`) | UI movies' adding animation tracks ran on every frame (the pause sub-menus' arrow slid ~5x as far) | `sub_82585C40` (calls 0.97; same code but float constants), table `0x82973088`/ready `0x82973080` | Ported: `src/subtick/per_frame_logic.cpp`, always on with `--subtick` (no cvar) |
| `hooks/title_screen.cpp` | `--skip_cutscenes`, `--attract_after_s`, `--log_boot_modes` | `82098DF0`, `820A4678`, `820AE840`, `82107D80` (exact), `8221FC48` (order 0.99; +36 and 600 checked); globals `0x8280DE3C`-`44` | Ported: `src/hooks/title_screen.cpp`. Changed from ITC: SBX's title screen plays its intro (Genki logos, warning) while it counts idle ticks, so the fast boot runs the title 590 ticks at once and then puts the idle count back to 0 (ITC kept the fast loop until the first race, which started the attract race straight away; with `--subtick` its load hung). `--log_boot_modes` also logs each handler set's state changes (`sub_82106410`) |
| `hooks/create_file.cpp` | `--log_file_opens` | `82292F48` (exact) | Ported: `src/hooks/create_file.cpp` |
| `hooks/voice.cpp` | Voice chat's 10 ms worker skipped (Xbox-only; CPU) | `825613F8` (order 0.87; the same code, checked) | Ported: `src/hooks/voice.cpp`, always on (no cvar) |
| `hooks/lights.cpp` (`--native_nearest_lights`) | `Lights::FindNearest` natively: 13% of ITC's main thread | `820F3FF8` (calls 1.00; the same code but the w constant, `0x82000A20`) | Ported: `src/hooks/lights.cpp`, always on (no cvar) |
| `hooks/cockpit_dials.cpp` (`--native_cockpit_dials`) | Needles posed each frame from the tick's gauges | `820E3EB0`, `820E4178` | Not needed: it fixes ITC's renderer-side blending; SBX blends in game logic, so the dials are already right (the user's call, 2026-10-24) |
| The race-exit timer (new, not in ITC) | The forced exit waits for the screen fades (`sub_82166940` posts message 49 once the four fade counts at `0x82694DBC`/`0x82694DC4` are 0), and the fades (`Fade::Update`, `sub_8209D250`) step once a call of the per-frame logic `sub_82107A38`, from Draw: at 60 fps the exit came twice as soon and cut the race-end music | `sub_82107A38`, `sub_8209D250`, `sub_82166940`; found with `--find_race_exit_target` and `--log_boot_modes` (the exit is message 49, ~20 passes after the race-end message 34) | Fixed: `src/subtick/per_frame_logic.cpp` puts the fade counts (and a countdown at `0x825F180C`) back after render-only passes; the stopgap and the diagnostics used to find it removed (confirmed by the user, 2026-10-24) |

Also fixed while porting (2026-10-24): the rear-view mirror's cars
juddered with `--subtick` (only camera pass 0 was blended; every camera
pass is now, `subtick/car_transforms.cpp`). And: with the native renderer and `--subtick=false`
the vblank wait was dropped and nothing paced the game; it now waits for
the 30 Hz tick on the pacer (`subtick/renderer_hooks.cpp`).

## New at ITC (its pull of 2026-10-25)

| ITC | What | SBX | Plan |
|---|---|---|---|
| `hooks/shader_compiles.cpp`, `graphics/compiled_shaders.h` (`--dump_shader_compiles`; the hardcoded shaders are always on, no switch) | The game's HLSL compiler (D3DXCompileShader) no longer runs: its boot compiles (the font's, the primitive drawer's) are answered with the bytes they made, stored in a header. ~640 functions out of play | `sub_822B3678` (ITC `822B8F10`, exact), D3DXCreateBuffer `sub_822B8C18` (exact) | Ported 2026-10-25 (`src/hooks/shader_compiles.cpp`, both builds; dump and hook in one file, `tools/compiled_shaders.py` takes the length from the file name). Table made by the user's dump run (2026-10-25): the same 12 sources as ITC's, at SBX's addresses |
| The movie library out of play (`--play_movies_with_ffmpeg`) | ~580 functions | Done in the Vulkan build (Phase 2.3) | The emulated GPU build keeps the library |
| ITC's `--glow` (emulated GPU) and `--motion_blur` | Turning the bloom and the motion blur off | Glow::Record `sub_821364B0` (ITC `821379D8`, by the post effects' 0x1528 shift; c33.y from +8452 checked), Glow::SetStrength `sub_821359F0` | `--bloom` ported 2026-10-25 as game state (`src/hooks/post_effects.cpp`, both builds); `--motion_blur` (on/off) kept in the Vulkan build; a sample count was tried (2026-10-25: the game's draw repeated at shorter lengths) and dropped by the user's call, too subtle to matter. The bloom's targets already draw at the frame's scale (256x144 is 768x432 at 4K, in the user's log) |
| `native/refcount.cpp/.h` | The graphics library's AddRef/Release without the global lock (~6% of ITC's render thread) | SBX's `sub_822A3D58`/`sub_822A3DB8` (TSV: calls 0.60/0.75) are plain loads and stores, no lwarx/stwcx, so no lock is taken | Not needed (checked 2026-10-25); the unbuilt copy in `src/native/` is to be deleted |
| `framework/` (`--native_framework`) | The app framework (run loop, dispatch, main loop) in C++: 56 of 68 functions | `docs/functions/framework.md`'s 68 rows need the TSV | Phase 4; our `subtick/game_messages.cpp` replaces the same dispatch, so take it with care |
| `debug/coverage.cpp`, `tools/static_reachability.py`, `coverage_seen.py` | Which recompiled functions run in play (Linux clang only) | | For Phase 4's scale; needs a Windows equivalent first |

## Ours (not at ITC)

| What | Where | State |
|---|---|---|
| CMAA2 (Intel's, Apache 2.0) on the 3D view, `--cmaa2` (off) | `src/native/passes.cpp` (`ApplyCmaa2`), `src/native/glsl/cmaa2/CMAA2.hlsl` unmodified, `glsl/compile_cmaa2.py` patches and compiles it to `generated_cmaa2.h`; the device enables `robustImageAccess` (CMAA2 reads and writes past the edges) | Added 2026-10-25, Vulkan build only. Before the main view's second color resolve of a frame (`--aa_resolve`, 2: the whole scene, before the bloom, motion blur and combine read it): its 2x MSAA resolved, CMAA2's four kernels, the result drawn back into every sample. Only race frames resolve the main view (pitch 1040, 2x) twice; the garage draws its own 1280x720 4x MSAA view (tiled), not covered. The user's first run (2026-10-25) was boot and garage only, so it never ran. In races (four main-view color resolves a frame) both passes ran and found the right edges (`--aa_debug`), but their blends didn't show: the main view is HDR, stored at 8x, and an edge blended against a bright pixel stays bright. Both now work on the samples tonemapped into 0..1 (`glsl/aa_in.frag`) and undo it when writing back (`aa_out.frag`; CMAA2 compiled for LDR input); to test |
| FXAA, `--fxaa` (off) | `src/native/glsl/fxaa.frag` (after FXAA 3.11's quality algorithm, written here; luma from a tonemapped copy of the HDR values), `ApplyFxaa` | Added 2026-10-25, same place as CMAA2 (after it if both are on): one resolve of the samples and one full-screen draw back into them |
| Frame blending motion blur, `--blur_frame_blend` (the blend's strength, 0 = off, Graphics/Experimental), `--blur_frame_blend_center` (0.3), `--blur_frame_blend_edge` (0.7), `--blur_frame_blend_shutter` (1/8) | `src/native/glsl/frame_blend.frag`, `ApplyFrameBlend` | Added 2026-10-25, Vulkan build only, not yet built. A shutter as bxt.rs's Half-Life recorder blends sub-frames (https://bxt.rs/blog/motion-blur-for-half-life-video-recording-with-vulkan/), but sliding: each rendered frame shows the average of the last N (a ring of N layers, the main view's samples resolved into the newest), and as Halo 3's, kept off the centre (the current frame inside `center`, the average from `edge` out, radius 0 centre to 1 corners). Needs `--subtick` and a high `--max_fps` (blur length N / max_fps); at the AA's resolve, after it. Restarts when a frame has no main view. Per-object blur for free, since the frames are the game's own interpolated ones |

## Phase 2: native audio (the user's next step)

ITC's `docs/native-audio.md`. Each layer replaces SDK emulation with our own
code; none changes the game's audio logic.

1. **Render driver** (ported 2026-10-24: `src/audio/native_audio.cpp`,
   `output_mix.cpp`; always on since 2026-11-09, no flag; the SDK's XMA
   decoder kept, the SDL DLL patch applied in both builds): our
   SDL3 output thread instead of the SDK's Audio Worker (~80% of a core in
   ITC's profile). SBX: `AudioRenderDriver::Register/Submit/Destroy`
   `8244D0A0`/`8244D128`/`8244D188` (already in `functions.md`).
2. **XMA decoding** (ported 2026-10-25, both builds: `src/audio/xma_stream.*`,
   `xma_packets.*` as ITC's, `src/hooks/xma.cpp`; always on since
   2026-11-09, no flag; working, the user's test 2026-10-25; FFmpeg's `xma2` from our build, now in both builds):
   the game's XMA objects are ours, each sound decoded into a flat buffer the
   voice reads; no XMA contexts, so the SDK's XMA decoder sits idle. SBX's 23
   helpers `82445678`-`82446490` (ITC's `8245C638`-`8245D4D0`; the TSV's
   matches and the rest by comparing the asm, the same library), the XMA
   object at voice +180 (ITC +184). SBX has no frame walk (ITC
   `8246A5F8`), nor ITC's `8245CE10`/`8245D010`; its flush calls the input
   position query `82446018` (answered 0, logged once). SBX's banks (version 37, 40-byte entries) hold XMA1-style packets, with no frame count in the headers (ITC's XMA2 do): the first run cut sounds short and lost the music's loop points; frames are found by walking them (`xma_packets.cpp`). The real fault in the first runs (found 2026-10-25 from the third run's log against the bank files): the sounds' data is at 0xE0000000 and up (`FD8C6800`...), where guest memory sits 4 KB further on in host memory (`REX_PHYS_HOST_OFFSET`), and ITC's code read it with `base + address`: every sound was the 4 KB before its own (the wrong sound, silence, garbage). Now `REX_RAW_ADDR`. `Stream` also keeps its samples host-side, decodes only a little ahead of the reading (as the 360's output ring did), resolves the start and loop points once the data up to them is fed, and loops where the decoded samples end (`bgm_tmp_04`'s loop end is 290 samples past them); the voice reads from a staging copy. Checked offline on `SB_SYSTEM_SE` and `bgm_tmp_04`: every wave to its full length, the music looping. Left from ITC's B2:
   `hooks/audio_volume.cpp` (`824447B8`, order 0.87; the voice category
   volume imports), with phase 3.
3. **Movies through FFmpeg** (ported 2026-10-24, Vulkan build only, not yet
   built: `src/movies/`, `src/hooks/movies.cpp`, `cmake/ffmpeg.cmake`,
   `Passes::DrawMovie`; FFmpeg n9.0.2 and ITC's configs in
   `sbx_recomp/thirdparty/`; SBX's Movie methods checked against ITC's, seven
   exact; ITC's `docs/native-movies.md`, 2026-10-25):
   the game's `Movie` class (vtable slots 0-7) on a player of our own (the
   asf demuxer, wmv3 and wmav2 decoders), frames drawn by the native
   renderer (`Passes::DrawMovie`, a YUV shader), sound mixed into the
   output above. Needs ITC's upstream FFmpeg (a submodule pinned to a
   release, with its pre-made configs in `thirdparty/ffmpeg-config/`), the
   same build step 2 uses. The emulated GPU build (removed 2026-10-26) kept the
   game's own movie library (it drew through the emulated GPU).
4. **XACT and the sound cues natively** (`src/xact/`, `src/sound_cues/`):
   in progress at ITC (2026-10-23); follow it once it settles.

## Phase 3: the SDK's kernel calls (ReXGlue removal, B3-B6 in ITC)

Small, mostly exact matches; together they remove the SDK's kernel, file
and system emulation under the game.

| ITC | What | SBX matches |
|---|---|---|
| `graphics_device.cpp`, `graphics_drops.cpp`, `gamma.cpp`, `video_mode.cpp` | Graphics set-up without `Vd*` calls (B3); for the native renderer | mostly high; `gamma.cpp`'s two have no match |
| `file_class.cpp`, `archive_streaming.cpp`, `movie_files.cpp`, `saves.cpp` | Files, archives, movies, saves on our own code (B4) | file/archive/saves exact or high; `movie_files.cpp` 4 of 5 unmatched |
| `network.cpp`, `notify.cpp`, `user.cpp`, `profile.cpp`, `start_up.cpp`, `xmp.cpp`, `voice.cpp`, `memory_status.cpp` | Xbox-only system calls answered or dropped (B5, B6) | mostly exact; `xmp.cpp` unmatched |
| `time.cpp`, `clock.cpp`, `debug_output.cpp`, `object_attributes.cpp`, `last_error.cpp`, `strings.cpp` | System helpers natively | exact, `clock` one at calls 0.75 |

## Phase 4: native game code (ITC's B7)

ITC translates the game's own subsystems to C++, each behind a
`--native_*` flag whose adapters fall back to the recompiled function, and
checks them with harnesses (`src/harnesses/`, `--run_harnesses=<name>`)
that run the native and recompiled versions on the same inputs and compare.
Speed and SDK removal, not fixes.

### Review of 2026-11-08 (ITC `e20b6ef`..`565fde5`)

775 commits on ITC's `rexglue-removal-project` (2026-10-25 to 11-07); its
`main` has none since 2026-10-25. All but nine are its B7 loop (an agent
loop: map a subsystem, migrate it to C++, validate with its harness and an
attract race); the others are three merges, a docs commit, a car makers
harness fix and four commits on a "conventional renderer" (`docs/conventional-renderer.md` there: native code
feeding our own renderer interface instead of the game's Direct3D, resolves
dropped, swappable backends; the plan, and GPU data moved out of guest
memory). ITC counts 4,699
native functions (up from 705 on 2026-10-22), in 107 source folders
(about 11 MB of C++, 6 MB more in harnesses), with 682 adapters.

No gameplay fixes came with it, so nothing in phases 1-3 changed.

**How portable it is.** `tools/port_native.py` (new) rewrites every guest
address in ITC's source (`sub_` names, `__imp__sub_`, `0x8...` constants,
bare macro arguments, tables named `k8...`) through the function matches and
the data map, and grades each (A exact, B high, C check, D none; see the
script). Run over ITC's whole `itc_recomp/src` with the matches extended by
`match_funcs.py --want` for the addresses it uses (379 of the 527 functions
not in `itc_to_sbx_functions.tsv` found; only 125 of those at 0.95 or more):

| | A | B | C | D |
|---|---|---|---|---|
| Addresses (distinct per folder) | 10,601 | 8,914 | 2,700 | 708 |
| Native functions (`ITC_NATIVE_RUNNING`) | 1,973 | 1,637 | 549 | 85 |

- Outside the harnesses, 468 distinct addresses have no match; most are in
  `graphics`, `xact`, `xaudio`, `frame_passes`, `race_event` and
  `post_effects`. The graphics ones are the Direct3D library, a different
  revision here (`docs/native-renderer-port.md`), so they were expected.
- A native function graded A has the same instructions with addresses
  masked, so the same struct offsets and constants: its C++ is right here as
  it is. B and C mean the code differs somewhere (as `Race::DrawStep`'s +64
  vs +92); only the harnesses can say whether the C++ still holds.
- After the rewrite, 478 of ITC's 512 non-harness, non-renderer `.cpp` files
  pass a clang syntax check against SBX's generated headers and the SDK's
  (2026-11-08). The 34 that don't fail on the unmatched addresses, files
  SBX doesn't have yet (`main.cpp`, the movie player's headers) and the
  audio and graphics library adapters.
- 17 of ITC's overrides are functions SBX already overrides: the native
  renderer's library hooks (`sub_8229BDA8`, `8229C1F8`, `822AB028`,
  `822AC858`, `822B1450`, `822B1748`, `822B1888`, `82573538`, `82573638`,
  `82575148`, `8257ACD0`, `825A3AE0`), subtick's (`sub_82092E60`
  `Renderer::WaitForFrame`, `82106478` `GameMessages::Dispatch`, `82159540`
  `Race::DrawStep`, `820A3B98` `CameraPass::Update`) and the patch loader's
  (`sub_8209E348` `Archive::Init`). Each needs one merged version.
- The folders depend on each other: 34 of them (the C runtime, archive,
  cameras, collision, frame pacing, graphics...) include each other, so the
  native layer comes over as one piece, not folder by folder. The only
  folders that stand alone are `memory`, `helpers`, `command_list`,
  `game_data`, `menu_sounds`, `scene_target` and `std_list`.

### Done in SBX

Written as SBX's own code from its generated code, with ITC's as the
reference (`docs/native-code.md` has each flag's functions and what
differs from ITC; `docs/functions.md`, "Native game code"):

- 2026-11-08: `native_menu_sounds`, `native_data_files`, `native_framework`,
  `native_car_specs` (off by default).
- 2026-11-09: `native_menu_helpers`, `native_race_hud`, `native_data_tables`,
  `native_file_layer`, `native_track_zones`, `native_race_scene` (the
  user's test). Every flag on by default since then.
- 2026-11-09, batch 3 (to test): `native_shader_objects`,
  `native_screen_objects`, `native_scene_models`, `native_save_data`,
  `native_accessories`, `native_strings_and_font`,
  `native_screen_constructors`, `native_game_modes`. `race_flow` stays
  recompiled (RaceFlow::Init copies stack words it never writes).
- 2026-11-09, batch 4 (to test): `native_collision`, `native_car_physics`,
  `native_vehicle`, `native_race_states`, `native_material_passes`,
  `native_replay_snapshots`: each subsystem's set-ups, accessors, copies and
  simple per-tick functions; the physics' step and solver, the race's
  ticks and the replay's own save and restore stay recompiled.
- 2026-11-09, batch 5 (to test): `native_ui_movies`, `native_data_load`,
  `native_race_sounds`, `native_race_screens`, `native_race_hud_object`,
  `native_course_scenery`.
- 2026-11-09, batch 6 (to test): `native_scene_loaders`, `native_scene`,
  `native_second_set`, `native_load_poll`, `native_car_makers`,
  `native_texture_compression`, `native_save_interface`. `build_screen`
  has no flag (its functions are the build screen's lists and draws).

Batches 3 to 6 tested in play (the user, 2026-11-09). Held: memory,
std_list, std_string (ITC's heap); command_list, sprites, scene_target (the
renderer); helpers (the kernel calls, Phase 3).

Where ITC's folders cover an address SBX already overrides, SBX's override
stays and is the base. Each of the seven is a wrapper around the
recompiled function (it watches, logs or reorders the call); ITC's version
of each is the function's own work as C++ and adds no behavior of its own
(read 2026-11-09). Merged so far, the wrapper calling our C++
(`src/hooks/native_calls.h`) in place of the recompiled function, under its
subsystem's flag:

| Address | SBX's override | Ours, called from it | ITC's |
|---|---|---|---|
| `sub_82106410` StateMachine::Handle | `title_screen.cpp` (boot mode log, autosave) | `game_modes.cpp`, `native_game_modes` | `game_modes.cpp` |
| `sub_8220F7B0` RivalLogo::Draw | `subtick/per_frame_logic.cpp` (the logo on render-only passes) | `race_hud.cpp`, `native_race_hud` | `race_hud.cpp` |
| `sub_820F1038` TextureAnimations::Advance | `subtick/per_frame_logic.cpp` (skipped on render-only passes) | `course_scenery.cpp`, `native_course_scenery` | `course_scenery.cpp` |
| `sub_82113FB0` Progress::SetDefaults | `unlock_cut_cars.cpp` (after it) | not yet | `save_data.cpp`: its three table copies through ITC's memcpy, the bits |
| `sub_8228CEE8` GameData::Save | `autosave.cpp` (its log) | not yet | `save_layer.cpp`: the name's swprintf, the save request, on ITC's save layer |
| `sub_8211D4B8` UiMovie::Update | subtick (the movies' adds once a tick) | not yet | `ui_movies/update.cpp`: the queue and time steps |
| `sub_82585C40` UiMovie::ApplyTrack | subtick (the same) | not yet | `ui_movies/keys.cpp`: the keys' interpolation, float for float |

The last four go with their subsystems' per-tick work (the UI movies) or
ITC's layers SBX doesn't have yet (its save layer, its C runtime's
swprintf).

### Brought over, then taken out (2026-11-09)

ITC's native layer at `565fde5` was merged into `src/` on 2026-11-08 (917
files: the game code, its adapters and the harnesses) and taken out again
the next day. It wasn't what was asked for: ITC is a reference for how the
game works, not code to copy. Merged, it brought ITC's own diagnostics
(`--log_overlay_uploads` and others), crashed in its overrides with
`--ported_hooks` (QueryPerformanceCounter, `sub_8259F788`, given a null
output pointer) and stopped in the harnesses at the first unmatched
function (`xmp`, `sub_825894C8`).

What stays from it: the one `sub_82106410` override (StateTable::Dispatch)
in `hooks/title_screen.cpp`, which hands each dispatch to
`hooks::Autosave_OnDispatch` (autosave and the boot mode log both overrode
it), and `tools/port_native.py`, which rewrites ITC's source to SBX's
addresses for reading.

**How ITC is used now.** When we take on a subsystem: run `port_native.py`
on ITC's folder for it, read the C++ (and its harness) with SBX's addresses
in it, check what it says against SBX's own code and the game files, and
write SBX's version in SBX's style, behind its own flag. Nothing of ITC's
is compiled in. The table below says how cleanly each of ITC's folders
reads at SBX's addresses.

Regenerate the grading (from `sbx_recomp/`):

    python tools\match_funcs.py <ITC-Recomp>\itc_recomp\generated\default generated\default -o matches_want.tsv --want <every address port_native.py reports D>
    python tools\port_native.py <ITC-Recomp>\itc_recomp\generated\default generated\default matches_want.tsv <ITC-Recomp>\itc_recomp\src --report port_report.tsv --summary port_summary.md

Per folder, on 2026-11-08 (addresses and native functions as A/B/C/D;
Ready: "yes" every address A or B and every native function A, "harness"
nothing unmatched but some native functions below A, "no" something
unmatched):

| Subsystem | Addresses A/B/C/D | Native functions A/B/C/D | Ready |
|---|---|---|---|
| accessories | 31/23/2/0 | 20/11/1/0 | harness |
| action_mappers | 40/26/5/1 | 22/10/5/1 | no |
| ai_drivers | 109/106/24/4 | 47/82/24/3 | no |
| alt_hud | 34/19/2/1 | 15/6/0/0 | no |
| app_start_up | 109/109/19/1 | 19/15/3/0 | no |
| app_states | 108/28/1/2 | 37/16/1/0 | no |
| archive | 40/11/2/1 | 19/5/1/0 | no |
| audio | 3/0/2/1 | 0/0/0/0 | no |
| boot_mode | 36/20/4/2 | 12/6/1/0 | no |
| build_screen | 137/58/2/0 | 34/18/1/0 | harness |
| c_runtime | 107/53/37/3 | 35/12/2/1 | no |
| camera_modes | 102/225/35/9 | 36/40/6/6 | no |
| cameras | 53/37/15/12 | 12/19/3/2 | no |
| car_draw | 210/377/165/39 | 44/24/8/2 | no |
| car_makers | 45/35/5/0 | 9/10/3/0 | harness |
| car_models | 99/41/38/9 | 43/14/6/0 | no |
| car_physics | 151/197/19/0 | 32/94/11/0 | harness |
| car_scene_draw | 55/153/15/15 | 2/9/6/4 | no |
| car_sounds | 85/77/30/4 | 13/36/10/3 | no |
| car_specs | 38/9/1/0 | 32/7/0/0 | harness |
| career | 37/19/0/5 | 32/18/0/3 | no |
| collision | 39/44/7/0 | 23/37/4/0 | harness |
| command_list | 7/3/3/0 | 0/0/0/0 | yes |
| course_scenery | 109/35/4/0 | 52/9/0/0 | harness |
| data_load | 41/27/1/0 | 9/10/0/0 | harness |
| data_tables | 17/2/1/0 | 16/1/1/0 | harness |
| debug | 52/38/8/4 | 0/0/0/0 | no |
| display | 2/0/3/0 | 0/0/0/0 | yes |
| file_layer | 9/14/0/0 | 4/7/0/0 | harness |
| frame_pacing | 31/22/8/2 | 0/0/0/0 | no |
| frame_passes | 21/81/3/2 | 5/4/1/0 | no |
| framework | 45/32/19/0 | 29/24/3/0 | harness |
| front_end_car_draw | 42/31/7/4 | 9/9/1/0 | no |
| game_data | 1/0/0/0 | 1/0/0/0 | yes |
| game_modes | 29/57/3/0 | 14/10/3/0 | harness |
| graphics | 79/73/166/25 | 53/41/132/5 | no |
| harnesses | 2352/1854/557/114 | 0/0/0/0 | no |
| helpers | 15/6/1/0 | 3/0/1/0 | harness |
| hooks | 2418/2051/691/270 | 6/15/0/0 | no |
| input | 18/15/8/1 | 8/7/2/1 | no |
| interpolation | 5/12/0/0 | 0/0/0/0 | yes |
| load_poll | 49/29/0/0 | 25/7/0/0 | harness |
| location_screen | 110/106/20/1 | 31/25/3/0 | no |
| material_passes | 44/29/11/0 | 15/7/0/0 | harness |
| matrix_math | 18/13/10/6 | 11/9/2/3 | no |
| memory | 19/10/1/0 | 17/7/0/0 | harness |
| menu_helpers | 23/0/0/0 | 2/0/0/0 | yes |
| menu_sounds | 8/11/0/0 | 0/4/0/0 | harness |
| minimap | 13/37/2/1 | 3/10/1/0 | no |
| mode_tick | 44/32/3/2 | 9/7/0/0 | no |
| model_animation | 11/25/20/1 | 6/18/17/1 | no |
| models | 25/26/31/3 | 14/14/13/1 | no |
| movies | 7/2/0/0 | 7/0/0/0 | yes |
| native | 12/29/25/8 | 0/0/1/0 | no |
| post_effects | 57/105/8/1 | 22/32/2/1 | no |
| power_graph | 18/12/0/1 | 2/2/0/0 | no |
| race_event | 65/51/2/1 | 58/42/0/0 | no |
| race_flow | 5/10/0/0 | 0/1/0/0 | harness |
| race_hud | 4/0/0/0 | 2/0/0/0 | yes |
| race_hud_machine | 96/79/2/1 | 16/12/0/0 | no |
| race_hud_object | 79/81/8/0 | 27/24/1/0 | harness |
| race_loading | 25/13/12/1 | 6/3/1/0 | no |
| race_overlays | 80/85/16/8 | 44/55/9/8 | no |
| race_scene | 24/13/0/0 | 9/2/0/0 | harness |
| race_screens | 277/69/1/0 | 67/25/0/0 | harness |
| race_setup | 41/48/5/7 | 21/21/4/0 | no |
| race_sounds | 128/42/2/0 | 24/7/0/0 | harness |
| race_states | 70/37/0/0 | 30/13/0/0 | harness |
| race_update | 27/7/24/2 | 2/2/3/0 | no |
| race_world | 54/60/9/1 | 15/29/2/0 | no |
| racer_controllers | 30/24/3/1 | 9/18/1/0 | no |
| replay_snapshots | 21/30/3/0 | 12/13/1/0 | harness |
| save_data | 21/14/1/0 | 9/9/1/0 | harness |
| save_interface | 42/20/1/0 | 32/14/0/0 | harness |
| scene | 31/20/2/0 | 11/10/2/0 | harness |
| scene_loaders | 62/44/15/0 | 48/25/4/0 | harness |
| scene_models | 42/16/2/0 | 18/10/2/0 | harness |
| scene_target | 9/21/0/0 | 2/3/0/0 | harness |
| scenery_lists | 44/49/16/4 | 9/12/4/1 | no |
| screen_constructors | 9/59/7/0 | 4/33/4/0 | harness |
| screen_objects | 31/17/1/0 | 23/16/0/0 | harness |
| second_set | 138/120/2/0 | 96/57/0/0 | harness |
| shader_objects | 7/12/5/0 | 3/4/2/0 | harness |
| sound_cues | 19/16/2/3 | 13/14/2/3 | no |
| sprites | 12/11/2/0 | 0/0/0/0 | yes |
| stand_models | 38/18/1/1 | 18/9/0/0 | no |
| std_list | 2/3/0/0 | 2/3/0/0 | harness |
| std_string | 16/8/0/0 | 12/4/0/0 | harness |
| strings_and_font | 28/40/3/0 | 17/15/0/0 | harness |
| texture_compression | 56/69/3/0 | 16/7/1/0 | harness |
| track_collision | 48/26/3/1 | 24/19/1/0 | no |
| track_route | 41/10/0/1 | 33/7/0/0 | no |
| track_zones | 95/6/1/0 | 18/4/0/0 | harness |
| tuning_pages | 641/56/2/2 | 50/14/1/0 | no |
| tuning_screen | 183/434/21/31 | 40/31/3/1 | no |
| type67_modes | 176/191/21/3 | 46/43/4/1 | no |
| ui_movies | 108/63/5/0 | 51/26/1/0 | harness |
| user | 5/4/0/1 | 0/0/0/0 | no |
| vehicle | 53/41/10/0 | 9/13/3/0 | harness |
| vehicle_accessors | 66/67/9/1 | 52/67/9/1 | no |
| xact | 43/135/174/40 | 26/64/83/19 | no |
| xaudio | 90/189/260/43 | 38/69/121/14 | no |
| **all** | 10601/8914/2700/708 | 1973/1637/549/85 | |

## Not brought over

- **ITC's frame pacing, renderer, movies and audio** (`src/frame_pacing/`,
  `src/native/`, `src/movies/`, `src/audio/`): subtick, our native
  renderer, our movie player and our audio do these (Phase 4 above).
- **ITC's interpolation** (`src/interpolation/`, the renderer's blend state):
  SBX's subtick does this in game logic, renderer-agnostic.
- **The rival logo cutscene fix** (`sub_821594B8`): ITC's cutscene; no SBX
  match.
- **Linux-only tooling** (evdev pad recording, `perf` profiles): kept as
  reference; pad replay would need a Windows input source first.
