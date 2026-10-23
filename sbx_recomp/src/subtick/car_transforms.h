// Interpolates the cars' world matrices and the main camera between the
// game's 30 Hz ticks (--subtick). See car_transforms.cpp.

#pragma once

#include <cstdint>
#include <vector>

#include "guest.h"

namespace subtick {

// A car's world matrix (car+16) as it was before it was blended, to put back.
struct CarTransformRestore {
  uint32_t address = 0;
  float saved[16] = {};
  float written[16] = {};  // the blend; put back only if the matrix still holds it
};

// Logic pass, from Car::DrawUpdate (car = r3 + 4288): records the car's
// matrix as this tick's pose, once per car per tick. Does nothing on
// render-only passes.
void CarTransforms_Record(uint8_t* base, uint32_t car);

// Before Race::DrawStep, on every pass in a race: blends the matrix of every
// car recorded this tick between its last two poses, by the frame's progress
// through the tick, and returns what it replaced. On a logic pass, cars whose
// matrix already holds the new tick's pose are recorded first.
std::vector<CarTransformRestore> CarTransforms_BlendAll(uint8_t* base);

// Puts back what CarTransforms_BlendAll replaced, after Race::DrawStep,
// unless the game has written the matrix since.
void CarTransforms_RestoreAll(uint8_t* base, const std::vector<CarTransformRestore>& restores);

// Around Car::DrawUpdate, after CarTransforms_Record: if the car's matrix
// isn't already blended (on a logic pass the game sets it during the draw),
// writes the blend for this draw. address is 0 if nothing was written.
// CarTransforms_DrawEnd puts the pose back.
CarTransformRestore CarTransforms_DrawBegin(uint8_t* base, uint32_t car);
void CarTransforms_DrawEnd(uint8_t* base, const CarTransformRestore& restore);

// Before Race::DrawStep on a render-only pass in a race: runs CameraPass::Update
// on pass 0 (built from the blend), as the skipped Update message does on a
// logic pass, so the cars and the race overlays are drawn with this frame's
// view-projection.
void CarTransforms_RebuildCamera(PPCContext& ctx, uint8_t* base);

// Main thread, at the start of each pass (before any game logic): puts the
// tick's eye, target, up and roll back into camera pass 0 if it still holds
// the blend written for the last frame.
void CarTransforms_EndFrame(uint8_t* base);

// For the CameraPass::Update wrapper.
bool CarTransforms_CameraEnabled();
void CarTransforms_CameraBegin(uint8_t* base);

}  // namespace subtick
