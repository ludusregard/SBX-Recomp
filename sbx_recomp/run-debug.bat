@echo off
rem run.bat with verbose logging (logs\ next to sbx.exe), for chasing crashes.
"%~dp0run.bat" --log_level=debug %*
