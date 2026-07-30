// resource.h — identyfikatory kontrolek GUI Bookbar.
#pragma once

#define IDD_MAIN            2000
#define IDD_ADVANCED        2001

#define IDC_ALGO            2100  // combo wyboru algorytmu
#define IDC_ENABLED         2101  // checkbox wlacz/wylacz
#define IDC_TEMPO_SLIDER    2102
#define IDC_TEMPO_LABEL     2103  // static: realna wartosc tempa (dla NVDA)
#define IDC_PITCH_SLIDER    2104
#define IDC_PITCH_LABEL     2105  // static: realna wartosc wysokosci (dla NVDA)
#define IDC_RESET           2106  // przycisk reset
#define IDC_ADVANCED_BTN    2107  // przycisk parametrow zaawansowanych

#define IDC_TEMPO_CAPTION   2108  // static etykieta "Tempo"
#define IDC_PITCH_CAPTION   2109  // static etykieta "Wysokosc"
#define IDC_ALGO_CAPTION    2110  // static etykieta "Algorytm"
#define IDC_RATE_SLIDER     2111
#define IDC_RATE_LABEL      2112  // static: realna wartosc rate (dla NVDA)
#define IDC_RATE_CAPTION    2113  // static etykieta "Rate"

// Advanced dialog — SoundTouch
#define IDC_ADV_INFO        2204
#define IDC_ADV_QUICKSEEK   2200
#define IDC_ADV_AAFILTER    2201
#define IDC_ADV_AALEN_CAP   2210
#define IDC_ADV_AALEN       2211
#define IDC_ADV_AALEN_LBL   2212
#define IDC_ADV_SEQ_CAP     2213
#define IDC_ADV_SEQUENCE    2202
#define IDC_ADV_SEQ_LABEL   2203
#define IDC_ADV_SEEK_CAP    2214
#define IDC_ADV_SEEKWIN     2215
#define IDC_ADV_SEEK_LBL    2216
#define IDC_ADV_OVL_CAP     2217
#define IDC_ADV_OVERLAP     2218
#define IDC_ADV_OVL_LBL     2219
// Advanced dialog — Bungee
#define IDC_ADV_HOP_CAP     2220
#define IDC_ADV_HOP         2221
#define IDC_ADV_HOP_LBL     2222
// wspolne
#define IDC_ADV_DEFAULTS    2230

// --- Wzbogacanie dzwieku (okno glowne) ---
#define IDC_ENH_LOUD_CAP    2300  // static etykieta "Wyrownywanie glosnosci"
#define IDC_ENH_LOUD        2301  // suwak 0..5
#define IDC_ENH_LOUD_LBL    2302  // static: nazwa poziomu (dla NVDA)
#define IDC_ENH_CLAR_CAP    2303  // static etykieta "Czytelnosc"
#define IDC_ENH_CLAR        2304  // suwak 0..5
#define IDC_ENH_CLAR_LBL    2305  // static: nazwa poziomu (dla NVDA)

// --- Skroty klawiszowe (okno glowne) ---
#define IDC_SHORTCUTS       2400  // checkbox: wlasny globalny hook skrotow
