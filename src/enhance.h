// enhance.h — modul "wzbogacania dzwieku" dla audiobookow (domyslnie wylaczony).
// Dwa niezalezne efekty, kazdy 5-stopniowy (0 = off):
//   1) LOUDNESS: brickwall look-ahead limiter true-peak (maksymalizacja glosnosci)
//      z ochrona inter-sample peaks (ISP, 4x oversampling).
//   2) CLARITY: bateria filtrow allpass (phase rotator) symetryzujaca przebieg,
//      na wzor procesorow nadawczych (Orban/Omnia) — poprawia czytelnosc mowy
//      na slabym sprzecie odtwarzajacym.
// Sygnal: float interleaved [-1,1]. Wolane w Processorze PO time-stretchu.
// Gdy oba poziomy = 0 -> modul jest CALKOWICIE pomijany (zero narzutu, zero zmian).
#pragma once
#include <vector>

namespace bookbar {

constexpr int ENH_LEVELS = 5;   // poziomy 1..5 (0 = off)

class Enhancer {
public:
    void configure(int srate, int nch);      // re-init przy zmianie formatu
    void setLoudness(int level);             // 0..5 (0=off)
    void setClarity(int level);              // 0..5 (0=off)
    bool active() const { return loud_ > 0 || clar_ > 0; }
    // Przetwarza w miejscu 'frames' ramek interleaved. Kolejnosc (jak w broadcast):
    // najpierw CLARITY (phase rotator), potem LOUDNESS (limiter).
    void process(float *interleaved, int frames);
    void reset();                            // porzuc stan (zmiana utworu/seek)

private:
    int srate_ = 0, nch_ = 0;
    int loud_ = 0, clar_ = 0;

    // ---------------- CLARITY: kaskada allpassow 2. rzedu (per kanal) ----------
    struct Biquad {                          // allpass RBJ (a0-znormalizowany)
        float b0=1, b1=0, b2=0, a1=0, a2=0;
        // stan per kanal (max 2 kanaly)
        float x1[2]={0,0}, x2[2]={0,0}, y1[2]={0,0}, y2[2]={0,0};
        inline float run(int ch, float x) {
            float y = b0*x + b1*x1[ch] + b2*x2[ch] - a1*y1[ch] - a2*y2[ch];
            x2[ch]=x1[ch]; x1[ch]=x; y2[ch]=y1[ch]; y1[ch]=y;
            return y;
        }
        void clear() { for(int c=0;c<2;++c){x1[c]=x2[c]=y1[c]=y2[c]=0;} }
    };
    std::vector<Biquad> ap_;                  // bateria allpassow wg poziomu
    void buildClarity();

    // ---------------- LOUDNESS: limiter true-peak (mono lub linked stereo) -----
    // stan limitera — patrz enhance.cpp (Limiter). Trzymany jako pImpl-lite inline.
    struct Limiter;
    Limiter *lim_ = nullptr;
    void buildLoudness();

public:
    ~Enhancer();
};

} // namespace bookbar
