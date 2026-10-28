@echo off
rem Launch the recompiled Shutoko Battle X built with the native Vulkan
rem renderer (build_vulkan.bat). Renders on the PC, not the emulated Xbox 360 GPU.
rem
rem   run_vulkan.bat [options]         e.g. run_vulkan.bat --log_level=debug
rem   set SBX_PRESET=win-amd64-debug & run_vulkan.bat     another build
rem
rem Defaults are environment variables (REX_<OPTION>), which the game reads
rem after its command line, so an option given here overrides its default
rem (the game rejects an option given twice on the command line).
setlocal
set "here=%~dp0"
set "here=%here:~0,-1%"
if "%SBX_PRESET%"=="" set "SBX_PRESET=win-amd64-relwithdebinfo"
set "exe=%here%\out\build\%SBX_PRESET%-vulkan\sbx.exe"
if not exist "%exe%" (echo %exe% not found: run build_vulkan.bat first & exit /b 1)
for %%I in ("%here%\..\assets") do set "assets=%%~fI"

set "REX_GAME_DATA_ROOT=%assets%"
set "REX_FULLSCREEN=false"
set "REX_MNK_MODE=true"
if "%REX_LOG_LEVEL%"=="" set "REX_LOG_LEVEL=info"

"%exe%" %* --height=2160