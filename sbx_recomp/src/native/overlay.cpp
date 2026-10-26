// The native renderer's ImGui overlay (overlay.h).
//
// ImGui itself lives in the SDK's runtime DLL (built into rexruntime, every
// symbol exported); the SDL3 and Vulkan back-ends are built into the game
// (CMakeLists.txt). In the native renderer's (detached) mode the SDK makes no
// ImGui drawer of its own, so this context is the only one in use.

#include "overlay.h"

#include "base/flags.h"
#include "base/log.h"
#include "generated_overlay_font.h"
#include "gpu_memory.h"

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
#include <cstring>
#include <filesystem>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

ITC_FLAG_STRING(overlay_font, "", "SBX",
                "The overlay's font: empty for the built-in Intel One Mono, or a TrueType file "
                "(.ttf) by path or by name in the Windows fonts folders (the system's and your "
                "own). ImGui's built-in font if a file doesn't load. Read at startup");

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

// The settings' tabs, in order (the renderer's own tab comes after them).
constexpr std::array<const char*, 6> kTabs = {"Graphics", "Fixes",   "Hooks",
                                              "Logging",  "ReXGlue", "Vulkan"};

// Our flags that are read once (at startup, or when first used): changing
// them while the game runs does nothing until a restart. The SDK's flags
// carry their own lifecycle.
const std::set<std::string> kStartOnly = {
    "width",           "height",          "fullscreen",       "msaa",
    "native_audio",    "native_xma",      "gpu_plugin",       "cmaa2_threshold",
    "native_renderer_validation",         "native_renderer_dump",
    "play_movies_with_ffmpeg", "overlay_font",
};

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
  // The accent, at twice the strengths of the first version (brighter over the
  // dark).
  auto tint = [&style](ImGuiCol which, float alpha) {
    style.Colors[which] = ImVec4(kAccent.x, kAccent.y, kAccent.z, std::min(alpha * 2.0f, 1.0f));
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

// The overlay's font: Intel One Mono, built in (generated_overlay_font.h), or
// overlay_font's file, as a path or as a name in the Windows fonts folders
// (the system's, then the user's). Only TrueType loads: this ImGui has no
// FreeType, so an OTF file fails and ImGui's built-in font stays.
bool LoadFont(ImGuiIO& io, float size) {
  const std::filesystem::path name = ITC_FLAG(overlay_font);
  if (name.empty()) {
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;  // the built-in data isn't ImGui's to free
    if (ImFont* font = io.Fonts->AddFontFromMemoryTTF(const_cast<uint8_t*>(k_overlay_font),
                                                      int(k_overlay_font_size), size, &config)) {
      io.FontDefault = font;
      return true;
    }
    ITC_LOG_WARN("native renderer: overlay: the built-in font didn't load; ImGui's built-in font");
    return false;
  }
  std::vector<std::filesystem::path> candidates = {name};
  if (name.is_relative()) {
    if (const char* windows = std::getenv("WINDIR")) {
      candidates.push_back(std::filesystem::path(windows) / "Fonts" / name);
    }
    if (const char* local = std::getenv("LOCALAPPDATA")) {
      candidates.push_back(std::filesystem::path(local) / "Microsoft" / "Windows" / "Fonts" /
                           name);
    }
  }
  for (const std::filesystem::path& path : candidates) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error)) {
      continue;
    }
    if (ImFont* font = io.Fonts->AddFontFromFileTTF(path.string().c_str(), size)) {
      io.FontDefault = font;
      ITC_LOG_INFO("native renderer: overlay font: {}", path.string());
      return true;
    }
  }
  ITC_LOG_WARN("native renderer: overlay font {} not found or not TrueType; ImGui's built-in font",
               name.string());
  return false;
}

