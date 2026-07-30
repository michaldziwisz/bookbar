// test_processfloat.cpp — test sciezki foobar (Processor::processFloat, float natywny).
// Weryfikuje: (1) passthrough bit-identyczny gdy stretch off, (2) przyspieszanie
// tempa produkuje ~proporcjonalnie mniej ramek (zachowanie energii), (3) zwalnianie
// 0.5x produkuje WIECEJ ramek (brak sufitu 2x — roznica vs Winamp), (4) enhance
// przy tempie 1.0 nie zmienia liczby ramek. Bez GUI/foobara.
#include "processor.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace bookbar;

static void genSine(std::vector<float>& buf, int frames, int nch, double& phase, double freq, int sr) {
    buf.resize((size_t)frames * nch);
    double dp = 2.0 * M_PI * freq / sr;
    for (int i = 0; i < frames; ++i) {
        float s = (float)(std::sin(phase) * 0.5); phase += dp;
        for (int c = 0; c < nch; ++c) buf[(size_t)i * nch + c] = s;
    }
}

// Zwraca laczna liczbe ramek wyjsciowych po przepuszczeniu 'seconds' sekund sygnalu.
static long long runStream(int algo, double tempo, double pitch, double rate,
                           int enhLoud, int enhClar, int sr, int nch, double seconds,
                           double* outEnergy = nullptr) {
    g_params.algo.store(algo);
    g_params.enabled.store(true);
    g_params.tempo.store(tempo);
    g_params.pitch.store(pitch);
    g_params.rate.store(rate);
    g_params.enh_loud.store(enhLoud);
    g_params.enh_clarity.store(enhClar);
    g_params.bumpGen();

    Processor p;
    const int block = 1024;
    const int total = (int)(sr * seconds);
    std::vector<float> in, out;
    double phase = 0.0;
    long long outFrames = 0; double energy = 0.0;
    for (int done = 0; done < total; done += block) {
        int nf = std::min(block, total - done);
        genSine(in, nf, nch, phase, 220.0, sr);
        int of = p.processFloat(in.data(), nf, nch, sr, out, false);
        outFrames += of;
        if (outEnergy) for (int i = 0; i < of * nch; ++i) energy += (double)out[i]*out[i];
    }
    if (outEnergy) *outEnergy = energy;
    return outFrames;
}

int main() {
    const int sr = 44100, nch = 2;
    int fails = 0;

    // (1) PASSTHROUGH bit-identyczny gdy stretch off (tempo=pitch=rate=1, enh off).
    {
        g_params.algo.store(ALGO_BUNGEE); g_params.enabled.store(true);
        g_params.tempo.store(1.0); g_params.pitch.store(1.0); g_params.rate.store(1.0);
        g_params.enh_loud.store(0); g_params.enh_clarity.store(0); g_params.bumpGen();
        Processor p;
        std::vector<float> in, out; double ph = 0;
        genSine(in, 1024, nch, ph, 440.0, sr);
        int of = p.processFloat(in.data(), 1024, nch, sr, out, false);
        bool ok = (of == 1024);
        for (int i = 0; i < 1024*nch && ok; ++i) if (out[i] != in[i]) ok = false;
        printf("[%s] passthrough bit-identyczny (of=%d)\n", ok?"PASS":"FAIL", of);
        if (!ok) ++fails;
    }

    // (2) PRZYSPIESZANIE 2x -> ~polowa ramek (Bungee).
    {
        long long o1 = runStream(ALGO_BUNGEE, 1.0, 1.0, 1.0, 0, 0, sr, nch, 3.0);
        long long o2 = runStream(ALGO_BUNGEE, 2.0, 1.0, 1.0, 0, 0, sr, nch, 3.0);
        double ratio = (double)o2 / (double)o1;
        bool ok = (ratio > 0.42 && ratio < 0.58);  // ~0.5
        printf("[%s] tempo 2x: outFrames ratio=%.3f (~0.5), o1=%lld o2=%lld\n",
               ok?"PASS":"FAIL", ratio, o1, o2);
        if (!ok) ++fails;
    }

    // (3) ZWALNIANIE 0.5x -> ~2x ramek. KLUCZOWE: foobar NIE ma sufitu 2x.
    {
        long long o1 = runStream(ALGO_BUNGEE, 1.0, 1.0, 1.0, 0, 0, sr, nch, 3.0);
        long long o05 = runStream(ALGO_BUNGEE, 0.5, 1.0, 1.0, 0, 0, sr, nch, 3.0);
        double ratio = (double)o05 / (double)o1;
        bool ok = (ratio > 1.7 && ratio < 2.3);  // ~2.0
        printf("[%s] tempo 0.5x: outFrames ratio=%.3f (~2.0, bez sufitu), o1=%lld o05=%lld\n",
               ok?"PASS":"FAIL", ratio, o1, o05);
        if (!ok) ++fails;
    }

    // (4) SoundTouch przyspieszanie 1.5x -> ~2/3 ramek.
    {
        long long o1 = runStream(ALGO_SOUNDTOUCH, 1.0, 1.0, 1.0, 0, 0, sr, nch, 3.0);
        long long o15 = runStream(ALGO_SOUNDTOUCH, 1.5, 1.0, 1.0, 0, 0, sr, nch, 3.0);
        double ratio = (double)o15 / (double)o1;
        bool ok = (ratio > 0.58 && ratio < 0.75);  // ~0.667
        printf("[%s] SoundTouch tempo 1.5x: ratio=%.3f (~0.667)\n", ok?"PASS":"FAIL", ratio);
        if (!ok) ++fails;
    }

    // (5) ENHANCE przy tempie 1.0: liczba ramek niezmieniona (in-place), energia > 0.
    {
        double e = 0;
        long long of = runStream(ALGO_BUNGEE, 1.0, 1.0, 1.0, 3, 3, sr, nch, 1.0, &e);
        long long expect = (long long)(sr * 1.0);
        bool ok = (llabs(of - expect) < 2048) && (e > 0.0);
        printf("[%s] enhance @1.0x: of=%lld (~%lld), energia=%.1f>0\n",
               ok?"PASS":"FAIL", of, expect, e);
        if (!ok) ++fails;
    }

    printf(fails ? "\nRESULT: %d FAIL\n" : "\nRESULT: ALL_PASS\n", fails);
    return fails ? 1 : 0;
}
