// The native renderer's ImGui overlay (overlay.h).
//
// ImGui itself lives in the SDK's runtime DLL (built into rexruntime, every
// symbol exported); the SDL3 and Vulkan back-ends are built into the game
// (CMakeLists.txt). In the native renderer's (detached) mode the SDK makes no
// ImGui drawer of its own, so this context is the only one in use.

#include "overlay.h"

#include "base/flags.h"
#include "base/log.h"
#include "base/settings_file.h"
#include "generated_overlay_font.h"
#include "gpu_memory.h"
#include "hooks/patch_loader.h"

#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <rex/cvar.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace native::overlay {
namespace {

// Shown panels, toggled from the main thread's event watch.
std::atomic<bool> g_panels{false};  // F4: settings and renderer
std::atomic<bool> g_hud{false};     // F3: frame rate

// Our window's events for ImGui, queued on the main thread while the panels
// are shown. Text events point at SDL's temporary memory, so their text is
// copied.
struct QueuedEvent {
  SDL_Event event;
  std::string text;
};
std::mutex g_queue_mutex;
std::vector<QueuedEvent> g_queue;
constexpr size_t kMaxQueued = 1024;

// Render thread.
bool g_ready = false;
VkFormat g_format = VK_FORMAT_UNDEFINED;
VkInstance g_instance = VK_NULL_HANDLE;
bool g_panels_were_shown = false;

// Panel state.
char g_search[128] = "";
constexpr size_t kHistory = 240;
std::array<float, kHistory> g_frame_ms{};
size_t g_frame_ms_next = 0;

// The panel's accent, #cdeb43, in place of ImGui's blue.
constexpr ImVec4 kAccent{0.804f, 0.922f, 0.263f, 1.0f};
constexpr ImVec4 kWarning{1.0f, 0.75f, 0.3f, 1.0f};
constexpr ImVec4 kError{1.0f, 0.4f, 0.35f, 1.0f};

// Saving (settings_file.h). Flags changed in this menu are written by Save;
// the rest of the file is left alone, so a value from the command line or
// the environment isn't saved unless it's changed here.
std::set<std::string> g_changed;
// Values for init-only flags, which can't change while the game runs: kept
// here, saved, and used from the next start.
std::map<std::string, std::string> g_next_start;
bool g_unsaved = false;
std::string g_save_status;
bool g_save_failed = false;

// The settings' tabs, in order (the renderer's own tab comes after them).
constexpr std::array<const char*, 6> kTabs = {"Graphics", "Hooks",   "Overrides",
                                              "Logging",  "ReXGlue", "Vulkan"};

// Our flags that are read once (at startup, or when first used): changing
// them while the game runs does nothing until a restart. The SDK's flags
// carry their own lifecycle.
const std::set<std::string> kStartOnly = {
    "width",           "height",          "fullscreen",       "msaa",
    "gpu_plugin",      "cmaa2_threshold",
    "native_renderer_validation",         "native_renderer_dump",
    "play_movies_with_ffmpeg", "overlay_font",
    "language",        "font_quality",
};

// Short notices (Notify): shown at the top of the screen for a few seconds,
// whether or not the panels are open. Set from any thread.
std::mutex g_notice_mutex;
std::string g_notice;
std::chrono::steady_clock::time_point g_notice_until{};
constexpr auto kNoticeTime = std::chrono::seconds(3);

bool NoticeShown() {
  std::lock_guard lock(g_notice_mutex);
  return !g_notice.empty() && std::chrono::steady_clock::now() < g_notice_until;
}

// The window an input or window event is for (0 for others: the gamepads'
// and the like, which the overlay doesn't take).
SDL_WindowID EventWindow(const SDL_Event& event) {
  switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
      return event.key.windowID;
    case SDL_EVENT_TEXT_INPUT:
      return event.text.windowID;
    case SDL_EVENT_TEXT_EDITING:
      return event.edit.windowID;
    case SDL_EVENT_MOUSE_MOTION:
      return event.motion.windowID;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
      return event.button.windowID;
    case SDL_EVENT_MOUSE_WHEEL:
      return event.wheel.windowID;
    default:
      if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) {
        return event.window.windowID;
      }
      return 0;
  }
}

PFN_vkVoidFunction LoadVulkanFunction(const char* name, void*) {
  return vk::GetInstanceProcAddr()(g_instance, name);
}

void CheckResult(VkResult result) {
  if (result != VK_SUCCESS) {
    ITC_LOG_ERROR("native renderer: overlay: Vulkan error {}", int(result));
  }
}

ImGui_ImplVulkan_PipelineInfo PipelineInfo() {
  ImGui_ImplVulkan_PipelineInfo info{};
  info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  info.PipelineRenderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
  info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &g_format;
  return info;
}

