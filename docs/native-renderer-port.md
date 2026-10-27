# Native renderer port (SBX)

Porting ITC-Recomp's native Vulkan renderer (`itc_recomp/src/native/`, on
its `rexglue-removal-project` branch, 2026-10-23) into `src/native/`. Plan:
`claude/sbx-native-vulkan-plan.md` in the SBX Recomp project. Address map:
`claude/sbx-native-renderer-address-map.md`.

The native renderer is the only build now (`build_vulkan.bat`). It was an option
(`SBX_NATIVE_RENDERER`, off by default) until 2026-10-26, when the emulated GPU
build was removed and the define renamed `SBX_VULKAN_RENDERER` (always set).

## The main finding: SBX's graphics library is a different revision

The game code matches ITC's closely, and so does the game's render command
list (the replay handlers all match). The Xbox 360 Direct3D library linked
into SBX does not: it's another XDK revision. ITC's renderer reads that
library's device structure and register shadow directly and wraps its
internal helpers, so that layer has to be re-derived for SBX, not renamed.

What differs (from the generated code, 2026-10-23):

- **Device header**: the command buffer pointer and end are at +80/+88
  (ITC +40/+48), so the header holds more than ITC's five dirty masks
  (+0 vertex constants, +8 pixel constants, +16, +24, +32 as in ITC).
- **Device fields**: shifted by region. From aligned library functions and
  the GPU kick (votes in brackets):

  | ITC | SBX | ITC name in native/ |
  |---|---|---|
  | 10768 | 10896 (27) | kDeviceProgressBlock |
  | 10772 | 10900 (13) | kDeviceCallbackBlock |
  | 10780 | 10908 (18) | kDeviceCommandPosition |
  | 10808 | 10928 (26) | kDeviceTilingFlags (bit meaning to check) |
  | 10809 | 10929 (39) | kDeviceFlags (ITC tests bit 30 of the word, SBX bit 29) |
  | 10820 | 10940 (5) | kDevicePrimaryWrite |
  | 10868 | 10988 (8) | kDevicePendingKicks |
  | 11552 | 11664 (1) | kDeviceVertexDeclaration (check) |
  | 11584 | 11696 (4) | kDeviceHizState |
  | 11600 | 11712 (3) | kDeviceScissorEnable |
  | 12180 | 12308 (draw) | kDeviceIndexBuffer |
  | 12184 | 12312 (14) | kDeviceRenderTarget0 |
  | 12200 | 12328 (18) | kDeviceDepthStencil |
  | 12416 | 12608 (6) | kDeviceScissorRect |
  | 12432 | 12720 (1) | kDevicePixelShader (check) |
  | 12436 | 12724 (2) | kDeviceVertexShader (check) |
  | 12488 | 12772 (7) | tile count (EndTiling) |
  | 12976 | 13200 (5) | (draw's packet block) |
  | 13496/13500 | 13716/13720 | primary ring base / size mask (kick) |
  | 15148 | 15336 (6) | kDeviceVblankCount |
  | 19980 | 20168 (13) | kick's object (+10809 bit set) |
  | 20124 | 20308 (5) | (kick) |

- **Register and constant shadow**: constants and fetch/bool/loop +128,
  register blocks +32. From `DrawIndexedVertices` (SBX `sub_825A3ED0`):

  | Block | ITC shadow | SBX shadow | SBX mask, lo, width |
  |---|---|---|---|
  | vertex float constants | 1792 | 1920 | +0 (all) |
  | pixel float constants | 5888 | 6016 | +8 (all) |
  | fetch constants 0x4800 (6 words) | 1024 | 1152 | +16, 32, 32 |
  | bool 0x4900 / loop 0x4908 | 9984 / 10016 | 10112 / 10144 | +16, 31, 1 (also sets HiZ +11696) |
  | 0x2000 x16 | 10240 | 10272 | +16, 14, 16 |
  | 0x2100 x21 | 10316 | 10348 | +24, 43, 21 |
  | 0x2180 x5 | 10400 | 10432 | +24, 38, 5 |
  | 0x2200 x12 | 10420 | 10452 | +24, 26, 12 |
  | 0x2280 x21 | 10468 | 10500 | +24, 5, 21 |
  | 0x2300 x38 | 10552 | 10584 | +32, 26, 38 |
  | 0x2380 x4 | 10704 | 10736 | +32, 18, 8 |

  (lo = 64 - rldicr shift - width, the bit that lands on top is unit 0.)
  ITC's table in `state.cpp` uses different masks and bits for almost every
  block, so the dirty-mask bookkeeping in `state.cpp`, `draw_packets.cpp`
  and `passes.cpp` has to follow this table.

- **Draw helpers**: ITC's draw calls eight flush helpers; SBX's calls a
  different set, in a different order:

  | ITC | SBX | What |
  |---|---|---|
  | `822ADE30` | `822AABF0` | register block flush (r4 mask, r5 first register, r6 shadow) |
  | `822AE1D0` | `822AADE8` | float constants flush |
  | `822AE070` | `822AACB0` | fetch constants flush |
  | `822AED18` | `822ABA90` | shader load (SBX: +32 bits 5-8; ITC: +16 bits 49-52) |
  | `822ADC08` (HiZ, register 0x2203) | none found | SBX stores HiZ inline in the draw |
  | `822ADEF0` (register blocks 2) | none found | |
  | | `822AAF10` (+32 bit 10) | not in ITC's draw; to identify |
  | | `822AAFF8` | BeginVertices only; to identify |

- **Draw entry points**, found through the matched command handlers:
  `825BE708`→`825A3ED0` (DrawIndexedVertices, command 18),
  `825BD8F0`→`825A31A0` (BeginVertices, command 19),
  `825BDDC0`→`825A35F0` (BeginIndexedVertices, command 27),
  `825BE300`→`825A3B00` (DrawVertices, command 17).
  `82304258` (ids 0, 36, 37, a 2-instruction stub) isn't a separate
  function in SBX.

## What this means for the port

Carries over as it is (no library layout in it): the Vulkan device, window,
swapchain and present (`renderer.cpp` minus its library reads),
`vulkan_functions`, `disk_cache`, `textures.cpp`'s texture cache and
untiling (fetch constants are the GPU's own format), the shader translation
(`shaders.cpp`, the SDK translator), the resolve shaders, `app_window`.

Has to be re-derived from SBX's library: `state.cpp`/`state.h` (shadow
table above, device fields), `draw_packets.cpp` (SBX's draw, its helpers
and mask bits), `gpu_link.cpp` (the kick, deferred kick, fences, interrupt
callback, ring and progress block offsets), `refcount.cpp` (AddRef/Release
patch sites `0x822A6794`/`0x822A6810`/`0x822A6818` are inside ITC's
functions), the device reads in `passes.cpp` and `buffers.cpp`, and the
replay hooks' library wrappers (inline draws, Resolve, vertex shader
patching).

