// params.h — wspoldzielone parametry (GUI <-> watek audio). Atomiki, bez zamkow.
#pragma once
#include <atomic>
#include "engine.h"

namespace bookbar {

struct Params {
    std::atomic<int>    algo{ALGO_BUNGEE};   // domyslny algorytm (decyzja Michala 26.07)
    std::atomic<bool>   enabled{true};
    std::atomic<double> tempo{1.0};
    std::atomic<double> pitch{1.0};
    std::atomic<double> rate{1.0};    // kaseta: tempo+wysokosc razem (resampling AA)

    // --- SoundTouch (jak w Reaperze) ---
    std::atomic<bool> st_quickseek{false};
    std::atomic<bool> st_aafilter{true};
    std::atomic<int>  st_aa_len{32};      // 8..128 taps
    std::atomic<int>  st_sequence_ms{0};  // 0 = auto/domyslne SoundToucha
    std::atomic<int>  st_seekwindow_ms{0};
    std::atomic<int>  st_overlap_ms{0};

    // --- Bungee ---
    std::atomic<int>  bg_resample_mode{0}; // ResampleMode: 0..4 (autoOut..forceIn)
    std::atomic<int>  bg_hop_adjust{0};    // log2SynthesisHopAdjust: -2..+2 (0=najlepsze)

    // --- Wzbogacanie dzwieku (domyslnie WYLACZONE, oba 0) ---
    std::atomic<int>  enh_loud{0};    // wyrownywanie/limiter true-peak: 0=off, 1..5
    std::atomic<int>  enh_clarity{0}; // czytelnosc/phase rotator:       0=off, 1..5

    // --- Skroty klawiszowe (foobar) ---
    // Wlasny globalny hook z domyslnymi klawiszami (-, =, Shift+, Ctrl+Shift+).
    // Domyslnie WLACZONY (dziala od razu po instalacji). User moze go wylaczyc
    // w oknie konfiguracji i podpiac wlasne klawisze przez menu foobara
    // (Preferences > Keyboard Shortcuts > komendy Bookbar).
    std::atomic<bool> shortcuts{true};


    // generacja: bump gdy parametr wymaga re-configure silnika (nie tempo/pitch/enabled).
    std::atomic<unsigned> gen{0};
    void bumpGen() { gen.fetch_add(1); }
};

extern Params g_params;

} // namespace bookbar
