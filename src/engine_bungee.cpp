// engine_bungee.cpp — implementacja IStretchEngine na Bungee (phase-vocoder).
// Bungee Stream: bufory per-kanal (deinterleaved), model "podaj outputFrameCount".
// Opakowujemy w interfejs FIFO: putSamples() buforuje, process() produkuje do FIFO out.
#include "engine.h"
// Bungee Stream.h jest niehigieniczny (uzywa std::vector, std::round/isnan/floor/ceil
// bez wlasnych include'ow) — MSVC wymaga tych naglowkow ZANIM wlaczymy Bungee.
#include <vector>
#include <deque>
#include <cmath>
#include <algorithm>
#include <bungee/Bungee.h>
#include <bungee/Stream.h>
#include "params.h"

namespace bookbar {

class BungeeEngine : public IStretchEngine {
    int srate_ = 0, nch_ = 0;
    double tempo_ = 1.0, pitch_ = 1.0, rate_ = 1.0;
    int hop_ = 0;                          // log2SynthesisHopAdjust (0 = najlepsza jakosc)
    static const int kMaxBlock = 8192;

    Bungee::Stretcher<Bungee::Basic> *stretcher_ = nullptr;
    Bungee::Stream<Bungee::Basic>    *stream_ = nullptr;

    std::vector<std::vector<float>> inCh_, outCh_;  // bufory per-kanal
    std::deque<float> fifo_;                         // wyjscie interleaved

    void destroy() {
        delete stream_;   stream_ = nullptr;
        delete stretcher_; stretcher_ = nullptr;
    }
    void build() {
        destroy();
        Bungee::SampleRates sr{srate_, srate_};
        stretcher_ = new Bungee::Stretcher<Bungee::Basic>(sr, nch_, hop_);
        stream_ = new Bungee::Stream<Bungee::Basic>(*stretcher_, kMaxBlock, nch_);
        inCh_.assign(nch_, std::vector<float>(kMaxBlock));
        outCh_.assign(nch_, std::vector<float>(kMaxBlock * 2 + 16));
        fifo_.clear();
    }
public:
    ~BungeeEngine() override { destroy(); }

    void configure(int srate, int nch) override {
        if (srate == srate_ && nch == nch_ && stretcher_) return;
        srate_ = srate; nch_ = nch;
        build();
    }
    void applySettings(const Params &p) override {
        int h = p.bg_hop_adjust.load();
        if (h < -2) h = -2; if (h > 2) h = 2;
        if (h != hop_ && stretcher_) { hop_ = h; build(); }
        else hop_ = h;
    }
    void setTempo(double t) override { tempo_ = t; }
    void setPitch(double p) override { pitch_ = p; }
    void setRate(double r) override { rate_ = r; }   // kaseta: mnozy tempo i pitch (nizej)

    void putSamples(const float *in, int frames) override {
        if (!stream_ || frames <= 0) return;
        for (int off = 0; off < frames; off += kMaxBlock) {
            int blk = std::min(kMaxBlock, frames - off);
            for (int c = 0; c < nch_; ++c)
                for (int i = 0; i < blk; ++i)
                    inCh_[c][i] = in[(size_t)(off + i) * nch_ + c];

            double outF = blk / (tempo_ * rate_);       // rate mnozy tempo (kaseta)
            int outCap = (int)std::ceil(outF) + 2;
            std::vector<const float*> ip(nch_);
            std::vector<float*> opv(nch_);
            for (int c = 0; c < nch_; ++c) { ip[c] = inCh_[c].data(); opv[c] = outCh_[c].data(); }

            int produced = stream_->process(ip.data(), opv.data(), blk, outF, pitch_ * rate_);
            produced = std::min(produced, outCap);
            for (int i = 0; i < produced; ++i)
                for (int c = 0; c < nch_; ++c)
                    fifo_.push_back(outCh_[c][i]);
        }
    }
    int receiveSamples(float *out, int maxFrames) override {
        int got = 0;
        while (got < maxFrames && (int)fifo_.size() >= nch_) {
            for (int c = 0; c < nch_; ++c) { out[(size_t)got * nch_ + c] = fifo_.front(); fifo_.pop_front(); }
            ++got;
        }
        return got;
    }
    int availableFrames() override { return nch_ ? (int)(fifo_.size() / nch_) : 0; }
    void reset() override { fifo_.clear(); }
    const char *name() const override { return "Bungee (phase-vocoder)"; }
};

IStretchEngine *createBungeeEngine() { return new BungeeEngine(); }

} // namespace bookbar
