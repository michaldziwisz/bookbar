@echo off
REM Buduje biblioteki foobar2000 SDK: pfc, foobar2000_SDK, shared (x64 Release, /MT).
setlocal
set VS=C:\Program Files\Microsoft Visual Studio\2022\Community
set MSB="%VS%\MSBuild\Current\Bin\MSBuild.exe"
cd /d D:\projekty\bookbar\sdk

echo === PFC ===
%MSB% pfc\pfc.vcxproj /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0 /m /nologo /v:m
if errorlevel 1 (echo PFC_FAILED & exit /b 1)

echo === SHARED ===
%MSB% foobar2000\shared\shared.vcxproj /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0 /m /nologo /v:m
if errorlevel 1 (echo SHARED_FAILED & exit /b 1)

echo === SDK ===
%MSB% foobar2000\SDK\foobar2000_SDK.vcxproj /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0 /m /nologo /v:m
if errorlevel 1 (echo SDK_FAILED & exit /b 1)

echo === HELPERS ===
%MSB% foobar2000\helpers\foobar2000_sdk_helpers.vcxproj /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0 /m /nologo /v:m
if errorlevel 1 (echo HELPERS_FAILED & exit /b 1)

echo === libPPUI ===
%MSB% libPPUI\libPPUI.vcxproj /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0 /m /nologo /v:m
if errorlevel 1 (echo PPUI_FAILED & exit /b 1)

echo SDK_BUILD_OK
endlocal