std::string Lower(std::string text) {
  for (char& c : text) {
    c = char(std::tolower(static_cast<unsigned char>(c)));
  }
  return text;
}

// Recolors ImGui's blue elements with the accent, at the strengths that keep
// text readable over them. The title bar stays dark.
void ApplyAccent(ImGuiStyle& style) {
  // ImGui's dark theme is blue in places: those go to neutral greys first.
  for (ImVec4& shade : style.Colors) {
    if (shade.z > shade.x + 0.05f && shade.z > shade.y) {
      const float grey = (shade.x + shade.y + shade.z) / 3.0f;
      shade = ImVec4(grey, grey, grey, shade.w);
    }
  }
  // Panel backgrounds and the tabs: dark greys.
  auto grey = [&style](ImGuiCol which, float value, float alpha) {
    style.Colors[which] = ImVec4(value, value, value, alpha);
  };
  grey(ImGuiCol_FrameBg, 0.14f, 0.6f);
  grey(ImGuiCol_FrameBgHovered, 0.22f, 0.6f);
  grey(ImGuiCol_FrameBgActive, 0.28f, 0.7f);
  grey(ImGuiCol_Tab, 0.13f, 0.9f);
  grey(ImGuiCol_TabDimmed, 0.10f, 0.8f);
  auto tint = [&style](ImGuiCol which, float alpha) {
    style.Colors[which] = ImVec4(kAccent.x, kAccent.y, kAccent.z, std::min(alpha * 1.5f, 1.0f));
  };
  tint(ImGuiCol_ResizeGrip, 0.2f);
  tint(ImGuiCol_CheckMark, 1.0f);
  tint(ImGuiCol_SliderGrab, 0.8f);
  tint(ImGuiCol_SliderGrabActive, 1.0f);
  tint(ImGuiCol_Button, 0.25f);
  tint(ImGuiCol_ButtonHovered, 0.45f);
  tint(ImGuiCol_ButtonActive, 0.65f);
  tint(ImGuiCol_Header, 0.3f);
  tint(ImGuiCol_HeaderHovered, 0.45f);
  tint(ImGuiCol_HeaderActive, 0.6f);
  tint(ImGuiCol_SeparatorHovered, 0.6f);
  tint(ImGuiCol_SeparatorActive, 0.9f);
  tint(ImGuiCol_ResizeGripHovered, 0.6f);
  tint(ImGuiCol_ResizeGripActive, 0.9f);
  tint(ImGuiCol_TabHovered, 0.55f);
  tint(ImGuiCol_TabSelected, 0.45f);
  tint(ImGuiCol_TabSelectedOverline, 1.0f);
  tint(ImGuiCol_TabDimmedSelected, 0.3f);
  tint(ImGuiCol_TabDimmedSelectedOverline, 0.6f);
  tint(ImGuiCol_PlotLines, 1.0f);
  tint(ImGuiCol_PlotLinesHovered, 1.0f);
  tint(ImGuiCol_PlotHistogram, 0.8f);
  tint(ImGuiCol_PlotHistogramHovered, 1.0f);
  tint(ImGuiCol_TextSelectedBg, 0.35f);
  tint(ImGuiCol_NavCursor, 0.8f);
  tint(ImGuiCol_DragDropTarget, 0.9f);
  style.Colors[ImGuiCol_TitleBgActive] =
      ImVec4(kAccent.x * 0.2f, kAccent.y * 0.2f, kAccent.z * 0.2f, 1.0f);
}

// The font's size, in the window's units (the scale is applied on top).
constexpr float kFontSize = 15.0f;

bool LoadFont(ImGuiIO& io, float size) {
  ImFontConfig config;
  config.FontDataOwnedByAtlas = false;  // the built-in data isn't ImGui's to free

  if (ImFont* font = io.Fonts->AddFontFromMemoryTTF(
          const_cast<uint8_t*>(k_overlay_font),
          int(k_overlay_font_size), size, &config)) {
    io.FontDefault = font;
    return true;
  }

  ITC_LOG_WARN("native renderer: overlay: the built-in font didn't load; ImGui's built-in font");
  return false;
}

// Whether a flag only takes effect at the next start.
bool AppliesOnRestart(const rex::cvar::FlagEntry& entry) {
  return kStartOnly.count(entry.name) ||
         entry.lifecycle == rex::cvar::Lifecycle::kRequiresRestart ||
         entry.lifecycle == rex::cvar::Lifecycle::kInitOnly;
}

// A value chosen in the menu. Init-only flags keep it for the next start
// (g_next_start); the rest are set now.
void Apply(const rex::cvar::FlagEntry& entry, const std::string& value) {
  if (entry.lifecycle == rex::cvar::Lifecycle::kInitOnly) {
    std::string current = entry.getter ? entry.getter() : std::string();
    if (value == current) {
      g_next_start.erase(entry.name);
    } else {
      g_next_start[entry.name] = value;
    }
  } else if (!rex::cvar::SetFlagByName(entry.name, value)) {
    ITC_LOG_WARN("settings: {} = {} wasn't accepted", entry.name, value);
    return;
  }
  g_changed.insert(entry.name);
  g_unsaved = true;
}

