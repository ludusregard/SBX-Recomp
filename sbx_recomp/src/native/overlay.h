// The native renderer's ImGui overlay (docs/imgui-vulkan-plan.md): drawn
// onto the swapchain image after the frame is copied there, at the window's
// own resolution. F4 shows the settings and renderer panels (the SDK's own
// key for its settings overlay in the emulated GPU build), F3 a small
// frame-rate readout (the SDK's debug overlay key).
//
// Threads: events arrive on the main thread (the SDK's event loop, through
// an SDL event watch) and are queued; everything else runs on the render
// thread, which owns the ImGui context.

#pragma once

#include "vulkan_functions.h"

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>

namespace native::overlay {

struct InitInfo {
  SDL_Window* window = nullptr;
  uint32_t api_version = 0;
  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  uint32_t queue_family = 0;
  VkQueue queue = VK_NULL_HANDLE;
  VkFormat format = VK_FORMAT_UNDEFINED;  // the swapchain's
  uint32_t image_count = 2;               // the swapchain's
};

// Render thread, once the device and swapchain exist. Returns false (and
// logs) if the overlay can't run; the renderer carries on without it.
bool Init(const InitInfo& info);

// Render thread, when the swapchain is made again.
void OnSwapchain(VkFormat format, uint32_t image_count);

// Main thread (an SDL event watch): our window's input. F3 and F4 toggle the
// panels; the rest is queued for ImGui while a panel is shown.
void OnEvent(const SDL_Event& event, SDL_WindowID our_window);

// What the renderer measured this frame, for the panels.
struct FrameInfo {
  double frame_ms = 0;  // since the last present
  uint32_t commands = 0, draws = 0;
  VkExtent2D source{}, window{};
};

// Render thread, every frame: the frame-time history.
void OnFrame(const FrameInfo& frame);

// Render thread: whether anything is to be drawn this frame.
bool Visible();

// Render thread: builds this frame's panels and records them into `image`
// (`view`, `extent`), which is in COLOR_ATTACHMENT_OPTIMAL. Does nothing
// when nothing is shown.
void Draw(VkCommandBuffer commands, VkImageView view, VkExtent2D extent, const FrameInfo& frame);

}  // namespace native::overlay
