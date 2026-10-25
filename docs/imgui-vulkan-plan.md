# ImGui debug overlay in the Vulkan build (plan)

Written 2026-10-25, parked by the user's call (to do in a later session).
The emulated GPU build has the SDK's live ImGui overlay (settings toggled
while the game runs); the Vulkan build doesn't, because the overlay draws
through the SDK's window and presenter, and the native renderer hides that
window and presents to its own (`src/native/app_window.cpp`: "exclusive
mode", the SDK's window hidden, its events forwarded).

## What's there already

- ImGui is built and linked: the SDK's `imgui::imgui` target
  (`CMakeLists.txt` adds its include path for `rex_app.cpp`).
- The SDK's overlay is a set of `rex::ui::ImGuiDialog`s drawn by a
  `rex::ui::ImGuiDrawer` (see the commented `OnCreateDialogs` and
  `CreateAchievementsOverlay` in `src/sbx_app.h`), on the SDK's presenter.
- Our renderer owns the SDL3 window, the Vulkan device, queue and
  swapchain (`src/native/renderer.cpp`), and draws each frame's last pass
  (the gamma pass, `Passes::ApplyGamma`) before the present.
- Settings are cvars (`REXCVAR_*` in shared code, `ITC_FLAG_*` in the
  native renderer, which map to the same command line).

## Steps

1. **Back-ends.** Add ImGui's `imgui_impl_sdl3.cpp` and
   `imgui_impl_vulkan.cpp` (from the SDK's ImGui checkout, same version) to
   the native sources. One ImGui context of our own, made on the render
   thread after the swapchain: `ImGui_ImplSDL3_InitForVulkan(window)`,
   `ImGui_ImplVulkan_Init` with our instance, device, queue family, a
   small descriptor pool of its own, the swapchain's format and image
   count (dynamic rendering, as our passes use, so no render pass object).
   Remake its swapchain-dependent parts when the swapchain is remade
   (resizes).
2. **Input.** Our window's SDL events go to `ImGui_ImplSDL3_ProcessEvent`
   before they're forwarded to the SDK (`app_window.cpp`). A hotkey (F1, or
   the SDK's own overlay key, to match the emulated build) shows and hides
   it; while it's shown and ImGui wants the mouse or keyboard, those events
   don't reach the game.
3. **Drawing.** Each frame, after the gamma pass and before the present:
   `NewFrame`, our panels, `Render`, `ImGui_ImplVulkan_RenderDrawData` into
   the swapchain image, at the window's own resolution (not the game's
   scale).
4. **The panels.** First try reusing the SDK's dialogs: if `ImGuiDialog`'s
   draw only issues ImGui calls, call it inside our frame with our context
   current. If it's tied to the SDK window's drawer, write our own:
   - a settings panel listing the cvars (by group: SBX, the renderer, the
     SDK) with checkboxes, sliders and text fields, read and written
     through the cvar API (check whether `rex/cvar.h` can list them; if
     not, a table of the ones we care about);
   - the renderer's stats (frames/s, draws, the frame's costs), which are
     only logged now.
5. **Live vs start-only.** Mark which settings take effect while running
   and re-apply them each frame where the code reads them once:
   - live already: `--bloom`, `--motion_blur`, `--max_fps`, the logging switches;
   - start-only (greyed out, or "restart to apply"): `--width`,
     `--height`, `--fullscreen`, `--subtick`, `--native_audio`.
   The renderer copies some flags into `Passes` each frame
   (`renderer.cpp`, `set_motion_blur` and the like); others are read once
   at start and need the same treatment to be live.
6. **Thread safety.** The overlay runs on the render thread; cvars read by
   the main thread are plain values, so a toggle lands on the next read.
   Anything bigger (a resolution change) goes through a request the owning
   thread picks up.

## Size

A self-contained job: about a day with testing. Nothing in the game's code
changes; it's all in `src/native/` and the build.
