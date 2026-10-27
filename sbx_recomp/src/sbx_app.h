// sbx - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/rex_app.h>

#include "audio/native_audio.h"
#include "hooks/custom_music.h"
#include "hooks/patch_loader.h"
#ifdef SBX_VULKAN_RENDERER
#include "base/log_sdk.h"
#include "movies/movie.h"
#include "native/app_window.h"
#endif

class SbxApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<SbxApp>(new SbxApp(ctx, "Shutokou Battle X",
        PPCImageConfig));
  }

  // Our own audio output instead of the SDK's render driver and Audio Worker
  // (src/audio/native_audio.cpp, from ITC-Recomp; --native_audio=false for
  // the SDK's).
  void OnPreSetup(rex::RuntimeConfig& config) override { native_audio::Configure(config); }

  // PATCH_DATA is looked for in the game data root (src/hooks/patch_loader.cpp),
  // and ADDON_DATA/CUSTOM_MUSIC (src/hooks/custom_music.cpp). With the native
  // renderer, the movies are read from there too (src/movies/movie.cpp).
  void OnPostSetup() override {
    patch_loader::SetGameDataRoot(game_data_root());
    custom_music::SetGameDataRoot(game_data_root());
#ifdef SBX_VULKAN_RENDERER
    movies::movie::SetDataRoot(game_data_root());
#endif
  }

#ifdef SBX_VULKAN_RENDERER
  // The native renderer (src/native/) draws in its own window: the SDK's is
  // hidden and gets its events (src/native/app_window.cpp).
  bool SetupPresentation() override {
    if (!rex::ReXApp::SetupPresentation()) {
      return false;
    }
    native::AppWindow_SetSdkWindow(window());
    return true;
  }

  // The native renderer's log (src/base/log.cpp) takes over the file the
  // SDK just opened, as in ITC-Recomp.
  void OnPostInitLogging() override { base::TakeOverSdkLogFile(); }
#endif

  // Override virtual hooks for customization:
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override;
  // std::unique_ptr<rex::ui::AchievementNotificationDialog>
  // CreateAchievementNotificationDialog() override;
  // void OnShutdown() override {}
  // void OnConfigurePaths(rex::PathConfig& paths) override {}
};
