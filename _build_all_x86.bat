@echo off
REM =====================================================================
REM  Bookbar — pelny build 32-bit (x86 / Win32) dla foobar2000 v2 32-bit.
REM  Buduje: biblioteki SDK (Win32), SoundTouch (Win32), Bungee (Win32),
REM  a na koniec sama wtyczke foo_bookbar.dll (x86, /MD).
REM  Wynik: build_x86\foo_bookbar.dll
REM  Analogiczny do stosu x64, tylko Platform/-A = Win32 i vcvars x86.
REM =====================================================================
setlocal
set VS=C:\Program Files\Microsoft Visual Studio\2022\Community
set MSB="%VS%\MSBuild\Current\Bin\MSBuild.exe"
set CMAKE="%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set ROOT=D:\projekty\bookbar

REM ---------- 1. Biblioteki foobar2000 SDK (Win32, /MD, target 81) ----------
cd /d %ROOT%\sdk
echo === PFC (Win32) ===
%MSB% pfc\pfc.vcxproj /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0 /m /nologo /v:m
if errorlevel 1 (echo PFC_FAILED & exit /b 1)

echo === SHARED (Win32) ===
%MSB% foobar2000\shared\shared.vcxproj /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0 /m /nologo /v:m
if errorlevel 1 (echo SHARED_FAILED & exit /b 1)

echo === SDK (Win32) ===
%MSB% foobar2000\SDK\foobar2000_SDK.vcxproj /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0 /m /nologo /v:m
if errorlevel 1 (echo SDK_FAILED & exit /b 1)

REM ---------- 2. SoundTouch (Win32, static, /MD) ----------
echo === SOUNDTOUCH (Win32) ===
%CMAKE% -S %ROOT%\third_party\soundtouch -B %ROOT%\third_party\soundtouch\build_x86 -G "Visual Studio 17 2022" -A Win32 -T v143 ^
  -DBUILD_SHARED_LIBS=OFF -DINTEGER_SAMPLES=OFF -DSOUNDSTRETCH=OFF -DSOUNDTOUCH_DLL=OFF ^
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
if errorlevel 1 (echo ST_CONFIGURE_FAILED & exit /b 1)
%CMAKE% --build %ROOT%\third_party\soundtouch\build_x86 --config Release -j
if errorlevel 1 (echo ST_BUILD_FAILED & exit /b 1)

REM ---------- 3. Bungee (Win32, static, /MD) ----------
echo === BUNGEE (Win32) ===
%CMAKE% -S %ROOT%\third_party\bungee -B %ROOT%\third_party\bungee\build_x86 -G "Visual Studio 17 2022" -A Win32 -T v143 ^
  -DBUNGEE_BUILD_SHARED_LIBRARY=OFF ^
  -DCMAKE_C_FLAGS="/DWIN32 /D_WINDOWS /D_USE_MATH_DEFINES" ^
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
if errorlevel 1 (echo BG_CONFIGURE_FAILED & exit /b 1)
%CMAKE% --build %ROOT%\third_party\bungee\build_x86 --config Release --target bungee_library -j
if errorlevel 1 (echo BG_BUILD_FAILED & exit /b 1)

REM ---------- 4. Wtyczka foo_bookbar.dll (x86, /MD) ----------
echo === COMPONENT (x86) ===
call "%VS%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
if errorlevel 1 (echo VCVARS_FAILED & exit /b 1)
cd /d %ROOT%
if not exist build_x86 mkdir build_x86

set SDK=sdk
set ST=third_party\soundtouch
set BG=third_party\bungee
set STLIB=%ST%\build_x86\Release\SoundTouch.lib
set BGLIB=%BG%\build_x86\Release\bungee.lib %BG%\build_x86\Release\pffft.lib
REM UWAGA: dla Win32 msbuild wypisuje .lib bez podkatalogu platformy (samo Release\), inaczej niz x64.
set SDKLIB=%SDK%\foobar2000\SDK\Release\foobar2000_SDK.lib %SDK%\foobar2000\shared\Release\shared.lib %SDK%\pfc\Release\pfc.lib

rc /nologo /fo build_x86\bookbar.res src\bookbar.rc
if errorlevel 1 (echo RC_FAILED & exit /b 1)

cl /nologo /LD /O2 /MD /EHsc /std:c++20 /D_USE_MATH_DEFINES /DUNICODE /D_UNICODE ^
   /I%SDK% /I%SDK%\foobar2000 /Isrc /I%ST%\include /I%BG% /I%BG%\submodules\eigen ^
   src\main.cpp src\dsp_bookbar.cpp src\foobar_glue.cpp ^
   %SDK%\foobar2000\foobar2000_component_client\component_client.cpp ^
   src\processor.cpp src\gui.cpp src\shortcuts.cpp src\enhance.cpp ^
   src\engine_soundtouch.cpp src\engine_bungee.cpp src\engine_factory.cpp ^
   build_x86\bookbar.res ^
   /Fe:build_x86\foo_bookbar.dll /Fo:build_x86\ ^
   /link %STLIB% %BGLIB% %SDKLIB% ^
   user32.lib gdi32.lib comctl32.lib comdlg32.lib ole32.lib oleaut32.lib oleacc.lib uuid.lib shlwapi.lib shell32.lib advapi32.lib
if errorlevel 1 (echo COMPONENT_BUILD_FAILED & exit /b 1)

echo ALL_X86_OK
dir /b build_x86\foo_bookbar.dll
endlocal
