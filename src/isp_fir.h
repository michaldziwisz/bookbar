// isp_fir.h — wspolczynniki filtra polyphase 4x do detekcji inter-sample peaks (ISP).
//
// Polyphase windowed-sinc, 12 odczepow x 4 fazy: dla kazdej z 4 faz liczy jedna
// probke miedzysamplowa (fractional delay 1/4, 2/4, 3/4). True-peak = max z |probki|
// oraz 4 interpolat. Uzywane WYLACZNIE do detekcji szczytu miedzysamplowego w
// limiterze (sygnal wyjsciowy NIE jest przez ten filtr przepuszczany).
#pragma once

namespace bookbar {

constexpr int ISP_FIR_TAPS   = 12;   // odczepy historii
constexpr int ISP_FIR_PHASES = 4;    // 4x oversampling (4 probki miedzysamplowe)

// coef[tap][phase]: dla fazy p wartosc miedzysamplowa = sum_tap hist[tap]*coef[tap][p].
static const float ISP_FIR[ISP_FIR_TAPS][ISP_FIR_PHASES] = {
    { -0.02206659f, -0.03212738f, -0.02915955f, -0.01659203f },
    { +0.04205728f, +0.06047273f, +0.05307734f, +0.02850044f },
    { -0.07229435f, -0.10719502f, -0.09681559f, -0.05324435f },
    { +0.10951936f, +0.16516852f, +0.15256524f, +0.08616626f },
    { -0.17800593f, -0.25662315f, -0.23108232f, -0.12913477f },
    { +0.95325112f, +0.78703380f, +0.53468549f, +0.25184369f },
    { +0.25184369f, +0.53468549f, +0.78703380f, +0.95325112f },
    { -0.12913477f, -0.23108232f, -0.25662315f, -0.17800593f },
    { +0.08616626f, +0.15256524f, +0.16516852f, +0.10951936f },
    { -0.05324435f, -0.09681559f, -0.10719502f, -0.07229435f },
    { +0.02850044f, +0.05307734f, +0.06047273f, +0.04205728f },
    { -0.01659203f, -0.02915955f, -0.03212738f, -0.02206659f },
};

} // namespace bookbar
