// mapping.h — czysta matematyka krzywej suwakow (testowalna bez GUI).
// Suwak: int 0..SLIDER_MAX (500 skokow strzalka=1). Mapping analityczny (double) = gladki.
// Krzywa ostrzej niz liniowa, nie logarytmiczna: potega gamma (~1.7) na znorm. odcinku.
// TEMPO: neutral 1.0x na 25%. Dolne 25%: 0.5x->1.0x, gorne 75%: 1.0x->4.0x (sufit API=0.5x).
// WYSOKOSC: neutral 0 pol na 50% (symetrycznie). Dolne 50%: -12..0, gorne 50%: 0..+12.
#pragma once
#include <cmath>

namespace bookbar {

constexpr int    SLIDER_MAX     = 500;   // 500 pozycji: powolny, masywny suwak
constexpr int    SLIDER_LINE    = 1;     // strzalka = 1 pozycja (powolutku, zawsze rusza)
constexpr int    SLIDER_PAGE    = SLIDER_MAX / 20;  // PageUp/Down = 5% (25 pozycji)
constexpr double CURVE_GAMMA    = 1.7;

constexpr double TEMPO_NEUTRAL  = 0.25;  // 1.0x na 25% suwaka
constexpr double TEMPO_MIN = 0.5, TEMPO_MID = 1.0, TEMPO_MAX = 8.0;
// UWAGA: limit DSP "max 2x numsamples" dotyczy tylko ZWALNIANIA (tempo<1 = wiecej
// probek). Przyspieszanie (tempo>1 = mniej probek) NIE ma limitu -> gora do 8x.

// WYSOKOSC: w CENTACH (czytelne dla muzyka). Suwak "masywny" jak tempo: 480
// pozycji (-240..+240), 1 pozycja = 5 centow -> zakres +-1200 centow (oktawa),
// srodek = 0. Strzalka rusza o 1 pozycje (5 centow), zawsze osiagalne 0.
constexpr int    PITCH_STEPS    = 240;                // +-240 krokow
constexpr int    PITCH_CENTS_PER_STEP = 5;            // 1 krok = 5 centow
constexpr int    PITCH_CENTS    = PITCH_STEPS * PITCH_CENTS_PER_STEP; // +-1200 centow
constexpr int    PITCH_SLIDER_MAX = 2 * PITCH_STEPS;  // 0..480
constexpr int    PITCH_LINE     = 1;                  // strzalka = 1 pozycja (5 centow)
constexpr int    PITCH_PAGE     = 20;                 // PageUp/Down = 20 pozycji (100c=polton)

// RATE (kaseta): zmienia tempo I wysokosc razem (resampling z antialiasingiem).
// Krzywa jak tempo (gamma), neutral 1.0x na 25%. Dol 0.5x (limit DSP), gora 4.0x.
constexpr double RATE_NEUTRAL   = 0.25;
constexpr double RATE_MIN = 0.5, RATE_MID = 1.0, RATE_MAX = 4.0;
constexpr double RATE_STEP = 0.05;

inline double sliderPos01(int pos) {
    double p = (double)pos / SLIDER_MAX;
    return p < 0 ? 0 : (p > 1 ? 1 : p);
}

// wspolna krzywa gamma dla tempa i rate (neutral na 25%)
inline double posToRatio(int pos, double lo, double mid, double hi) {
    double p = sliderPos01(pos);
    if (p <= TEMPO_NEUTRAL) {
        double v = (TEMPO_NEUTRAL - p) / TEMPO_NEUTRAL;
        return mid - (mid - lo) * std::pow(v, CURVE_GAMMA);
    }
    double u = (p - TEMPO_NEUTRAL) / (1.0 - TEMPO_NEUTRAL);
    return mid + (hi - mid) * std::pow(u, CURVE_GAMMA);
}
inline int ratioToPos(double t, double lo, double mid, double hi) {
    if (t < lo) t = lo; if (t > hi) t = hi;
    double p;
    if (t <= mid) p = TEMPO_NEUTRAL - std::pow((mid - t) / (mid - lo), 1.0 / CURVE_GAMMA) * TEMPO_NEUTRAL;
    else          p = TEMPO_NEUTRAL + std::pow((t - mid) / (hi - mid), 1.0 / CURVE_GAMMA) * (1.0 - TEMPO_NEUTRAL);
    int pos = (int)(p * SLIDER_MAX + 0.5);
    return pos < 0 ? 0 : (pos > SLIDER_MAX ? SLIDER_MAX : pos);
}

inline double posToTempo(int pos) { return posToRatio(pos, TEMPO_MIN, TEMPO_MID, TEMPO_MAX); }
inline int    tempoToPos(double t) { return ratioToPos(t, TEMPO_MIN, TEMPO_MID, TEMPO_MAX); }
inline double posToRate(int pos)  { return posToRatio(pos, RATE_MIN, RATE_MID, RATE_MAX); }
inline int    rateToPos(double r) { return ratioToPos(r, RATE_MIN, RATE_MID, RATE_MAX); }

// pozycja suwaka wysokosci (0..480) -> centy (-1200..+1200); 1 pozycja = 5 centow
inline int posToPitchCents(int pos) { return (pos - PITCH_STEPS) * PITCH_CENTS_PER_STEP; }
// odwrotnosc: centy -> pozycja suwaka (zaokraglona do kroku)
inline int pitchCentsToPos(int cents) {
    if (cents < -PITCH_CENTS) cents = -PITCH_CENTS; if (cents > PITCH_CENTS) cents = PITCH_CENTS;
    return (int)lround((double)cents / PITCH_CENTS_PER_STEP) + PITCH_STEPS;
}
// centy -> mnoznik czestotliwosci (oktawa = 1200 centow)
inline double pitchCentsToMul(int cents) { return std::pow(2.0, (double)cents / 1200.0); }
// mnoznik -> centy (calkowite)
inline int mulToPitchCents(double mul) { return (int)lround(1200.0 * (std::log(mul) / std::log(2.0))); }

constexpr double TEMPO_STEP = 0.05;
inline int tempoNeutralPos() { return (int)(TEMPO_NEUTRAL * SLIDER_MAX + 0.5); }
inline int rateNeutralPos()  { return (int)(RATE_NEUTRAL * SLIDER_MAX + 0.5); }
inline int pitchNeutralPos() { return PITCH_STEPS; }  // srodek = 0 centow

// Snap-to-neutral: gdy krok przechodzi przez wartosc neutralna, zatrzymaj sie
// DOKLADNIE na niej (detent). Gwarantuje osiagalnosc 1.0x / 0 centow strzalka.
inline double stepToward(double cur, double step, double neutral) {
    double next = cur + step;
    if ((cur < neutral && next > neutral) || (cur > neutral && next < neutral)) return neutral;
    return next;
}

} // namespace bookbar