// One flag's row: its name (with a marker if it only applies on restart),
// the description as a tooltip, an editor by its type, and a right-click
// menu to reset it.
void FlagRow(const rex::cvar::FlagEntry& entry) {
  bool restart = AppliesOnRestart(entry);
  bool init_only = entry.lifecycle == rex::cvar::Lifecycle::kInitOnly;
  auto next = g_next_start.find(entry.name);
  bool pending = next != g_next_start.end();
  ImGui::PushID(entry.name.c_str());
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  if (restart) {
    ImGui::TextColored(kWarning, "%s%s", entry.name.c_str(),
                       pending    ? " (next start)"
                       : init_only ? " (startup only)"
                                   : " (restart)");
  } else {
    ImGui::TextUnformatted(entry.name.c_str());
  }
  if (ImGui::IsItemHovered()) {
    std::string tip = entry.description;
    if (!tip.empty()) {
      tip += "\n";
    }
    tip += "Default: " + entry.default_value;
    using rex::cvar::Source;
    if (entry.source == Source::kCommandLine || entry.source == Source::kEnvironment) {
      tip += "\nSet by a launch option, which wins over the settings file.";
    }
    if (init_only) {
      tip += "\nRead once at startup: a change here is saved for the next start.";
    }
    tip += "\nRight-click to reset.";
    ImGui::SetTooltip("%s", tip.c_str());
  }
  if (ImGui::BeginPopupContextItem("reset")) {
    if (ImGui::MenuItem("Reset to default")) {
      Apply(entry, entry.default_value);
    }
    ImGui::EndPopup();
  }
  ImGui::TableNextColumn();
  ImGui::SetNextItemWidth(-1.0f);
  std::string value = pending ? next->second : entry.getter ? entry.getter() : std::string();
  using rex::cvar::FlagType;
  switch (entry.type) {
    case FlagType::Boolean: {
      bool on = value == "true" || value == "1";
      if (ImGui::Checkbox("##v", &on)) {
        Apply(entry, on ? "true" : "false");
      }
      break;
    }
    case FlagType::Int32:
    case FlagType::Int64:
    case FlagType::Uint32:
    case FlagType::Uint64: {
      int number = std::atoi(value.c_str());
      if (ImGui::InputInt("##v", &number, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (entry.constraints.min) {
          number = std::max(number, int(*entry.constraints.min));
        }
        if (entry.constraints.max) {
          number = std::min(number, int(*entry.constraints.max));
        }
        Apply(entry, std::to_string(number));
      }
      break;
    }
    case FlagType::Double: {
      double number = std::atof(value.c_str());
      if (ImGui::InputDouble("##v", &number, 0.0, 0.0, "%.4f",
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
        Apply(entry, std::to_string(number));
      }
      break;
    }
    default: {
      // --language: the folders found in ADDON_DATA/LANGUAGES/, by their
      // language.xml names; --font_quality: hd or sd.
      if (entry.name == "language" || entry.name == "font_quality") {
        std::vector<std::pair<std::string, std::string>> options;  // value, label
        if (entry.name == "language") {
          options.push_back({"", "Automatic (EN if found, else JP)"});
          bool has_jp = false;
          for (const patch_loader::Language& language : patch_loader::Languages()) {
            options.push_back({language.code, language.name + " (" + language.code + ")"});
            has_jp |= language.code == "JP" || language.code == "jp";
          }
          if (!has_jp) {
            options.push_back({"JP", "Japanese (JP, the disc's)"});
          }
        } else {
          options = {{"hd", "HD"}, {"sd", "SD"}};
        }
        std::string shown = value;
        for (const auto& [option, label] : options) {
          if (option == value) {
            shown = label;
          }
        }
        if (ImGui::BeginCombo("##v", shown.c_str())) {
          for (const auto& [option, label] : options) {
            if (ImGui::Selectable(label.c_str(), option == value)) {
              Apply(entry, option);
            }
          }
          ImGui::EndCombo();
        }
        break;
      }
      if (!entry.constraints.allowed_values.empty()) {
        if (ImGui::BeginCombo("##v", value.c_str())) {
          for (const std::string& option : entry.constraints.allowed_values) {
            if (ImGui::Selectable(option.c_str(), option == value)) {
              Apply(entry, option);
            }
          }
          ImGui::EndCombo();
        }
        break;
      }
      // Strings, including our float settings (cmaa2_threshold and the
      // like): applied on Enter.
      char buffer[256];
      std::snprintf(buffer, sizeof(buffer), "%s", value.c_str());
      if (ImGui::InputText("##v", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        Apply(entry, buffer);
      }
      break;
    }
  }
  ImGui::PopID();
}

// A table of flags: a setting and its value per row.
void FlagTable(const std::vector<const rex::cvar::FlagEntry*>& flags) {
  if (!ImGui::BeginTable("flags", 2,
                         ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                             ImGuiTableFlags_SizingStretchProp)) {
    return;
  }
  ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthStretch, 0.55f);
  ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.45f);
  for (const rex::cvar::FlagEntry* entry : flags) {
    FlagRow(*entry);
  }
  ImGui::EndTable();
}

// Where a flag shows: its tab and its group within it.
struct Place {
  std::string tab;
  std::string group;
};

// The Overrides tab's group for a native code flag (--native_<subsystem>,
// docs/native-code.md), by the subsystem's first word: a new flag named as
// the others are lands in its group with nothing added here.
const char* OverrideGroup(std::string_view subsystem) {
  static const std::pair<const char*, std::array<std::string_view, 12>> kGroups[] = {
      {"Race", {"race", "replay", "track", "racer", "ai", "minimap", "mode", "nearest"}},
      {"Cars", {"car", "vehicle", "collision", "accessories", "tuning", "power", "stand", "front"}},
      {"Scene",
       {"scene", "second", "course", "material", "shader", "texture", "model", "models",
        "scenery", "camera", "cameras", "post"}},
      {"Menus and UI",
       {"menu", "screen", "ui", "strings", "game", "location", "alt", "type67", "build",
        "career", "boot", "movies"}},
      {"Data and Saves", {"data", "file", "load", "save", "saves", "archive", "user"}},
      {"Audio", {"sound", "audio", "xact", "xaudio"}},
      {"Runtime",
       {"c", "std", "framework", "heap", "memory", "helpers", "input", "action", "app",
        "frame", "matrix", "display"}},
  };
  const std::string_view first = subsystem.substr(0, subsystem.find('_'));
  for (const auto& [group, words] : kGroups) {
    if (std::find(words.begin(), words.end(), first) != words.end()) {
      return group;
    }
  }
  return "Other";
}

// Our flags all share one SDK category ("SBX"), so they're placed by name
// here. The SDK's categories ("GPU/Vulkan" and the like) are placed by theirs.
// A new flag of ours shows under Hooks/Other until it's named here.
Place PlaceFlag(const rex::cvar::FlagEntry& entry) {
  static const std::map<std::string, Place> kNamed = {
      {"msaa", {"Graphics", "Anti-Aliasing"}},
      {"cmaa2", {"Graphics", "Anti-Aliasing"}},
      {"cmaa2_threshold", {"Graphics", "Anti-Aliasing"}},
      {"fxaa", {"Graphics", "Anti-Aliasing"}},
      {"aa_debug", {"Graphics", "Anti-Aliasing"}},
      {"aa_resolve", {"Graphics", "Anti-Aliasing"}},
      {"motion_blur", {"Graphics", "Post-Process"}},
      {"bloom", {"Graphics", "Post-Process"}},
      {"xbox_gamma", {"Graphics", "Post-Process"}},
      {"subtick", {"Graphics", "Frame Rate"}},
      {"subtick_input", {"Graphics", "Frame Rate"}},
      {"subtick_input_sensitivity", {"Graphics", "Frame Rate"}},
      {"max_fps", {"Graphics", "Frame Rate"}},
      {"width", {"Graphics", "Window"}},
      {"height", {"Graphics", "Window"}},
      {"fullscreen", {"Graphics", "Window"}},
      {"camera_fov", {"Graphics", "Camera"}},
      {"blur_frame_blend", {"Graphics", "Experimental"}},
      {"blur_frame_blend_center", {"Graphics", "Experimental"}},
      {"blur_frame_blend_edge", {"Graphics", "Experimental"}},
      {"blur_frame_blend_shutter", {"Graphics", "Experimental"}},
      {"per_tick_post_race_check", {"Hooks", "Fixes: Race"}},
      {"post_race_check_cycles", {"Hooks", "Fixes: Race"}},
      {"per_tick_car_draw_flag", {"Hooks", "Fixes: Interpolation"}},
      {"per_tick_cockpit_dials", {"Hooks", "Fixes: Interpolation"}},
      {"redraw_rival_logo", {"Hooks", "Fixes: Interpolation"}},
      {"skip_cutscenes", {"Hooks", "Title"}},
      {"attract_after_s", {"Hooks", "Title"}},
      {"unlock_cut_cars", {"Hooks", "Cars"}},
      {"cut_car_wheels", {"Hooks", "Cars"}},
      {"cut_car_steelies", {"Hooks", "Cars"}},
      {"audio_maxqframes", {"Hooks", "Audio"}},
      {"audio_mute", {"Hooks", "Audio"}},
      {"custom_music", {"Hooks", "Audio"}},
      {"new_music_roll", {"Hooks", "Audio"}},
      {"custom_music_shuffle", {"Hooks", "Audio"}},
      {"custom_music_log", {"Hooks", "Audio"}},
      {"custom_music_adaptive_volume", {"Hooks", "Audio"}},
      {"play_movies_with_ffmpeg", {"Hooks", "Movies"}},
      {"patch_loader", {"Hooks", "Patches"}},
      {"language", {"Hooks", "Language"}},
      {"font_quality", {"Hooks", "Language"}},
      {"autosave", {"Hooks", "Saves"}},
      {"log_autosave", {"Logging", "Game"}},
      {"half_pixel_offset", {"Vulkan", "Renderer"}},
      {"dump_shaders", {"Logging", "Native Renderer"}},
      {"gbuffer", {"Graphics", "G-Buffer"}},
      {"gbuffer_resolve", {"Graphics", "G-Buffer"}},
      {"gbuffer_debug", {"Graphics", "G-Buffer"}},
      {"gbuffer_debug_depth_range", {"Graphics", "G-Buffer"}},
      {"gbuffer_debug_velocity_scale", {"Graphics", "G-Buffer"}},
      {"log_boot_modes", {"Logging", "Game"}},
      {"log_file_opens", {"Logging", "Game"}},
      {"log_bgm", {"Logging", "Game"}},
      {"log_message_handlers", {"Logging", "Game"}},
      {"log_race_overlays", {"Logging", "Game"}},
      {"log_render_only_draws", {"Logging", "Native Renderer"}},
      {"log_gpu_memory", {"Logging", "Native Renderer"}},
      {"log_record_matrix", {"Logging", "Debugging"}},
      {"find_per_frame_state", {"Logging", "Debugging"}},
      {"find_per_frame_state_interval_ms", {"Logging", "Debugging"}},
      {"watch_addresses", {"Logging", "Debugging"}},
      {"find_any_steps", {"Logging", "Debugging"}},
      {"find_countdowns", {"Logging", "Debugging"}},
      {"log_cockpit_draw", {"Logging", "Debugging"}},
  };
  if (auto it = kNamed.find(entry.name); it != kNamed.end()) {
    return it->second;
  }
  const std::string& name = entry.name;
  if (name == "native_renderer_disk_cache" || name == "native_renderer_skip_repeated_state" ||
      name.starts_with("native_renderer_cache_") || name.starts_with("native_renderer_reuse_")) {
    return {"Vulkan", "Caching"};
  }
  if (name == "native_renderer_validation" || name.starts_with("native_renderer_check_") ||
      name.starts_with("native_renderer_verify_")) {
    return {"Vulkan", "Validation"};
  }
  if (name.starts_with("native_renderer_log_")) {
    return {"Logging", "Native Renderer"};
  }
  if (name.starts_with("native_renderer_")) {
    return {"Vulkan", "Debugging"};
  }
  if (name.starts_with("native_")) {
    return {"Overrides", OverrideGroup(std::string_view(name).substr(7))};
  }
  // The SDK's window, presenter, GPU emulation and audio driver settings: our
  // renderer and audio replace those parts, so these change nothing here and
  // aren't shown. (Those of them our code does read are named above.)
  static const std::set<std::string> kUnused = {
      "UI/Window", "Display",    "UI/Presenter", "UI/Vulkan", "UI/D3D12",
      "GPU",       "GPU/Vulkan", "GPU/D3D12",    "GPU/Shader", "Audio",
  };
  if (kUnused.count(entry.category)) {
    return {};
  }
  if (entry.category == "SBX") {
    return {"Hooks", "Other"};
  }
  return {"ReXGlue", entry.category};
}

// A tab's flags by group; each group's flags are sorted by name. Commands
// (actions, not settings) are left out.
using Groups = std::map<std::string, std::vector<const rex::cvar::FlagEntry*>>;

std::map<std::string, Groups> Layout() {
  std::map<std::string, Groups> tabs;
  for (const rex::cvar::FlagEntry& entry : rex::cvar::GetRegistry()) {
    if (entry.type == rex::cvar::FlagType::Command) {
      continue;
    }
    Place place = PlaceFlag(entry);
    if (place.tab.empty()) {
      continue;
    }
    tabs[place.tab][place.group].push_back(&entry);
  }
  // By name, but for these, in this order at the top of their group.
  static const std::map<std::string, int> kFirst = {{"width", 0}, {"height", 1}};
  auto rank = [](const rex::cvar::FlagEntry* e) {
    auto it = kFirst.find(e->name);
    return it != kFirst.end() ? it->second : int(kFirst.size());
  };
  for (auto& [tab, groups] : tabs) {
    for (auto& [group, flags] : groups) {
      std::sort(flags.begin(), flags.end(), [&](const auto* a, const auto* b) {
        return std::pair(rank(a), a->name) < std::pair(rank(b), b->name);
      });
    }
  }
  return tabs;
}

// A tab's groups, each a collapsible header over its table.
void GroupedFlags(const std::string& tab, const Groups& groups) {
  ImGui::PushID(tab.c_str());
  for (const auto& [group, flags] : groups) {
    ImGui::PushID(group.c_str());
    if (ImGui::CollapsingHeader(group.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
      FlagTable(flags);
    }
    ImGui::PopID();
  }
  ImGui::PopID();
}

// Every setting whose name or description has the search text, across tabs.
void SearchResults(const std::string& search) {
  std::vector<const rex::cvar::FlagEntry*> found;
  for (const rex::cvar::FlagEntry& entry : rex::cvar::GetRegistry()) {
    if (entry.type == rex::cvar::FlagType::Command) {
      continue;
    }
    if (PlaceFlag(entry).tab.empty()) {
      continue;
    }
    if (Lower(entry.name).find(search) != std::string::npos ||
        Lower(entry.description).find(search) != std::string::npos) {
      found.push_back(&entry);
    }
  }
  std::sort(found.begin(), found.end(),
            [](const auto* a, const auto* b) { return a->name < b->name; });
  ImGui::Text("%zu settings match", found.size());
  FlagTable(found);
}

// Writes the flags changed in this menu to the settings file.
void SaveSettings() {
  std::vector<settings_file::Entry> entries;
  bool restart = false;
  for (const rex::cvar::FlagEntry& entry : rex::cvar::GetRegistry()) {
    if (entry.type == rex::cvar::FlagType::Command || !g_changed.count(entry.name)) {
      continue;
    }
    auto next = g_next_start.find(entry.name);
    std::string value = next != g_next_start.end() ? next->second
                        : entry.getter              ? entry.getter()
                                                    : std::string();
    Place place = PlaceFlag(entry);
    settings_file::Entry& saved = entries.emplace_back();
    saved.name = entry.name;
    saved.value = settings_file::TOMLValue(entry.type, value);
    saved.is_default = value == entry.default_value;
    saved.section = place.tab + " / " + place.group;
    restart |= AppliesOnRestart(entry);
  }
  std::string file = settings_file::Path().filename().string();
  std::string error;
  if (!settings_file::Save(entries, error)) {
    ITC_LOG_ERROR("settings: {}", error);
    g_save_status = "Not saved: " + error;
    g_save_failed = true;
    return;
  }
  g_unsaved = false;
  g_save_failed = false;
  g_save_status = entries.empty() ? "Nothing changed to save"
                                  : "Saved to " + file +
                                        (restart ? " (marked settings apply after a restart)" : "");
}

// The Save button and what happened last.
void SaveBar() {
  std::string file = settings_file::Path().filename().string();
  if (ImGui::Button("Save")) {
    SaveSettings();
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Writes the settings changed here to %s next to sbx.exe,\n"
                      "loaded at every start. Launch options still win over it.",
                      file.c_str());
  }
  ImGui::SameLine();
  if (g_unsaved) {
    ImGui::TextColored(kAccent, "Unsaved changes");
  } else if (!g_save_status.empty()) {
    if (g_save_failed) {
      ImGui::TextColored(kError, "%s", g_save_status.c_str());
    } else {
      ImGui::TextDisabled("%s", g_save_status.c_str());
    }
  } else {
    ImGui::TextDisabled("Settings file: %s", file.c_str());
  }
}

float AverageFrameMs() {
  float sum = 0;
  int count = 0;
  for (float ms : g_frame_ms) {
    if (ms > 0) {
      sum += ms;
      count++;
    }
  }
  return count ? sum / count : 0.0f;
}

void RendererTab(const FrameInfo& frame) {
  float average = AverageFrameMs();
  ImGui::Text("%.0f frames/s (%.2f ms average over the last %zu frames)",
              average > 0 ? 1000.0f / average : 0.0f, average, kHistory);
  float most = *std::max_element(g_frame_ms.begin(), g_frame_ms.end());
  ImGui::PlotLines("##frame_ms", g_frame_ms.data(), int(kHistory), int(g_frame_ms_next),
                   "frame time (ms)", 0.0f, std::max(most, 1.0f) * 1.1f,
                   ImVec2(-1.0f, ImGui::GetFontSize() * 6));
  ImGui::Text("This frame: %u commands, %u draws", frame.commands, frame.draws);
  ImGui::Text("Drawn at %ux%u, shown in a %ux%u window", frame.source.width, frame.source.height,
              frame.window.width, frame.window.height);
  ImGui::SeparatorText("GPU memory");
  ImGui::TextWrapped("%s", gpu_memory::Describe().c_str());
}

void Hud() {
  float average = AverageFrameMs();
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  float pad = ImGui::GetFontSize() * 0.6f;
  ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - pad,
                                 viewport->WorkPos.y + pad),
                          ImGuiCond_Always, ImVec2(1.0f, 0.0f));
  ImGui::SetNextWindowBgAlpha(0.6f);
  if (ImGui::Begin("##hud", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                       ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs)) {
    ImGui::Text("%.0f fps  %.2f ms", average > 0 ? 1000.0f / average : 0.0f, average);
  }
  ImGui::End();
}

