// processor.h — rdzen DSP: int16<->float, FIFO, limit 2*numsamples, reakcja na format.
#pragma once
#include "engine.h"
#include "enhance.h"
#include "params.h"
#include <vector>
#include <deque>

namespace bookbar {

// Zwraca liczbe ramek zapisanych do samples (w zakresie [0 ; 2*numsamples]).
// bufor samples ma pojemnosc 2*numsamples*nch int16 (gwarancja Winampa).
class Processor {
    IStretchEngine *engine_ = nullptr;
    Enhancer enhancer_;                     // wzbogacanie dzwieku (po time-stretch)
    int enhLoud_ = -1, enhClar_ = -1;       // ostatnio zastosowane poziomy
    int algo_ = -1, srate_ = 0, nch_ = 0, bps_ = 0;
    unsigned gen_ = (unsigned)-1;            // ostatnio zastosowana generacja parametrow
    bool passthrough_ = false;              // srate>48k dla SoundTouch itp.
    double tempo_ = 1.0, pitch_ = 1.0, rate_ = 1.0;

    std::deque<short> outFifo_;             // interleaved int16 gotowe do oddania (Winamp)
    std::deque<float> outFifoF_;            // interleaved float gotowe do oddania (foobar)
    std::vector<float> fin_, fout_;         // bufory konwersji
    std::vector<float> enhBuf_;             // bufor float dla Enhancera
    int enhSrate_ = 0, enhNch_ = 0;         // format pod jaki skonfigurowano Enhancer

    void rebuild(int algo, int srate, int nch);
    void applyEnhance(short *samples, int frames, int nch, int srate);
    void applyEnhanceFloat(float *samples, int frames, int nch, int srate);
    void ensureEnhance(int nch, int srate);
public:
    ~Processor();
    // Glowny punkt wejscia z Winampowego ModifySamples (int16). Zostawiony dla testow.
    int process(short *samples, int numsamples, int bps, int nch, int srate);

    // --- Sciezka foobar2000: float natywny, BEZ limitu 2*numsamples ---
    // Wpycha 'inFrames' ramek float interleaved [-1,1] i zwraca WSZYSTKIE gotowe
    // ramki wyjsciowe do outBuf (resize wg potrzeb). Foobar model listy chunkow
    // nie ma sufitu wyjscia, wiec oddajemy calosc. Zwraca liczbe ramek wyjsciowych.
    // Gdy time-stretch nieaktywny: opcjonalne wzbogacenie in-place, wyjscie=wejscie.
    // p_flush: dodatkowo osusz ogon silnika (koniec utworu/seek-flush).
    int processFloat(const float *in, int inFrames, int nch, int srate,
                     std::vector<float> &outBuf, bool p_flush = false);

    // Ile ramek wyjsciowych jest jeszcze zbuforowanych (dla get_latency).
    double latencySeconds(int srate) const;
    // Porzuc bufory (seek/flush foobara).
    void resetStream();
};

} // namespace bookbar
