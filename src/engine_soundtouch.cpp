// engine_soundtouch.cpp — implementacja IStretchEngine na SoundTouch (WSOLA).
// Domyslny algorytm dla mowy/audiobookow.
#include "engine.h"
#include "params.h"
#include <SoundTouch.h>

using namespace soundtouch;

namespace bookbar {

class SoundTouchEngine : public IStretchEngine {
    SoundTouch st;
    int srate_ = 0, nch_ = 0;
public:
    void configure(int srate, int nch) override {
        if (srate == srate_ && nch == nch_) return;
        srate_ = srate; nch_ = nch;
        st.setSampleRate((uint)srate);
        st.setChannels((uint)nch);
    }
    void applySettings(const Params &p) override {
        st.setSetting(SETTING_USE_QUICKSEEK, p.st_quickseek.load() ? 1 : 0);
        st.setSetting(SETTING_USE_AA_FILTER, p.st_aafilter.load() ? 1 : 0);
        int aalen = p.st_aa_len.load(); if (aalen >= 8 && aalen <= 128) st.setSetting(SETTING_AA_FILTER_LENGTH, aalen);
        int seq = p.st_sequence_ms.load();   if (seq > 0) st.setSetting(SETTING_SEQUENCE_MS, seq);
        int sw  = p.st_seekwindow_ms.load(); if (sw  > 0) st.setSetting(SETTING_SEEKWINDOW_MS, sw);
        int ov  = p.st_overlap_ms.load();    if (ov  > 0) st.setSetting(SETTING_OVERLAP_MS, ov);
    }
    void setTempo(double t) override { st.setTempo(t); }
    void setPitch(double p) override { st.setPitch(p); }
    void setRate(double r) override { st.setRate(r); }   // SoundTouch resampling z AA-filter
    void putSamples(const float *in, int frames) override {
        if (frames > 0) st.putSamples(in, (uint)frames);
    }
    int receiveSamples(float *out, int maxFrames) override {
        return (int)st.receiveSamples(out, (uint)maxFrames);
    }
    int availableFrames() override { return (int)st.numSamples(); }
    void reset() override { st.clear(); }
    const char *name() const override { return "SoundTouch (WSOLA)"; }
};

IStretchEngine *createSoundTouchEngine() { return new SoundTouchEngine(); }

} // namespace bookbar
