// Native game code called from other overrides (docs/native-code.md): where
// an address already has SBX's own override (a wrapper that watches or
// reorders the call), the wrapper stays as it is and calls one of these in
// place of the recompiled function. Each runs our own C++ with its
// subsystem's flag on, else the recompiled function, with the original's
// registers in and out.

#pragma once

#include "guest.h"

#include <cstdint>

namespace hooks {

// StateMachine::Handle (sub_82106410; --native_game_modes), from
// title_screen.cpp's override.
void NativeGameModes_Handle(PPCContext& ctx, uint8_t* base);

// RivalLogo::Draw (sub_8220F7B0; --native_race_hud), from
// subtick/per_frame_logic.cpp's override.
void NativeRaceHud_RivalLogoDraw(PPCContext& ctx, uint8_t* base);

// TextureAnimations::Advance (sub_820F1038; --native_course_scenery), from
// subtick/per_frame_logic.cpp's override.
void NativeCourseScenery_AdvanceTextureAnimations(PPCContext& ctx, uint8_t* base);

// SaveData::NewGame (sub_82113FB0; --native_save_data), from
// unlock_cut_cars.cpp's override.
void NativeSaveData_NewGame(PPCContext& ctx, uint8_t* base);

// SaveGame::Write (sub_8228CEE8; --native_save_interface), from
// autosave.cpp's override.
void NativeSaveInterface_Write(PPCContext& ctx, uint8_t* base);

// UiMovie::Update (sub_8211D4B8) and UiMovie::ApplyTrack (sub_82585C40;
// --native_ui_movies), from subtick/per_frame_logic.cpp's overrides.
void NativeUiMovies_Update(PPCContext& ctx, uint8_t* base);
void NativeUiMovies_ApplyTrack(PPCContext& ctx, uint8_t* base);

}  // namespace hooks