## Done (2026-10-23: aiming at a first build)

- `src/native/`: ITC's renderer, every ITC address replaced by SBX's
  (function names, `__imp__` calls, the command handlers given to macros,
  and the data addresses: device pointer `0x8296E1F0`, command tables
  `0x8264EB40`/`0x8264EAA8`, constants `0x82000A24`/`0x82000940`).
- The library layer re-pointed with what's derived above: `state.h`
  device fields, `state.cpp`'s shadow table (SBX's blocks, masks and
  bits), `gpu_link.cpp`'s device fields (and the kick's other-path flag bit
  0x4), `passes.cpp`'s depth-stencil field, the tile count in
  `replay_hooks.cpp`, `draw_packets.cpp`'s helpers (SBX has no second
  register-block writer or HiZ writer) and fields. Each is marked with
  ITC's value in a comment. Unverified at runtime.
- `draw_packets.cpp`'s packet-free indexed draw is off
  (`--native_renderer_library_packets` defaults to true): it still follows
  ITC's mask layout. The library's own draw runs, and its flush helpers
  mark what the renderer reads again, which doesn't depend on the layout.
- Left out: `refcount.cpp` (a lock-free AddRef/Release, only a speed-up;
  patches return addresses inside ITC's functions), ITC's interpolation,
  replay statistics and native memcpy.
- Command 7 (set shader constants) wrapped like the other handlers.
- `src/base/` (log, flags, hash) and `src/helpers/executable.*` from ITC.
  The log takes over the SDK's file from `SbxApp::OnPostInitLogging`.
- Hooked in: `Renderer_OnListHandedOver` before the game's WaitForFrame,
  `Renderer_OnReplayStart`/`Renderer_EndFrame` around DrawFrame
  (`sub_82091FB0`), the vblank wait dropped, `TextureCache_RaceLeft` when a
  race ends, `AppWindow_SetSdkWindow` in `SbxApp::SetupPresentation`,
  `subtick::GameMessages_PassFromAnyThread` for the buffer logs.
- `CMakeLists.txt`: the SDK is built for
  Vulkan (not D3D12), `patches/rexglue-sdk-vulkan/` is applied (SDL as a
  DLL), `sbx_xenos_shaders` is built from the SDK's translator sources, and
  the SDL, Vulkan and xxHash headers are on the include path.

## Still to check

- The command handler matches below 0.8 (commands 5, 11, 22, 30), and the
  lower-confidence library matches in the project's address map.
- The device fields marked "to check", the tiling flag's bit, and the
  shader object fields (+24, +48, +600; the vertex shader patcher matches
  0.97, so they're probably the same).
- `sub_822AAF10` and `sub_822AAFF8`, SBX-only draw helpers.
- Running: `SDL3.dll` next to `sbx.exe`, no `--gpu_plugin`.

## Build notes (from ITC's Windows build, `docs/windows.md` there)

- The SDK must be configured with `REXGLUE_USE_VULKAN=ON` and
  `REXGLUE_USE_D3D12=OFF`: on Windows it defaults to D3D12, which leaves
  out the Vulkan headers (the first build's `vulkan/vulkan.h` errors).
  The headers are the SDK's own (`thirdparty/vulkan-headers`); the Vulkan
  SDK install isn't needed.
- SDL has to be a DLL on Windows (ITC's SDK patch
  `0003-itc-windows-sdl-dll.patch`), so the game and `rexruntime.dll`
  share one SDL, with SDL's Vulkan support on.
- The xenos shader translator is built from SDK sources into a static
  library of our own (`itc_xenos_shaders` in ITC's CMake); the SDK sources
  are the same files in SBX's SDK.
- ITC's support headers the native code includes: `base/log.h`,
  `base/flags.h`, `base/hash.h`, `helpers/executable.h`, and
  `frame_pacing/game_messages.h` (ours is `subtick/game_messages.h`).
  `debug/replay_stats.h` and `c_runtime/strings.h` go (diagnostics and
  ITC's native memcpy).
- Integration points (ITC's `frame_pacing/renderer_hooks.cpp`):
  `Renderer_OnListHandedOver` before the game's WaitForFrame,
  `Renderer_OnReplayStart`/`Renderer_EndFrame` around the render thread's
  DrawFrame (SBX `sub_82091FB0`), the vblank wait dropped (SBX already
  does with `--subtick`), `TextureCache_RaceLeft` when a race ends,
  `AppWindow_SetSdkWindow` in the app's `SetupPresentation`, and command 7
  wrapped like the other handlers (ITC wraps it in its interpolation code).