void Notice() {
  std::string text;
  {
    std::lock_guard lock(g_notice_mutex);
    if (g_notice.empty() || std::chrono::steady_clock::now() >= g_notice_until) {
      return;
    }
    text = g_notice;
  }
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  float pad = ImGui::GetFontSize() * 0.6f;
  ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
                                 viewport->WorkPos.y + pad),
                          ImGuiCond_Always, ImVec2(0.5f, 0.0f));
  ImGui::SetNextWindowBgAlpha(0.7f);
  if (ImGui::Begin("##notice", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                       ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs)) {
    ImGui::TextUnformatted(text.c_str());
  }
  ImGui::End();
}

void Panels(const FrameInfo& frame) {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.05f,
                                 viewport->WorkPos.y + viewport->WorkSize.y * 0.08f),
                          ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x * 0.45f, viewport->WorkSize.y * 0.7f),
                           ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowBgAlpha(0.9f);
  if (ImGui::Begin("SBX (F4 to close)")) {
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##search", "search every setting", g_search, sizeof(g_search));
    ImGui::TextDisabled("Changes apply at once, except those marked; Enter applies a text field.");
    SaveBar();
    std::string search = Lower(g_search);
    if (!search.empty()) {
      SearchResults(search);
    } else if (ImGui::BeginTabBar("tabs")) {
      std::map<std::string, Groups> layout = Layout();
      for (const char* tab : kTabs) {
        if (ImGui::BeginTabItem(tab)) {
          GroupedFlags(tab, layout[tab]);
          ImGui::EndTabItem();
        }
      }
      if (ImGui::BeginTabItem("Renderer")) {
        RendererTab(frame);
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
  }
  ImGui::End();
}

}  // namespace

