// engine.h — abstrakcja silnika time-stretch/pitch dla Bookamp.
// Model streamingowy FIFO: putSamples() != receiveSamples() (wejscie != wyjscie).
// Probki: float interleaved, zakres [-1,1]. Konwersja int16<->float w warstwie DSP.
#pragma once

namespace bookbar {

enum Algo { ALGO_SOUNDTOUCH = 0, ALGO_BUNGEE = 1 };

struct Params; // fwd

class IStretchEngine {
public:
    virtual ~IStretchEngine() {}
    virtual void configure(int srate, int nch) = 0; // re-init przy zmianie srate/nch
    virtual void applySettings(const Params &p) = 0; // parametry zaawansowane silnika
    virtual void setTempo(double tempo) = 0;        // 1.0 = bez zmian, >1 szybciej
    virtual void setPitch(double pitch) = 0;        // 1.0 = bez zmian (mnoznik czestotliwosci)
    virtual void setRate(double rate) = 0;          // kaseta: tempo+pitch razem (resampling AA)
    virtual void putSamples(const float *interleaved, int frames) = 0;
    virtual int  receiveSamples(float *out, int maxFrames) = 0; // zwraca frames odebrane
    virtual int  availableFrames() = 0;             // gotowe do odbioru
    virtual void reset() = 0;                       // porzuc bufory (zmiana utworu)
    virtual const char *name() const = 0;
};

// Fabryka. Zwraca nullptr gdy algorytm niedostepny.
IStretchEngine *createEngine(Algo a);

} // namespace bookbar
