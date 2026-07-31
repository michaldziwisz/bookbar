@echo off
REM Pakuje dual-arch .fb2k-component: foo_bookbar.dll (x86) w korzeniu ZIP-a
REM + x64\foo_bookbar.dll (x64) w podkatalogu. foobar v2 32-bit laduje korzen,
REM foobar v2 64-bit laduje podkatalog x64\.
setlocal
set VS=C:\Program Files\Microsoft Visual Studio\2022\Community
set SZ="C:\Program Files\7-Zip\7z.exe"
cd /d D:\projekty\bookbar

REM Zbuduj czysta strukture w dist\
if exist dist rmdir /s /q dist
mkdir dist
mkdir dist\x64
copy /y build_x86\foo_bookbar.dll dist\foo_bookbar.dll >nul
copy /y build\foo_bookbar.dll dist\x64\foo_bookbar.dll >nul

del /q dist\foo_bookbar.fb2k-component 2>nul
cd dist
%SZ% a -tzip foo_bookbar.fb2k-component foo_bookbar.dll x64
if errorlevel 1 (echo ZIP_FAILED & exit /b 1)
cd ..

echo === ZAWARTOSC PAKIETU ===
%SZ% l dist\foo_bookbar.fb2k-component
echo === MACHINE x86 (korzen) ===
call "%VS%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
dumpbin /headers dist\foo_bookbar.dll | findstr /i "machine"
echo === MACHINE x64 (podkatalog) ===
dumpbin /headers dist\x64\foo_bookbar.dll | findstr /i "machine"
endlocal
