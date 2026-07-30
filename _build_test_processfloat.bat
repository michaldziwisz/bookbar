@echo off
REM Test sciezki foobar (Processor::processFloat). x64 /MD (jak wtyczka).
setlocal
set VS=C:\Program Files\Microsoft Visual Studio\2022\Community
call "%VS%\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul
if errorlevel 1 (echo VCVARS_FAILED & exit /b 1)
cd /d D:\projekty\bookbar
if not exist build mkdir build
if exist build\test_processfloat.exe del /q build\test_processfloat.exe

set ST=third_party\soundtouch
set BG=third_party\bungee
set LIBS=%ST%\build_x64\Release\SoundTouch.lib %BG%\build_x64\Release\bungee.lib %BG%\build_x64\Release\pffft.lib

cl /nologo /O2 /MD /EHsc /std:c++20 /W3 /D_USE_MATH_DEFINES ^
   /Isrc /I%ST%\include /I%BG% /I%BG%\submodules\eigen ^
   tests\test_processfloat.cpp src\processor.cpp src\enhance.cpp ^
   src\engine_soundtouch.cpp src\engine_bungee.cpp src\engine_factory.cpp ^
   /Fe:build\test_processfloat.exe /Fo:build\ ^
   /link %LIBS%
if errorlevel 1 (echo BUILD_FAILED & exit /b 1)
echo BUILD_OK
build\test_processfloat.exe
endlocal
