// The boot and the title screen's options for automated runs (ported from
// ITC-Recomp's src/hooks/title_screen.cpp): --skip_intro, --attract_after_s,
// --log_boot_modes.

#pragma once

namespace hooks {

// Whether the main loop's 30 Hz wait is skipped now: with --skip_intro, until
// the first race is drawn.
bool TitleScreen_SkipTickWait();

// Main thread: a race's draw step ran (ends --skip_intro's fast menus).
void TitleScreen_OnRaceDrawn();

// Render thread: Movie::RenderFrame (sub_820AE840) has returned
// (--skip_intro waits for it before ending the boot movie). Its hook is in
// title_screen.cpp, or in hooks/movies.cpp with the native renderer.
void TitleScreen_MovieRendered();

}  // namespace hooks
