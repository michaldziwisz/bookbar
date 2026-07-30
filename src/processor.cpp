// processor.cpp — rdzen DSP Bookamp.
#include "processor.h"
#include <algorithm>

namespace bookbar {

Params g_params;

// SoundTouch oficjalnie do 48 kHz; powyzej -> passthrough (do zmierzenia pozniej).
static const int ST_MAX_SRATE = 48000;

Processor::~Processor() { delete engine_; }

void Processor::rebuild(int algo, int srate, int nch) {
    delete engine_; engine_ = nullptr;
    algo_ = algo; srate_ = srate; nch_ = nch;
    passthrough_ = (algo == ALGO_SOUNDTOUCH && srate > ST_MAX_SRATE);
    if (passthrough_) return;
    engine_ = createEngine((Algo)algo);
    if (engine_) {
        engine_->configure(srate, nch);
        engine_->applySettings(g_params);
        engine_->setTempo(tempo_);
        engine_->setPitch(pitch_);
        engine_->setRate(rate_);
    }
    gen_ = g_params.gen.load();
    outFifo_.clear();
}

int Processor::process(short *samples, int numsamples, int bps, int nch, int srate) {
    const int algo = g_params.algo.load();
    const bool on = g_params.enabled.load();
    const double tempo = g_params.tempo.load();
    const double pitch = g_params.pitch.load();
    const double rate = g_params.rate.load();
    const int enhLoud = g_params.enh_loud.load();
    const int enhClar = g_params.enh_clarity.load();
    const bool enhOn = (on && bps == 16 && (enhLoud > 0 || enhClar > 0));

    // Time-stretch jest no-op gdy wylaczony / nie-16bit / brak zmiany tempa.
    const bool stretchNoop = (!on) || bps != 16 || (tempo == 1.0 && pitch == 1.0 && rate == 1.0);

    // Reakcja na zmiane formatu/algorytmu.
    if (algo != algo_ || srate != srate_ || nch != nch_ || engine_ == nullptr)
        if (!stretchNoop) rebuild(algo, srate, nch);
    bps_ = bps;

    // --- Sciezka bez time-stretchu: ewentualnie samo wzbogacenie dzwieku ---
    if (stretchNoop || passthrough_ || engine_ == nullptr) {
        if (enhOn) applyEnhance(samples, numsamples, nch, srate);  // in-place, bez zmiany liczby ramek
        return numsamples;
    }

    // aktualizacja parametrow silnika (tanie)
    if (tempo != tempo_) { tempo_ = tempo; engine_->setTempo(tempo); }
    if (pitch != pitch_) { pitch_ = pitch; engine_->setPitch(pitch); }
    if (rate  != rate_)  { rate_  = rate;  engine_->setRate(rate); }
    // parametry zaawansowane zmienione w GUI -> zastosuj (rzadko)
    unsigned g = g_params.gen.load();
    if (g != gen_) { gen_ = g; engine_->applySettings(g_params); }

    // int16 -> float [-1,1]
    const int inSmp = numsamples * nch;
    if ((int)fin_.size() < inSmp) fin_.resize(inSmp);
    for (int i = 0; i < inSmp; ++i) fin_[i] = samples[i] * (1.0f / 32768.0f);
    engine_->putSamples(fin_.data(), numsamples);

    // odbierz WSZYSTKO gotowe do FIFO int16
    const int cap = 4096;
    if ((int)fout_.size() < cap * nch) fout_.resize(cap * nch);
    int got;
    while ((got = engine_->receiveSamples(fout_.data(), cap)) > 0) {
        // wzbogacanie dzwieku na FLOAT [-1,1] przed kwantyzacja do int16
        if (enhOn) {
            if (enhSrate_ != srate || enhNch_ != nch) {
                enhancer_.configure(srate, nch); enhSrate_ = srate; enhNch_ = nch;
                enhLoud_ = enhClar_ = -1;
            }
            if (enhLoud_ != enhLoud) { enhancer_.setLoudness(enhLoud); enhLoud_ = enhLoud; }
            if (enhClar_ != enhClar) { enhancer_.setClarity(enhClar); enhClar_ = enhClar; }
            enhancer_.process(fout_.data(), got);
        }
        const int n = got * nch;
        for (int i = 0; i < n; ++i) {
            float v = fout_[i];
            if (v > 1.0f) v = 1.0f; else if (v < -1.0f) v = -1.0f;
            outFifo_.push_back((short)(v * 32767.0f));
        }
    }

    // wydaj z FIFO maks 2*numsamples ramek (twardy limit Winampa)
    const int maxOut = 2 * numsamples;
    int outFrames = std::min((int)(outFifo_.size() / nch), maxOut);
    const int outSmp = outFrames * nch;
    for (int i = 0; i < outSmp; ++i) { samples[i] = outFifo_.front(); outFifo_.pop_front(); }
    return outFrames; // moze byc 0 na starcie (dozwolone)
}

// Wzbogacanie dzwieku w miejscu (bez zmiany liczby ramek) — dla sciezki bez
// time-stretchu (user chce samo wyrownanie/czytelnosc przy naturalnym tempie).
void Processor::applyEnhance(short *samples, int frames, int nch, int srate) {
    const int enhLoud = g_params.enh_loud.load();
    const int enhClar = g_params.enh_clarity.load();
    if (enhSrate_ != srate || enhNch_ != nch) {
        enhancer_.configure(srate, nch); enhSrate_ = srate; enhNch_ = nch;
        enhLoud_ = enhClar_ = -1;
    }
    if (enhLoud_ != enhLoud) { enhancer_.setLoudness(enhLoud); enhLoud_ = enhLoud; }
    if (enhClar_ != enhClar) { enhancer_.setClarity(enhClar); enhClar_ = enhClar; }
    const int n = frames * nch;
    if ((int)enhBuf_.size() < n) enhBuf_.resize(n);
    for (int i = 0; i < n; ++i) enhBuf_[i] = samples[i] * (1.0f / 32768.0f);
    enhancer_.process(enhBuf_.data(), frames);
    for (int i = 0; i < n; ++i) {
        float v = enhBuf_[i];
        if (v > 1.0f) v = 1.0f; else if (v < -1.0f) v = -1.0f;
        samples[i] = (short)(v * 32767.0f);
    }
}

// ============================================================================
//  Sciezka foobar2000 — float natywny, BEZ limitu 2*numsamples.
// ============================================================================

// Skonfiguruj Enhancer pod aktualny format (leniwie).
void Processor::ensureEnhance(int nch, int srate) {
    if (enhSrate_ != srate || enhNch_ != nch) {
        enhancer_.configure(srate, nch); enhSrate_ = srate; enhNch_ = nch;
        enhLoud_ = enhClar_ = -1;
    }
    const int enhLoud = g_params.enh_loud.load();
    const int enhClar = g_params.enh_clarity.load();
    if (enhLoud_ != enhLoud) { enhancer_.setLoudness(enhLoud); enhLoud_ = enhLoud; }
    if (enhClar_ != enhClar) { enhancer_.setClarity(enhClar); enhClar_ = enhClar; }
}

// Wzbogacanie w miejscu na FLOAT [-1,1] (bez zmiany liczby ramek).
void Processor::applyEnhanceFloat(float *samples, int frames, int nch, int srate) {
    ensureEnhance(nch, srate);
    enhancer_.process(samples, frames);
}

void Processor::resetStream() {
    if (engine_) engine_->reset();
    outFifo_.clear();
    outFifoF_.clear();
}

double Processor::latencySeconds(int srate) const {
    if (srate <= 0 || nch_ <= 0) return 0.0;
    return (double)(outFifoF_.size() / (size_t)nch_) / (double)srate;
}

int Processor::processFloat(const float *in, int inFrames, int nch, int srate,
                            std::vector<float> &outBuf, bool p_flush) {
    const int algo = g_params.algo.load();
    const bool on = g_params.enabled.load();
    const double tempo = g_params.tempo.load();
    const double pitch = g_params.pitch.load();
    const double rate = g_params.rate.load();
    const int enhLoud = g_params.enh_loud.load();
    const int enhClar = g_params.enh_clarity.load();
    const bool enhOn = (on && (enhLoud > 0 || enhClar > 0));

    // Time-stretch jest no-op gdy wylaczony / brak zmiany tempa/pitch/rate.
    const bool stretchNoop = (!on) || (tempo == 1.0 && pitch == 1.0 && rate == 1.0);

    // Reakcja na zmiane formatu/algorytmu (tylko gdy stretch aktywny).
    if (algo != algo_ || srate != srate_ || nch != nch_ || engine_ == nullptr)
        if (!stretchNoop) rebuild(algo, srate, nch);
    bps_ = 32;  // float

    // --- Sciezka bez time-stretchu: passthrough (+ ewentualne wzbogacenie) ---
    if (stretchNoop || passthrough_ || engine_ == nullptr) {
        const int n = inFrames * nch;
        if ((int)outBuf.size() < n) outBuf.resize(n);
        if (inFrames > 0) memcpy(outBuf.data(), in, (size_t)n * sizeof(float));
        if (enhOn && inFrames > 0) applyEnhanceFloat(outBuf.data(), inFrames, nch, srate);
        return inFrames;
    }

    // aktualizacja parametrow silnika (tanie)
    if (tempo != tempo_) { tempo_ = tempo; engine_->setTempo(tempo); }
    if (pitch != pitch_) { pitch_ = pitch; engine_->setPitch(pitch); }
    if (rate  != rate_)  { rate_  = rate;  engine_->setRate(rate); }
    unsigned g = g_params.gen.load();
    if (g != gen_) { gen_ = g; engine_->applySettings(g_params); }

    // Wpychamy wejscie do silnika (float [-1,1], interleaved).
    if (inFrames > 0) engine_->putSamples(in, inFrames);

    // Odbierz WSZYSTKO gotowe do float FIFO.
    const int cap = 4096;
    if ((int)fout_.size() < cap * nch) fout_.resize(cap * nch);
    int got;
    while ((got = engine_->receiveSamples(fout_.data(), cap)) > 0) {
        if (enhOn) {
            ensureEnhance(nch, srate);
            enhancer_.process(fout_.data(), got);
        }
        const int n = got * nch;
        for (int i = 0; i < n; ++i) outFifoF_.push_back(fout_[i]);
        if (got < cap) break;
    }

    // Wydaj CALE FIFO (foobar nie ma sufitu wyjscia — model listy chunkow).
    const int outFrames = (int)(outFifoF_.size() / (size_t)nch);
    const int outSmp = outFrames * nch;
    if ((int)outBuf.size() < outSmp) outBuf.resize(outSmp);
    for (int i = 0; i < outSmp; ++i) { outBuf[i] = outFifoF_.front(); outFifoF_.pop_front(); }
    (void)p_flush;  // silniki WSOLA/PV nie maja jawnego 'drain'; ogon wychodzi naturalnie
    return outFrames;
}

} // namespace bookbar