// One flag's row: its name (with a marker if it only applies on restart),
// the description as a tooltip, and an editor by its type.
void FlagRow(const rex::cvar::FlagEntry& entry) {
  bool restart = kStartOnly.count(entry.name) ||
                 entry.lifecycle == rex::cvar::Lifecycle::kRequiresRestart;
  bool read_only = entry.lifecycle == rex::cvar::Lifecycle::kInitOnly;
  ImGui::PushID(entry.name.c_str());
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  if (restart || read_only) {
    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "%s%s", entry.name.c_str(),
                       read_only ? " (startup only)" : " (restart)");
  } else {
    ImGui::TextUnformatted(entry.name.c_str());
  }
  if (ImGui::IsItemHovered() && !entry.description.empty()) {
    ImGui::SetTooltip("%s\nDefault: %s", entry.description.c_str(), entry.default_value.c_str());
  }
  ImGui::TableNextColumn();
  ImGui::SetNextItemWidth(-1.0f);
  if (read_only) {
    ImGui::BeginDisabled();
  }
  std::string value = entry.getter ? entry.getter() : std::string();
  using rex::cvar::FlagType;
  switch (entry.type) {
    case FlagType::Boolean: {
      bool on = value == "true" || value == "1";
      if (ImGui::Checkbox("##v", &on)) {
        rex::cvar::SetFlagByName(entry.name, on ? "true" : "false");
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
        rex::cvar::SetFlagByName(entry.name, std::to_string(number));
      }
      break;
    }
    case FlagType::Double: {
      double number = std::atof(value.c_str());
      if (ImGui::InputDouble("##v", &number, 0.0, 0.0, "%.4f",
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
        rex::cvar::SetFlagByName(entry.name, std::to_string(number));
      }
      break;
    }
    default: {
      if (!entry.constraints.allowed_values.empty()) {
        if (ImGui::BeginCombo("##v", value.c_str())) {
          for (const std::string& option : entry.constraints.allowed_values) {
            if (ImGui::Selectable(option.c_str(), option == value)) {
              rex::cvar::SetFlagByName(entry.name, option);
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
        rex::cvar::SetFlagByName(entry.name, buffer);
      }
      break;
    }
  }
  if (read_only) {
    ImGui::EndDisabled();
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
      {"overlay_font", {"Graphics", "Overlay"}},
      {"camera_fov", {"Graphics", "Camera"}},
      {"log_record_matrix", {"Logging", "Debugging"}},
      {"blur_frame_blend", {"Graphics", "Experimental"}},
      {"blur_frame_blend_center", {"Graphics", "Experimental"}},
      {"blur_frame_blend_edge", {"Graphics", "Experimental"}},
      {"blur_frame_blend_shutter", {"Graphics", "Experimental"}},
      {"unlock_cut_cars", {"Fixes", "Cars"}},
      {"cut_car_wheels", {"Fixes", "Cars"}},
      {"cut_car_steelies", {"Fixes", "Cars"}},
      {"per_tick_post_race_check", {"Fixes", "Race"}},
      {"post_race_check_cycles", {"Fixes", "Race"}},
      {"skip_cutscenes", {"Hooks", "Title"}},
      {"attract_after_s", {"Hooks", "Title"}},
      {"native_audio", {"Hooks", "Audio"}},
      {"native_xma", {"Hooks", "Audio"}},
      {"voice_thread", {"Hooks", "Audio"}},
      {"play_movies_with_ffmpeg", {"Hooks", "Movies"}},
      {"patch_loader", {"Hooks", "Patches"}},
      {"dump_shader_compiles", {"Hooks", "Shaders"}},
      {"log_boot_modes", {"Logging", "Game"}},
      {"log_file_opens", {"Logging", "Game"}},
      {"log_bgm", {"Logging", "Game"}},
      {"log_message_handlers", {"Logging", "Game"}},
      {"log_gpu_memory", {"Logging", "Native Renderer"}},
      {"find_per_frame_state", {"Logging", "Debugging"}},
      {"find_per_frame_state_interval_ms", {"Logging", "Debugging"}},
      {"watch_addresses", {"Logging", "Debugging"}},
      {"find_any_steps", {"Logging", "Debugging"}},
      {"find_countdowns", {"Logging", "Debugging"}},
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
  if (entry.category == "UI/Vulkan") {
    return {"Vulkan", "Presentation"};
  }
  if (entry.category == "GPU/Vulkan") {
    return {"Vulkan", "Renderer"};
  }
  if (entry.category == "UI/Presenter") {
    return {"ReXGlue", "Presenter"};
  }
  if (entry.category == "UI/Window" || entry.category == "Display") {
    return {"ReXGlue", "Window"};
  }
  if (entry.category == "Audio") {
    return {"ReXGlue", "Audio"};
  }
  if (entry.category == "GPU") {
    return {"ReXGlue", "GPU"};
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
    tabs[place.tab][place.group].push_back(&entry);
  }
  for (auto& [tab, groups] : tabs) {
    for (auto& [group, flags] : groups) {
      std::sort(flags.begin(), flags.end(),
                [](const auto* a, const auto* b) { return a->name < b->name; });
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
  return g_ready && (g_panels || g_hud);
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
  if (!panels && !g_hud) {
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

}  // namespace native::overlay
