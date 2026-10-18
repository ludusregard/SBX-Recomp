@echo off
rem Configure, build the rexglue tool, run codegen on sbx_manifest.toml, then
rem configure again so CMake picks up generated\default\sources.cmake.
rem
rem   codegen.bat [preset]        default preset: win-amd64-relwithdebinfo
rem
rem Everything is logged to out\codegen.log (overwritten each run). On a
rem failure the last lines and every "error" line are shown, and the window
rem waits for a key (set SBX_NOPAUSE=1 to skip that when running from a
rem script).
rem
rem Codegen also runs as part of build.bat when its inputs change (stock
rem generated\rexglue.cmake). It OVERWRITES generated\default and rewrites
rem generated\rexglue.cmake: once generated code is hand-edited, commit
rem first (docs\codegen.md).
setlocal
cd /d "%~dp0"
set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=win-amd64-relwithdebinfo"
set "B=out\build\%PRESET%"
if not exist out mkdir out
set "LOG=%CD%\out\codegen.log"
echo codegen.bat %PRESET% - %DATE% %TIME% > "%LOG%"

call :step "Configuring (%PRESET%)" cmake --preset %PRESET% || goto :failed
call :step "Building the rexglue tool and the SDK it needs (slow the first time)" cmake --build "%B%" --target rexglue || goto :failed
call :step "Running codegen" cmake --build "%B%" --target sbx_codegen || goto :failed
call :step "Configuring again (picks up the generated sources)" cmake --preset %PRESET% || goto :failed

echo.
findstr /i /c:"unresolved" /c:"Codegen summary" "%LOG%"
echo.
echo Codegen done. Next: build.bat %PRESET%
echo Log: %LOG%
if not defined SBX_NOPAUSE pause
exit /b 0

:step
echo.
echo === %~1
echo. >> "%LOG%"
echo === %~1 >> "%LOG%"
shift
set "CMD="
:collect
if "%~1"=="" goto :run
set "CMD=%CMD% %1"
shift
goto :collect
:run
echo %CMD% >> "%LOG%"
%CMD% >> "%LOG%" 2>&1
exit /b %ERRORLEVEL%

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
