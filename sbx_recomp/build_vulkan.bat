@echo off
rem Build sbx.exe with the native Vulkan renderer (SBX_NATIVE_RENDERER=ON):
rem   build_vulkan.bat [preset]   (default win-amd64-relwithdebinfo)
rem
rem Output: out\build\<preset>-vulkan\sbx.exe, separate from build.bat's
rem out\build\<preset> (the emulated GPU build). Configure runs every time.
rem Logged to out\build_vulkan.log (overwritten each run). On a failure the
rem last lines and every "error" line are shown and the window waits for a
rem key (set SBX_NOPAUSE=1 to skip that).
setlocal
cd /d "%~dp0"
set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=win-amd64-relwithdebinfo"
set "B=out\build\%PRESET%-vulkan"
if not exist out mkdir out
set "LOG=%CD%\out\build_vulkan.log"
echo build_vulkan.bat %PRESET% - %DATE% %TIME% > "%LOG%"

if not exist "generated\default\sources.cmake" (
  echo No generated code yet: run codegen.bat first.
  echo No generated code yet: run codegen.bat first. >> "%LOG%"
  goto :failed
)

echo === Configuring ^(%PRESET%, SBX_NATIVE_RENDERER=ON, %B%^)
cmake --preset %PRESET% -B "%B%" -DSBX_NATIVE_RENDERER=ON >> "%LOG%" 2>&1 || goto :failed

echo === Building ^(the first build compiles the Vulkan SDK and generated code: slow^)
cmake --build "%B%" >> "%LOG%" 2>&1 || goto :failed
if not exist "%B%\sbx.exe" (
  echo The build reported success but %B%\sbx.exe is missing. >> "%LOG%"
  goto :failed
)
echo.
echo Built %B%\sbx.exe  -  run it with run_vulkan.bat
if not defined SBX_NOPAUSE pause
exit /b 0

:failed
echo.
echo ===== FAILED. Last 40 lines of %LOG%:
powershell -NoProfile -Command "Get-Content -LiteralPath '%LOG%' -Tail 40"
echo.
echo ===== Lines mentioning errors:
findstr /i /n /c:"error" /c:"FAILED:" "%LOG%"
echo.
echo Full log: %LOG%
if not defined SBX_NOPAUSE pause
exit /b 1
