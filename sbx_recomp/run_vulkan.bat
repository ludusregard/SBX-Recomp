@echo off
rem Launch the recompiled Shutoko Battle X built with the native Vulkan
rem renderer (build_vulkan.bat). Renders on the PC, not the emulated Xbox 360 GPU.
rem
rem   run_vulkan.bat [options]         e.g. run_vulkan.bat --log_level=debug
rem   set SBX_PRESET=win-amd64-debug & run_vulkan.bat     another build
rem
rem Settings live in sbx.toml next to sbx.exe (the F4 menu's Save button
rem writes it). The first run makes one with this launcher's old defaults.
rem Options given here win over the file for that run only.
setlocal
set "here=%~dp0"
set "here=%here:~0,-1%"
if "%SBX_PRESET%"=="" set "SBX_PRESET=win-amd64-relwithdebinfo"
set "exe=%here%\out\build\%SBX_PRESET%-vulkan\sbx.exe"
if not exist "%exe%" (echo %exe% not found: run build_vulkan.bat first & exit /b 1)
for %%I in ("%here%\..\assets") do set "assets=%%~fI"

set "REX_GAME_DATA_ROOT=%assets%"

for %%I in ("%exe%") do set "settings=%%~dpIsbx.toml"
if not exist "%settings%" (
  echo # Shutoko Battle X settings, loaded at every start.
  echo # Written by the Save button in the F4 menu; safe to edit by hand while the
  echo # game is closed. One setting per line: name = value ^(strings in quotes^).
  echo # Launch options ^(--name=value^) and REX_^<NAME^> variables win over this file.
  echo.
  echo # Made by run_vulkan.bat from its old defaults
  echo fullscreen = true
  echo mnk_mode = true
  echo skip_cutscenes = true
  echo new_music_roll = 1
) > "%settings%"

"%exe%" %*
