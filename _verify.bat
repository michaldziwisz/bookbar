@echo off
setlocal
set VS=C:\Program Files\Microsoft Visual Studio\2022\Community
call "%VS%\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul
cd /d D:\projekty\bookbar
echo === MACHINE ===
dumpbin /HEADERS build\foo_bookbar.dll | findstr /I "machine"
echo === EXPORTS ===
dumpbin /EXPORTS build\foo_bookbar.dll | findstr /I "foobar2000_get_interface"
echo === DEPENDENTS ===
dumpbin /DEPENDENTS build\foo_bookbar.dll | findstr /I ".dll"
endlocal
