// sbx - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/rex_app.h>

#include "hooks/patch_loader.h"
#include "tick_rate/tick_rate.h"

class SbxApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<SbxApp>(new SbxApp(ctx, "Shutokou Battle X",
        PPCImageConfig));
  }

  // PATCH_DATA is looked for in the game data root (src/hooks/patch_loader.cpp).
  void OnPostSetup() override { patch_loader::SetGameDataRoot(game_data_root()); }

  // --tick_rate_half_step (src/tick_rate/tick_rate.cpp).
  void OnPostLoadXexImage() override {
    tick_rate::PatchStepConstants(runtime()->memory());
  }

  // Override virtual hooks for customization:
  // void OnPostInitLogging() override {}
  // void OnPreSetup(rex::RuntimeConfig& config) override {}
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override;
  // std::unique_ptr<rex::ui::AchievementNotificationDialog>
  // CreateAchievementNotificationDialog() override;
  // void OnShutdown() override {}
  // void OnConfigurePaths(rex::PathConfig& paths) override {}
};
