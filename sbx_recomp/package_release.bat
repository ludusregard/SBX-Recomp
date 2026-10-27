@echo off
rem Package the Vulkan build (build_vulkan.bat) as a release:
rem   package_release.bat [preset]   (default win-amd64-relwithdebinfo)
rem
rem Output: release\sbx-vulkan\ (sbx.exe, its DLLs, run_sbx.bat and an empty
rem assets folder) and release\sbx-vulkan.zip. Game files are not packaged:
rem each player puts their own copy in assets\. Debug symbols (.pdb) are left
rem out. The Vulkan driver (vulkan-1.dll) comes from the GPU driver.
rem Run build_vulkan.bat first. Set SBX_NOPAUSE=1 to skip the final wait.
setlocal
cd /d "%~dp0"
set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=win-amd64-relwithdebinfo"
set "B=out\build\%PRESET%-vulkan"
set "R=release\sbx-vulkan"

if not exist "%B%\sbx.exe" (
  echo %B%\sbx.exe not found: run build_vulkan.bat first.
  goto :failed
)

echo === Packaging %PRESET% into %R%
if not exist "%R%\assets" mkdir "%R%\assets"
copy /Y "%B%\sbx.exe" "%R%\" >nul || goto :failed
copy /Y "%B%\*.dll" "%R%\" >nul || goto :failed
copy /Y "release_launcher.bat" "%R%\run_sbx.bat" >nul || goto :failed

echo === Zipping to release\sbx-vulkan.zip
powershell -NoProfile -Command "Compress-Archive -Path '%R%\*' -DestinationPath 'release\sbx-vulkan.zip' -Force" || goto :failed

echo.
echo Packaged %R%\ and release\sbx-vulkan.zip
if not defined SBX_NOPAUSE pause
exit /b 0

:failed
echo.
echo ===== FAILED: the release was not packaged.
if not defined SBX_NOPAUSE pause
exit /b 1
