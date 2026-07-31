@echo off
setlocal
set VS=C:\Program Files\Microsoft Visual Studio\2022\Community
call "%VS%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
cd /d D:\projekty\bookbar
cl /nologo /EHsc /std:c++20 _miniloader_x86.cpp /Fe:build_x86\miniloader.exe /Fo:build_x86\ /link user32.lib
if errorlevel 1 (echo MINILOADER_BUILD_FAILED & exit /b 1)
build_x86\miniloader.exe
endlocal
