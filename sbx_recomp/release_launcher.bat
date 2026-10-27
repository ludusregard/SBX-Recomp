@echo off
rem Starts Shutoko Battle X with the Vulkan renderer. The game files go in the
rem assets folder next to this file (your own copy of the game).
setlocal
set "REX_GAME_DATA_ROOT=%~dp0assets"
"%~dp0sbx.exe" %*
