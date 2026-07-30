@echo off
REM Buduje SoundTouch + Bungee (x64, /MT). Odpalac po nalozeniu patcha na bungee.
setlocal
cd /d D:\projekty\bookbar\third_party
echo === SOUNDTOUCH ===
call _build_soundtouch.bat
if errorlevel 1 (echo ST_FAILED & exit /b 1)
echo === BUNGEE ===
call _build_bungee.bat
if errorlevel 1 (echo BG_FAILED & exit /b 1)
echo ALL_LIBS_OK
endlocal