bool Init(const InitInfo& info) {
  g_instance = info.instance;
  g_format = info.format;
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;  // no imgui.ini next to the game
  // The cursor is the SDK's and the game's to show or hide; SDL's cursor
  // calls belong on the main thread.
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NoMouseCursorChange;
  ImGui::StyleColorsDark();
  ApplyAccent(ImGui::GetStyle());
  if (LoadFont(io, kFontSize)) {
    ImGui::GetStyle().FontSizeBase = kFontSize;
  }
  // ImGui works in the window's units (SDL's, points on a scaled desktop),
  // drawn at its pixel density. Sized for the desktop's scaling, and at
  // least as large as a 1080-line window's would be (a 4K screen at 100%).
  float density = SDL_GetWindowPixelDensity(info.window);
  float scale = SDL_GetWindowDisplayScale(info.window);
  scale = density > 0.0f && scale > 0.0f ? scale / density : 1.0f;
  int window_width = 0, window_height = 0;
  SDL_GetWindowSize(info.window, &window_width, &window_height);
  scale = std::max(scale, float(window_height) / 1080.0f);
  scale = std::max(scale, 1.0f);
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(scale);
  style.FontScaleDpi = scale;

  if (!ImGui_ImplSDL3_InitForVulkan(info.window)) {
    ITC_LOG_ERROR("native renderer: overlay: the SDL back-end didn't start");
    ImGui::DestroyContext();
    return false;
  }
  // Text input and the gamepads are left alone here: SDL's text input is
  // turned on from the main thread while the panels are shown (OnEvent),
  // and the game's controllers aren't the overlay's.
  ImGui::GetPlatformIO().Platform_SetImeDataFn = nullptr;
  ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode_Manual, nullptr, 0);

  if (!ImGui_ImplVulkan_LoadFunctions(info.api_version, LoadVulkanFunction)) {
    ITC_LOG_ERROR("native renderer: overlay: Vulkan functions missing");
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    return false;
  }
  ImGui_ImplVulkan_InitInfo init{};
  init.ApiVersion = info.api_version;
  init.Instance = info.instance;
  init.PhysicalDevice = info.physical_device;
  init.Device = info.device;
  init.QueueFamily = info.queue_family;
  init.Queue = info.queue;
  init.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
  // Its vertex buffers rotate over this many frames: more than ours in
  // flight, whatever the swapchain holds.
  init.MinImageCount = 2;
  init.ImageCount = std::max(info.image_count, 4u);
  init.UseDynamicRendering = true;
  init.PipelineInfoMain = PipelineInfo();
  init.CheckVkResultFn = CheckResult;
  if (!ImGui_ImplVulkan_Init(&init)) {
    ITC_LOG_ERROR("native renderer: overlay: the Vulkan back-end didn't start");
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    return false;
  }
  g_ready = true;
  ITC_LOG_INFO("native renderer: overlay ready (ImGui {}): F4 settings and renderer, F3 frame "
               "rate",
               IMGUI_VERSION);
  return true;
}

