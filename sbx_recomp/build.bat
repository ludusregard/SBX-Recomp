@echo off
rem Build sbx.exe:  build.bat [preset]   (default win-amd64-relwithdebinfo)
rem
rem Logged to out\build.log (overwritten each run). On a failure the last
rem lines and every "error" line are shown and the window waits for a key
rem (set SBX_NOPAUSE=1 to skip that).
setlocal
cd /d "%~dp0"
set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=win-amd64-relwithdebinfo"
set "B=out\build\%PRESET%"
if not exist out mkdir out
set "LOG=%CD%\out\build.log"
echo build.bat %PRESET% - %DATE% %TIME% > "%LOG%"

if not exist "generated\default\sources.cmake" (
  echo No generated code yet: run codegen.bat first.
  echo No generated code yet: run codegen.bat first. >> "%LOG%"
  goto :failed
)
if not exist "%B%\build.ninja" (
  echo === Configuring ^(%PRESET%^)
  cmake --preset %PRESET% >> "%LOG%" 2>&1 || goto :failed
)

echo === Building (the first build compiles ~190 generated files: slow)
cmake --build "%B%" >> "%LOG%" 2>&1 || goto :failed
if not exist "%B%\sbx.exe" (
  echo The build reported success but %B%\sbx.exe is missing. >> "%LOG%"
  goto :failed
)
echo.
echo Built %B%\sbx.exe  -  run it with run.bat
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
