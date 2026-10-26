# Parity with ITC-Recomp

What ITC-Recomp (the US release, Import Tuner Challenge, recompiled with the
same SDK) changed, whether it applies to SBX, and in what order to bring it
over. Written 2026-10-24 from ITC-Recomp's `main` checkout at
`S:\Games\TXR_Modding\ITC_Recomp\git\ITC-Recomp` (its `src/`, its docs and
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
| Native Vulkan renderer, unlocked resolution (`--width`/`--height`), `--xbox_gamma`, `--motion_blur` | | `src/native/`, `SBX_NATIVE_RENDERER=ON` (`docs/native-renderer-port.md`) |
| The per-frame state finder (`--find_per_frame_state`) | | `src/debug/frame_state_diff.cpp` (Windows memory map, race-end mode) |

## Phase 1: game-logic fixes and tools (renderer-agnostic)

| ITC | What | SBX function(s) | Status |
|---|---|---|---|
| `per_frame_logic.cpp` `sub_825A0548` (`--per_tick_ui_movie_tracks`) | UI movies' adding animation tracks ran on every frame (the pause sub-menus' arrow slid ~5x as far) | `sub_82585C40` (calls 0.97; same code but float constants), table `0x82973088`/ready `0x82973080` | Ported: `src/subtick/per_frame_logic.cpp`, always on with `--subtick` (no cvar) |
| `hooks/title_screen.cpp` | `--skip_cutscenes`, `--attract_after_s`, `--log_boot_modes` | `82098DF0`, `820A4678`, `820AE840`, `82107D80` (exact), `8221FC48` (order 0.99; +36 and 600 checked); globals `0x8280DE3C`-`44` | Ported: `src/hooks/title_screen.cpp`. Changed from ITC: SBX's title screen plays its intro (Genki logos, warning) while it counts idle ticks, so the fast boot runs the title 590 ticks at once and then puts the idle count back to 0 (ITC kept the fast loop until the first race, which started the attract race straight away; with `--subtick` its load hung). `--log_boot_modes` also logs each handler set's state changes (`sub_82106410`) |
| `hooks/create_file.cpp` | `--log_file_opens` | `82292F48` (exact) | Ported: `src/hooks/create_file.cpp` |
| `hooks/voice.cpp` | Voice chat's 10 ms worker skipped (Xbox-only; CPU) | `825613F8` (order 0.87; the same code, checked) | Ported: `src/hooks/voice.cpp` (`--voice_thread=true` runs the original) |
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
   `output_mix.cpp`, `--native_audio`, on by default; the SDK's XMA
   decoder kept, the SDL DLL patch applied in both builds): our
   SDL3 output thread instead of the SDK's Audio Worker (~80% of a core in
   ITC's profile). SBX: `AudioRenderDriver::Register/Submit/Destroy`
   `8244D0A0`/`8244D128`/`8244D188` (already in `functions.md`).
2. **XMA decoding** (ported 2026-10-25, both builds: `src/audio/xma_stream.*`,
   `xma_packets.*` as ITC's, `src/hooks/xma.cpp`, `--native_xma`, on by
   default; working, the user's test 2026-10-25; FFmpeg's `xma2` from our build, now in both builds):
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
   same build step 2 uses. The emulated GPU build keeps the game's own
   movie library (it draws through the emulated GPU).
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

ITC translates the game's own subsystems to C++, verified by harnesses
(`src/harnesses/`): C runtime and math, heap and std::string, matrix math,
cameras, car physics, collision, track collision and route, model animation,
UI movies, race overlays, sprites and billboards, strings and font, data
tables and car specs, vehicle accessors, the command list, action mappers.
Speed and SDK removal, not fixes. Each needs every function and data address
remapped; the TSV covers functions, data needs `dataport.py`-style matching.
Large; after phases 1-3.

## Not brought over

- **ITC's interpolation** (`src/interpolation/`, the renderer's blend state):
  SBX's subtick does this in game logic, renderer-agnostic.
- **The rival logo cutscene fix** (`sub_821594B8`): ITC's cutscene; no SBX
  match.
- **Linux-only tooling** (evdev pad recording, `perf` profiles): kept as
  reference; pad replay would need a Windows input source first.