void OnSwapchain(VkFormat format, uint32_t) {
  if (!g_ready || format == g_format) {
    return;
  }
  g_format = format;
  ImGui_ImplVulkan_PipelineInfo info = PipelineInfo();
  ImGui_ImplVulkan_CreateMainPipeline(&info);
}

void OnEvent(const SDL_Event& event, SDL_WindowID our_window) {
  if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
      event.key.windowID == our_window) {
    if (event.key.key == SDLK_F4) {
      bool shown = !g_panels.load();
      g_panels = shown;
      // This is the main thread: SDL's text input (for the panels' text
      // fields) and the cursor are switched here.
      if (SDL_Window* window = SDL_GetWindowFromID(our_window)) {
        if (shown) {
          SDL_StartTextInput(window);
        } else {
          SDL_StopTextInput(window);
        }
      }
      static bool cursor_was_visible = true;
      if (shown) {
        cursor_was_visible = SDL_CursorVisible();
        SDL_ShowCursor();
      } else if (!cursor_was_visible) {
        SDL_HideCursor();
      }
      return;
    }
    if (event.key.key == SDLK_F3) {
      g_hud = !g_hud.load();
      return;
    }
  }
  if (!g_panels || EventWindow(event) != our_window) {
    return;
  }
  std::lock_guard lock(g_queue_mutex);
  if (g_queue.size() >= kMaxQueued) {
    return;
  }
  QueuedEvent& queued = g_queue.emplace_back();
  queued.event = event;
  if (event.type == SDL_EVENT_TEXT_INPUT && event.text.text) {
    queued.text = event.text.text;
  } else if (event.type == SDL_EVENT_TEXT_EDITING && event.edit.text) {
    queued.text = event.edit.text;
  }
}

