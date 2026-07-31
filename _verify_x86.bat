@echo off
setlocal
set VS=C:\Program Files\Microsoft Visual Studio\2022\Community
call "%VS%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
cd /d D:\projekty\bookbar
echo === HEADERS (machine) ===
dumpbin /headers build_x86\foo_bookbar.dll | findstr /i "machine"
echo === EXPORTS ===
dumpbin /exports build_x86\foo_bookbar.dll | findstr /i "foobar2000_get_interface"
echo === DEPENDENTS ===
dumpbin /dependents build_x86\foo_bookbar.dll | findstr /i ".dll"
endlocal
