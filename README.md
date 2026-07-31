# Bookbar — audiobook DSP component for foobar2000

*Polski opis znajduje się [poniżej](#bookbar--komponent-dsp-foobar2000-do-audiobooków).*

Bookbar is a DSP component for **foobar2000** (32-bit and 64-bit), designed for listening to
audiobooks, podcasts and spoken-word recordings. It lets you smoothly change
playback tempo, pitch and "tape speed", and optionally enhances the sound
(loudness leveling and speech intelligibility). The configuration window is fully
accessible to screen readers (NVDA, JAWS).

Bookbar is a port of the Winamp plugin
**[Bookamp](https://github.com/michaldziwisz/bookamp)** to foobar2000.

Author: **Michał Dziwisz**. Subject-matter consultant: **Patryk Faliszewski**.

---

## Download

The ready-to-use component (`foo_bookbar.fb2k-component`) is published as a
**GitHub Release** — built and hosted by GitHub, downloadable without logging in.
The single package is **dual-architecture**: it contains both the 32-bit and the
64-bit build, so it installs on either flavour of foobar2000 v2:

**→ https://github.com/michaldziwisz/bookbar/releases/latest**

There are **no** binaries in the repository — to build it yourself, see the
"Building" section below.

---

## Bilingual interface (English / Polish)

Bookbar picks the window language based on the **Windows UI language**:

- Polish Windows → **Polish** interface.
- Any other Windows language → **English** interface.

Detection happens once, at startup (`GetUserDefaultUILanguage`).

---

## Features

### Tempo, pitch and "tape" control

Three independent sliders:

- **Tempo** — changes playback speed **without** changing pitch. Range 0.5x–8.0x,
  neutral 1.00x placed at 25% of the slider.
- **Pitch** — changes pitch **without** changing tempo. Range ±1200 cents
  (±1 octave) in 5-cent steps; 100 cents = 1 semitone. Neutral: 0 (slider center).
- **Rate (tape)** — changes tempo **and** pitch together (like a tape deck's speed),
  with an anti-aliasing filter. Range 0.5x–4.0x.

Values are announced by the screen reader as real numbers (e.g. "1.50x",
"+2 semitones (+200 cents)"), not as a slider-position percentage.

### Two time-stretch engines

- **Bungee** (phase-vocoder) — default, high quality.
- **SoundTouch** (WSOLA) — an alternative tuned for speech.

Each engine has its own advanced parameters (separate window).

### Sound enhancement (optional, off by default)

Two independent 5-level effects (Off / Subtle / Moderate / Medium / Strong / Maximum):

- **Loudness** — a true-peak brickwall limiter with inter-sample peak protection
  (ISP, 4x oversampling). Evens out quiet and loud passages.
- **Speech intelligibility** — a bank of allpass filters (phase rotator), modeled
  after broadcast processors (Orban/Omnia). Improves speech clarity on poor speakers.

When both levels are 0, the module is skipped entirely (zero overhead).

---

## Keyboard shortcuts

Bookbar registers Main Menu commands (**Playback** menu) that you can bind to any
keys in **Preferences → Keyboard Shortcuts**:

| Command                          | Suggested key       |
|----------------------------------|---------------------|
| Bookbar: Tempo down / up         | `-` / `=`           |
| Bookbar: Pitch down / up         | `Shift`+`-` / `=`   |
| Bookbar: Rate (tape) down / up   | `Ctrl`+`Shift`+ …   |
| Bookbar: Reset tempo/pitch/rate  | (your choice)       |

In addition, Bookbar installs its own **built-in shortcuts** that work out of the
box while foobar2000 is focused (and not while typing in a text field):

| Shortcut                  | Action                              |
|---------------------------|-------------------------------------|
| `-` / `=`                 | tempo down / up (step 0.05x)        |
| `Shift` + `-` / `=`       | pitch −1 / +1 semitone              |
| `Ctrl`+`Shift`+ `-` / `=` | rate (tape) down / up               |
| `Shift` + `Backspace`     | reset all sliders                   |

The built-in shortcuts can be turned off with a checkbox in the configuration
window (if you prefer to bind your own keys via Preferences → Keyboard Shortcuts).

---

## Installation

Use **File → Preferences → Components → Install…**, pick
`foo_bookbar.fb2k-component`, restart foobar2000, then add **Bookbar** in
**Preferences → Playback → DSP Manager**. foobar2000 unpacks the correct build
(32-bit or 64-bit) automatically from the single package.

Requires **foobar2000 v2.0 or newer** (32-bit or 64-bit).

---

## Building

Requirements: Windows, Visual Studio 2022 (toolset v143), 7-Zip.

The SoundTouch and Bungee sources are included as **git submodules**; the
foobar2000 SDK is fetched by a script (its license forbids redistribution):

```
git clone --recurse-submodules https://github.com/michaldziwisz/bookbar
cd bookbar
```

(If you cloned without `--recurse-submodules`:
`git submodule update --init --recursive`.)

Then, from a normal command prompt:

```
REM 0) fetch the foobar2000 SDK into sdk\
fetch_sdk.bat

REM 1) apply the MSVC compatibility patch to Bungee (upstream targets GCC/Clang)
cd third_party\bungee && git apply ..\..\patches\bungee-msvc.patch && cd ..\..

REM 2) build the foobar2000 SDK libraries (pfc, shared, SDK) — x64 /MD
_build_sdk.bat

REM 3) build the third-party libraries (SoundTouch, Bungee) — x64 /MD
third_party\_build_all_libs.bat

REM 4) build the component (x64)
_build_plugin.bat

REM 5) OPTIONAL — also build the 32-bit stack + component in one go
_build_all_x86.bat

REM 6) OPTIONAL — pack both DLLs into one dual-arch .fb2k-component
_package_dual.bat
```

Output: `build\foo_bookbar.dll` (x64), `build_x86\foo_bookbar.dll` (x86) and,
after step 6, the dual-arch `dist\foo_bookbar.fb2k-component`. The CI workflow
(`.github/workflows/build.yml`) builds both architectures and packages them the
same way.

foobar2000 components use the **dynamic CRT (/MD)** — the host ships the VC++
runtime. (This differs from the Winamp version, which is statically linked.)

Building also runs automatically in GitHub Actions — pushing a `vX.Y.Z` tag
produces a public Release with the ready component.

---

## License

Bookbar is released under the **GNU GPL v3** (see [LICENSE](LICENSE)).

It uses open-source libraries (foobar2000 SDK — BSD-style, SoundTouch — LGPL 2.1,
Bungee — MPL 2.0 and others). Full list and notices: [NOTICE.md](NOTICE.md).

<br>

═══════════════════════════════════════════════════════════════════════════

<br>

# Bookbar — komponent DSP foobar2000 do audiobooków

*The English description is [above](#bookbar--audiobook-dsp-component-for-foobar2000).*

Bookbar to komponent DSP dla **foobara2000** (32- i 64-bitowego), zaprojektowany z myślą
o słuchaniu audiobooków, podcastów i nagrań mowy. Pozwala płynnie zmieniać tempo,
wysokość dźwięku oraz „prędkość taśmy", a dodatkowo opcjonalnie wzbogaca dźwięk
(wyrównanie głośności i poprawa czytelności mowy). Okno konfiguracji jest w pełni
dostępne dla czytników ekranu (NVDA, JAWS).

Bookbar jest portem wtyczki Winamp
**[Bookamp](https://github.com/michaldziwisz/bookamp)** na foobar2000.

Autor: **Michał Dziwisz**. Konsultacja merytoryczna: **Patryk Faliszewski**.

---

## Pobieranie

Gotowy komponent (`foo_bookbar.fb2k-component`) jest publikowany jako
**GitHub Release** — budowany i hostowany przez GitHub, do pobrania bez logowania.
Jeden pakiet jest **dwuarchitekturowy**: zawiera zarówno wersję 32-, jak i
64-bitową, więc instaluje się w obu odmianach foobara2000 v2:

**→ https://github.com/michaldziwisz/bookbar/releases/latest**

W repozytorium **nie ma** binarek — jak zbudować samodzielnie, patrz sekcja
„Budowanie" na dole.

---

## Dwujęzyczny interfejs (polski / angielski)

Bookbar automatycznie dobiera język okna do **języka interfejsu systemu Windows**:

- Polski Windows → interfejs **polski**.
- Każdy inny język Windows → interfejs **angielski**.

Wykrycie następuje raz, przy starcie (funkcja `GetUserDefaultUILanguage`).

---

## Funkcje

### Zmiana tempa, wysokości i „taśmy"

Trzy niezależne suwaki:

- **Tempo** — zmienia szybkość odtwarzania **bez** zmiany wysokości głosu.
  Zakres 0,5x–8,0x, wartość neutralna 1,00x na 25% suwaka.
- **Wysokość** — zmienia wysokość głosu **bez** zmiany tempa. Zakres ±1200 centów
  (±1 oktawa), w krokach 5 centów; 100 centów = 1 półton. Neutralnie: 0 (środek).
- **Rate (kaseta)** — zmienia tempo **i** wysokość jednocześnie (jak zmiana
  prędkości magnetofonu), z filtrem antyaliasingowym. Zakres 0,5x–4,0x.

Wartości są odczytywane przez czytnik jako realne liczby (np. „1,50x",
„+2 półtony (+200 centów)"), a nie procent położenia suwaka.

### Dwa silniki time-stretch

- **Bungee** (phase-vocoder) — domyślny, wysoka jakość.
- **SoundTouch** (WSOLA) — alternatywa zoptymalizowana pod mowę.

Każdy silnik ma własne parametry zaawansowane (osobne okno).

### Wzbogacanie dźwięku (opcjonalne, domyślnie wyłączone)

Dwa niezależne 5-stopniowe efekty (Wyłączone / Delikatnie / Umiarkowanie /
Średnio / Mocno / Maksymalnie):

- **Wyrównywanie głośności** — brickwall limiter true-peak z ochroną przed
  przesterowaniem międzypróbkowym (ISP, 4-krotny oversampling).
- **Czytelność** — bateria filtrów allpass (phase rotator), na wzór procesorów
  nadawczych (Orban/Omnia). Poprawia zrozumiałość mowy na słabych głośnikach.

Gdy oba poziomy = 0, moduł jest całkowicie pomijany (zero narzutu).

---

## Skróty klawiszowe

Bookbar rejestruje komendy w menu głównym (**Playback**), które możesz podpiąć pod
dowolne klawisze w **Preferencje → Skróty klawiszowe (Keyboard Shortcuts)**:

| Komenda                          | Sugerowany klawisz  |
|----------------------------------|---------------------|
| Bookbar: Tempo down / up         | `-` / `=`           |
| Bookbar: Pitch down / up         | `Shift`+`-` / `=`   |
| Bookbar: Rate (tape) down / up   | `Ctrl`+`Shift`+ …   |
| Bookbar: Reset tempo/pitch/rate  | (dowolny)           |

Dodatkowo Bookbar instaluje **własne wbudowane skróty**, które działają od razu po
instalacji (gdy foobar2000 jest aktywny i nie piszesz w polu tekstowym):

| Skrót                    | Działanie                          |
|--------------------------|------------------------------------|
| `-` / `=`                | tempo w dół / w górę (krok 0,05x)  |
| `Shift` + `-` / `=`      | wysokość −1 / +1 półton            |
| `Ctrl`+`Shift`+ `-` / `=`| rate (kaseta) w dół / w górę       |
| `Shift` + `Backspace`    | reset wszystkich suwaków           |

Wbudowane skróty można wyłączyć checkboxem w oknie konfiguracji (jeśli wolisz
podpiąć własne klawisze przez Preferencje → Skróty klawiszowe).

---

## Instalacja

Użyj **Plik → Preferencje → Components → Install…**, wskaż
`foo_bookbar.fb2k-component`, zrestartuj foobara2000, a następnie dodaj
**Bookbar** w **Preferencje → Playback → DSP Manager**. foobar2000 sam wypakuje
z jednego pakietu właściwą wersję (32- lub 64-bitową).

Wymaga **foobara2000 v2.0 lub nowszego** (32- lub 64-bitowego).

---

## Budowanie

Wymagania: Windows, Visual Studio 2022 (toolset v143), 7-Zip.

Kod SoundTouch i Bungee dołączony jest jako **submoduły git**; foobar2000 SDK
pobiera skrypt (jego licencja zabrania redystrybucji):

```
git clone --recurse-submodules https://github.com/michaldziwisz/bookbar
cd bookbar
```

(Jeśli sklonowałeś bez `--recurse-submodules`:
`git submodule update --init --recursive`.)

Następnie ze zwykłego wiersza poleceń:

```
REM 0) pobierz foobar2000 SDK do sdk\
fetch_sdk.bat

REM 1) nałóż łatkę zgodności z MSVC na Bungee (upstream celuje w GCC/Clang)
cd third_party\bungee && git apply ..\..\patches\bungee-msvc.patch && cd ..\..

REM 2) zbuduj biblioteki foobar2000 SDK (pfc, shared, SDK) — x64 /MD
_build_sdk.bat

REM 3) zbuduj biblioteki third-party (SoundTouch, Bungee) — x64 /MD
third_party\_build_all_libs.bat

REM 4) zbuduj komponent (x64)
_build_plugin.bat

REM 5) OPCJONALNIE — zbuduj też cały stos 32-bit + komponent za jednym razem
_build_all_x86.bat

REM 6) OPCJONALNIE — spakuj oba DLL w jeden dwuarchitekturowy .fb2k-component
_package_dual.bat
```

Wynik: `build\foo_bookbar.dll` (x64), `build_x86\foo_bookbar.dll` (x86), a po
kroku 6 dwuarchitekturowy `dist\foo_bookbar.fb2k-component`. Workflow CI
(`.github/workflows/build.yml`) buduje obie architektury i pakuje je tak samo.

Komponenty foobara2000 używają **dynamicznego CRT (/MD)** — host dostarcza
środowisko uruchomieniowe VC++. (To różnica względem wersji Winamp, która jest
linkowana statycznie.)

Budowanie odbywa się też automatycznie w GitHub Actions — po wypchnięciu tagu
`vX.Y.Z` powstaje publiczny Release z gotowym komponentem.

---

## Licencja

Bookbar jest wydany na licencji **GNU GPL v3** (patrz [LICENSE](LICENSE)).

Korzysta z bibliotek open source (foobar2000 SDK — BSD-style, SoundTouch — LGPL
2.1, Bungee — MPL 2.0 i inne). Pełna lista i noty: [NOTICE.md](NOTICE.md).