bool Visible() {
  return g_ready && (g_panels || g_hud || NoticeShown());
}

void OnFrame(const FrameInfo& frame) {
  g_frame_ms[g_frame_ms_next] = float(frame.frame_ms);
  g_frame_ms_next = (g_frame_ms_next + 1) % kHistory;
}

void Draw(VkCommandBuffer commands, VkImageView view, VkExtent2D extent, const FrameInfo& frame) {
  if (!g_ready) {
    return;
  }
  bool panels = g_panels;
  if (!panels && !g_hud && !NoticeShown()) {
    return;
  }
  ImGuiIO& io = ImGui::GetIO();
  if (panels != g_panels_were_shown) {
    // Keys and buttons held when the panels closed don't stay down.
    io.ClearInputKeys();
    io.ClearInputMouse();
    g_panels_were_shown = panels;
  }
  std::vector<QueuedEvent> events;
  {
    std::lock_guard lock(g_queue_mutex);
    events.swap(g_queue);
  }
  for (QueuedEvent& queued : events) {
    if (queued.event.type == SDL_EVENT_TEXT_INPUT) {
      queued.event.text.text = queued.text.c_str();
    } else if (queued.event.type == SDL_EVENT_TEXT_EDITING) {
      queued.event.edit.text = queued.text.c_str();
    }
    if (panels) {
      ImGui_ImplSDL3_ProcessEvent(&queued.event);
    }
  }

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();  // the window's size and pixel density
  ImGui::NewFrame();
  if (g_hud) {
    Hud();
  }
  Notice();
  if (panels) {
    Panels(frame);
  }
  ImGui::Render();
  ImDrawData* data = ImGui::GetDrawData();
  if (!data || data->CmdListsCount == 0) {
    return;
  }
  VkRenderingAttachmentInfo attachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
  attachment.imageView = view;
  attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  VkRenderingInfo rendering{VK_STRUCTURE_TYPE_RENDERING_INFO};
  rendering.renderArea = {{0, 0}, extent};
  rendering.layerCount = 1;
  rendering.colorAttachmentCount = 1;
  rendering.pColorAttachments = &attachment;
  vk::vkCmdBeginRendering(commands, &rendering);
  ImGui_ImplVulkan_RenderDrawData(data, commands);
  vk::vkCmdEndRendering(commands);
}

void Notify(const std::string& text) {
  std::lock_guard lock(g_notice_mutex);
  g_notice = text;
  g_notice_until = std::chrono::steady_clock::now() + kNoticeTime;
}

}  // namespace native::overlay
