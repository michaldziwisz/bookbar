Bookbar — wykorzystane oprogramowanie open source / third-party notices
=======================================================================

Bookbar (c) 2026 Michał Dziwisz. Licencja: GNU GPL v3 (patrz plik LICENSE).
Konsultacja merytoryczna: Patryk Faliszewski.

Bookbar jest portem wtyczki Winamp „Bookamp" na platformę foobar2000.
Wtyczka korzysta z następujących bibliotek. Kod bibliotek time-stretch NIE
jest zawarty w tym repozytorium — dołączony jest jako submoduły git (katalog
third_party/); foobar2000 SDK pobiera się skryptem fetch_sdk.bat. Binarną
wersję buduje się ze źródeł (lokalnie lub w CI).

------------------------------------------------------------------------
1. foobar2000 SDK
   Autor: Peter Pawlowski.
   Źródło: https://www.foobar2000.org/SDK
   Licencja: BSD-style (patrz sdk/sdk-license.txt po pobraniu).
   Biblioteki pfc i libPPUI mają osobne, mniej restrykcyjne licencje.
   SDK NIE jest redystrybuowane w tym repozytorium — pobierz je skryptem
   fetch_sdk.bat ze strony producenta.

------------------------------------------------------------------------
2. SoundTouch
   Silnik time-stretch/pitch (WSOLA).
   Autor: Olli Parviainen.
   Źródło: https://codeberg.org/soundtouch/soundtouch
   Licencja: GNU LGPL v2.1 (patrz third_party/soundtouch/COPYING.TXT).

------------------------------------------------------------------------
3. Bungee
   Silnik time-stretch/pitch (phase-vocoder).
   Źródło: https://github.com/bungee-audio-stretch/bungee  (tag v2.4.24)
   Licencja: Mozilla Public License 2.0 (patrz third_party/bungee/LICENSE).

   Bungee używa dalszych bibliotek (jako własne submoduły):
   - Eigen  — MPL 2.0 — https://gitlab.com/libeigen/eigen
   - PFFFT  — licencja typu BSD/FFTPACK — https://bitbucket.org/jpommier/pffft
   - cxxopts — MIT — https://github.com/jarro2783/cxxopts
     (używane tylko przez narzędzie CLI Bungee, nie przez wtyczkę).

------------------------------------------------------------------------
Moduł „wzbogacania dźwięku" (wyrównywanie głośności / czytelność) w pliku
src/enhance.cpp jest własną implementacją algorytmów DSP z wiedzy publicznej
(limiter true-peak z ochroną ISP, phase rotator z filtrów allpass). Nie zawiera
kodu ani binariów osób trzecich.
