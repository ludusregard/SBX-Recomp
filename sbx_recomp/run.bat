@echo off
rem Launch the recompiled Shutoko Battle X from the build tree (development).
rem
rem   run.bat [options]         e.g. run.bat --log_level=debug
rem   set SBX_PRESET=win-amd64-debug & run.bat     another build
rem
rem Uses the emulated Xbox 360 GPU (--gpu_plugin=xenos).
rem Defaults are environment variables (REX_<OPTION>), which the game reads
rem after its command line, so an option given here overrides its default
rem (the game rejects an option given twice on the command line).
setlocal
set "here=%~dp0"
set "here=%here:~0,-1%"
if "%SBX_PRESET%"=="" set "SBX_PRESET=win-amd64-relwithdebinfo"
set "exe=%here%\out\build\%SBX_PRESET%\sbx.exe"
if not exist "%exe%" (echo %exe% not found: run build.bat first & exit /b 1)
for %%I in ("%here%\..\assets") do set "assets=%%~fI"

set "REX_GAME_DATA_ROOT=%assets%"
set "REX_GPU_PLUGIN=xenos"
set "REX_FULLSCREEN=false"
set "REX_MNK_MODE=true"
if "%REX_LOG_LEVEL%"=="" set "REX_LOG_LEVEL=info"

"%exe%" %*  --subtick=true