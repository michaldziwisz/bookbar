// i18n.h — dwujezyczny interfejs (PL/EN). Polski TYLKO gdy jezyk UI Windows =
// polski; kazdy inny jezyk -> angielski. Wykrycie raz przy starcie wtyczki.
// Napisy jako Unicode (\u), zero zaleznosci. Uzycie: T(S_ENABLED) itd.
#pragma once
#include <windows.h>

namespace bookbar {

enum StrId {
    S_ENABLED = 0, S_ALGO_CAP, S_TEMPO_CAP, S_PITCH_CAP, S_RATE_CAP,
    S_ENH_LOUD_CAP, S_ENH_CLAR_CAP, S_RESET_BTN, S_ADVANCED_BTN,
    S_ALGO_ST, S_ALGO_BG,
    S_LV_OFF, S_LV_SUBTLE, S_LV_MODERATE, S_LV_MEDIUM, S_LV_STRONG, S_LV_MAX,
    S_ADV_INFO_ST, S_ADV_INFO_BG,
    S_QUICKSEEK, S_AAFILTER, S_AALEN_CAP, S_SEQ_CAP, S_SEEK_CAP, S_OVL_CAP,
    S_HOP_CAP, S_DEFAULTS_BTN, S_ADV_CAPTION,
    S_PITCH_ZERO, S_PITCH_SEMI, S_PITCH_CENTS,
    S_SHORTCUTS, S_MAIN_CAPTION,
    STR_COUNT
};

static const wchar_t* const kStrPL[STR_COUNT] = {
    /*S_ENABLED*/      L"W\u0142\u0105cz Bookbar",
    /*S_ALGO_CAP*/     L"Algorytm:",
    /*S_TEMPO_CAP*/    L"Tempo:",
    /*S_PITCH_CAP*/    L"Wysoko\u015B\u0107 (0 = bez zmian):",
    /*S_RATE_CAP*/     L"Rate (kaseta \u2013 tempo i wysoko\u015B\u0107):",
    /*S_ENH_LOUD_CAP*/ L"Wyr\u00F3wnywanie g\u0142o\u015Bno\u015Bci:",
    /*S_ENH_CLAR_CAP*/ L"Czytelno\u015B\u0107:",
    /*S_RESET_BTN*/    L"Reset (Shift+Backspace)",
    /*S_ADVANCED_BTN*/ L"Parametry zaawansowane...",
    /*S_ALGO_ST*/      L"SoundTouch (WSOLA) \u2013 mowa",
    /*S_ALGO_BG*/      L"Bungee (phase-vocoder) \u2013 domy\u015Blny",
    /*S_LV_OFF*/       L"Wy\u0142\u0105czone",
    /*S_LV_SUBTLE*/    L"Delikatnie",
    /*S_LV_MODERATE*/  L"Umiarkowanie",
    /*S_LV_MEDIUM*/    L"\u015Arednio",
    /*S_LV_STRONG*/    L"Mocno",
    /*S_LV_MAX*/       L"Maksymalnie",
    /*S_ADV_INFO_ST*/  L"Parametry silnika SoundTouch (WSOLA). 0 = warto\u015B\u0107 domy\u015Blna.",
    /*S_ADV_INFO_BG*/  L"Parametry silnika Bungee. Hop 0 = najlepsza jako\u015B\u0107.",
    /*S_QUICKSEEK*/    L"QuickSeek (szybciej, ni\u017Csza jako\u015B\u0107)",
    /*S_AAFILTER*/     L"Filtr antyaliasingowy",
    /*S_AALEN_CAP*/    L"D\u0142ugo\u015B\u0107 filtra AA:",
    /*S_SEQ_CAP*/      L"Sekwencja (ms):",
    /*S_SEEK_CAP*/     L"Okno wyszukiwania (ms):",
    /*S_OVL_CAP*/      L"Na\u0142o\u017Cenie/overlap (ms):",
    /*S_HOP_CAP*/      L"Ziarnisto\u015B\u0107 (hop, 0=najlepsza jako\u015B\u0107):",
    /*S_DEFAULTS_BTN*/ L"Domy\u015Blne",
    /*S_ADV_CAPTION*/  L"Bookbar \u2013 parametry zaawansowane",
    /*S_PITCH_ZERO*/   L"0 (bez zmian)",
    /*S_PITCH_SEMI*/   L"%+d p\u00F3\u0142tonu (%+d cent\u00F3w)",
    /*S_PITCH_CENTS*/  L"%+d cent\u00F3w",
    /*S_SHORTCUTS*/    L"Skr\u00F3ty klawiszowe (-, =, Shift, Ctrl+Shift)",
    /*S_MAIN_CAPTION*/ L"Bookbar \u2013 tempo, wysoko\u015B\u0107, wzbogacanie",
};

static const wchar_t* const kStrEN[STR_COUNT] = {
    /*S_ENABLED*/      L"Enable Bookbar",
    /*S_ALGO_CAP*/     L"Algorithm:",
    /*S_TEMPO_CAP*/    L"Tempo:",
    /*S_PITCH_CAP*/    L"Pitch (0 = no change):",
    /*S_RATE_CAP*/     L"Rate (tape \u2013 tempo & pitch):",
    /*S_ENH_LOUD_CAP*/ L"Loudness:",
    /*S_ENH_CLAR_CAP*/ L"Speech intelligibility:",
    /*S_RESET_BTN*/    L"Reset (Shift+Backspace)",
    /*S_ADVANCED_BTN*/ L"Advanced parameters...",
    /*S_ALGO_ST*/      L"SoundTouch (WSOLA) \u2013 speech",
    /*S_ALGO_BG*/      L"Bungee (phase-vocoder) \u2013 default",
    /*S_LV_OFF*/       L"Off",
    /*S_LV_SUBTLE*/    L"Subtle",
    /*S_LV_MODERATE*/  L"Moderate",
    /*S_LV_MEDIUM*/    L"Medium",
    /*S_LV_STRONG*/    L"Strong",
    /*S_LV_MAX*/       L"Maximum",
    /*S_ADV_INFO_ST*/  L"SoundTouch (WSOLA) engine parameters. 0 = default value.",
    /*S_ADV_INFO_BG*/  L"Bungee engine parameters. Hop 0 = best quality.",
    /*S_QUICKSEEK*/    L"QuickSeek (faster, lower quality)",
    /*S_AAFILTER*/     L"Anti-aliasing filter",
    /*S_AALEN_CAP*/    L"AA filter length:",
    /*S_SEQ_CAP*/      L"Sequence (ms):",
    /*S_SEEK_CAP*/     L"Seek window (ms):",
    /*S_OVL_CAP*/      L"Overlap (ms):",
    /*S_HOP_CAP*/      L"Granularity (hop, 0=best quality):",
    /*S_DEFAULTS_BTN*/ L"Defaults",
    /*S_ADV_CAPTION*/  L"Bookbar \u2013 advanced parameters",
    /*S_PITCH_ZERO*/   L"0 (no change)",
    /*S_PITCH_SEMI*/   L"%+d semitones (%+d cents)",
    /*S_PITCH_CENTS*/  L"%+d cents",
    /*S_SHORTCUTS*/    L"Keyboard shortcuts (-, =, Shift, Ctrl+Shift)",
    /*S_MAIN_CAPTION*/ L"Bookbar \u2013 tempo, pitch, enhancement",
};

// true = polski interfejs. Ustawiane raz (jezyk UI Windows = polski).
inline bool& langIsPolish() {
    static bool pl = (PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_POLISH);
    return pl;
}

inline const wchar_t* T(StrId id) {
    return (langIsPolish() ? kStrPL : kStrEN)[id];
}

} // namespace bookbar
