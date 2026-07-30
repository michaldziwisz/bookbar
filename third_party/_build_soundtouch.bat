@echo off
REM Build SoundTouch jako static lib, x64, /MT (statyczny CRT), float samples.
setlocal
set CMAKE="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set SRC=D:\projekty\bookbar\third_party\soundtouch
set BUILD=D:\projekty\bookbar\third_party\soundtouch\build_x64

%CMAKE% -S "%SRC%" -B "%BUILD%" -G "Visual Studio 17 2022" -A x64 -T v143 ^
  -DBUILD_SHARED_LIBS=OFF ^
  -DINTEGER_SAMPLES=OFF ^
  -DSOUNDSTRETCH=OFF ^
  -DSOUNDTOUCH_DLL=OFF ^
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL ^
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
if errorlevel 1 (echo CONFIGURE_FAILED & exit /b 1)

%CMAKE% --build "%BUILD%" --config Release -j
if errorlevel 1 (echo BUILD_FAILED & exit /b 1)

echo BUILD_OK
dir /s /b "%BUILD%\*.lib"
endlocal
