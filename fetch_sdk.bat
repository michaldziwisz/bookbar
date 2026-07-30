@echo off
REM fetch_sdk.bat — pobiera i rozpakowuje foobar2000 SDK do katalogu sdk\.
REM SDK nie jest commitowane (licencja + rozmiar). Wymaga 7-Zip w PATH albo
REM standardowej lokalizacji. Uruchom RAZ przed budowaniem.
setlocal
set SDKVER=SDK-2025-03-07
set URL=https://www.foobar2000.org/downloads/%SDKVER%.7z
cd /d D:\projekty\bookbar

if exist sdk\foobar2000\SDK\dsp.h (echo SDK juz obecny & exit /b 0)

echo Pobieram %URL% ...
powershell -Command "Invoke-WebRequest -Uri '%URL%' -OutFile '%SDKVER%.7z'"
if errorlevel 1 (echo DOWNLOAD_FAILED & exit /b 1)

set SZ=7z
where 7z >nul 2>&1 || set SZ="C:\Program Files\7-Zip\7z.exe"
%SZ% x -y -osdk %SDKVER%.7z
if errorlevel 1 (echo EXTRACT_FAILED & exit /b 1)

echo SDK_OK
endlocal
