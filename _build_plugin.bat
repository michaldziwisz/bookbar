@echo off
REM Build foo_bookbar.dll — komponent foobar2000, x64, /MT (statyczny CRT).
REM Wymaga wczesniej: _build_sdk.bat (pfc/shared/SDK) + third_party libs x64.
setlocal
set VS=C:\Program Files\Microsoft Visual Studio\2022\Community
call "%VS%\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul
if errorlevel 1 (echo VCVARS_FAILED & exit /b 1)

cd /d D:\projekty\bookbar
if not exist build mkdir build

set SDK=sdk
set ST=third_party\soundtouch
set BG=third_party\bungee
set STLIB=%ST%\build_x64\Release\SoundTouch.lib
set BGLIB=%BG%\build_x64\Release\bungee.lib %BG%\build_x64\Release\pffft.lib
set SDKLIB=%SDK%\foobar2000\SDK\x64\Release\foobar2000_SDK.lib %SDK%\foobar2000\shared\x64\Release\shared.lib %SDK%\pfc\x64\Release\pfc.lib

REM zasoby GUI
rc /nologo /fo build\bookbar.res src\bookbar.rc
if errorlevel 1 (echo RC_FAILED & exit /b 1)

cl /nologo /LD /O2 /MD /EHsc /std:c++20 /D_USE_MATH_DEFINES ^
   /DUNICODE /D_UNICODE ^
   /I%SDK% /I%SDK%\foobar2000 /Isrc /I%ST%\include /I%BG% /I%BG%\submodules\eigen ^
   src\main.cpp src\dsp_bookbar.cpp src\foobar_glue.cpp ^
   %SDK%\foobar2000\foobar2000_component_client\component_client.cpp ^
   src\processor.cpp src\gui.cpp src\shortcuts.cpp src\enhance.cpp ^
   src\engine_soundtouch.cpp src\engine_bungee.cpp src\engine_factory.cpp ^
   build\bookbar.res ^
   /Fe:build\foo_bookbar.dll /Fo:build\ ^
   /link %STLIB% %BGLIB% %SDKLIB% ^
   user32.lib gdi32.lib comctl32.lib comdlg32.lib ole32.lib oleaut32.lib oleacc.lib uuid.lib shlwapi.lib shell32.lib advapi32.lib
if errorlevel 1 (echo BUILD_FAILED & exit /b 1)

echo BUILD_OK
dir /b build\foo_bookbar.dll
endlocal
