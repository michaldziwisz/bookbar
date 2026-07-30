// preset.h — serializacja parametrow Bookbar do/z dsp_preset foobara.
// Foobar trzyma konfiguracje DSP jako blob bajtow (dsp_preset). Zapisujemy
// wszystkie pola g_params w wersjonowanej, stalej strukturze (little-endian,
// desktop). pitch trzymany jako CENTY (int, dokladnie), tempo/rate jako double.
#pragma once
#include <cstdint>
#include <cstring>
#include "params.h"
#include "mapping.h"

namespace bookbar {

#pragma pack(push, 1)
struct PresetBlob {
    uint32_t magic;      // 'BKBR'
    uint32_t version;    // = 1
    int32_t  algo;
    uint8_t  enabled;
    double   tempo;
    int32_t  pitch_cents;
    double   rate;
    uint8_t  st_quickseek;
    uint8_t  st_aafilter;
    int32_t  st_aa_len;
    int32_t  st_sequence_ms;
    int32_t  st_seekwindow_ms;
    int32_t  st_overlap_ms;
    int32_t  bg_hop_adjust;
    int32_t  enh_loud;
    int32_t  enh_clarity;
    uint8_t  shortcuts;  // 1 = wlasny globalny hook skrotow wlaczony (domyslnie 1)
};
#pragma pack(pop)

static const uint32_t BKBR_MAGIC = 0x52424B42u; // 'B''K''B''R' LE

// Zapelnij blob z globalnych g_params.
inline void blobFromParams(PresetBlob &b) {
    memset(&b, 0, sizeof(b));
    b.magic = BKBR_MAGIC; b.version = 1;
    b.algo            = g_params.algo.load();
    b.enabled         = g_params.enabled.load() ? 1 : 0;
    b.tempo           = g_params.tempo.load();
    b.pitch_cents     = mulToPitchCents(g_params.pitch.load());
    b.rate            = g_params.rate.load();
    b.st_quickseek    = g_params.st_quickseek.load() ? 1 : 0;
    b.st_aafilter     = g_params.st_aafilter.load() ? 1 : 0;
    b.st_aa_len       = g_params.st_aa_len.load();
    b.st_sequence_ms  = g_params.st_sequence_ms.load();
    b.st_seekwindow_ms= g_params.st_seekwindow_ms.load();
    b.st_overlap_ms   = g_params.st_overlap_ms.load();
    b.bg_hop_adjust   = g_params.bg_hop_adjust.load();
    b.enh_loud        = g_params.enh_loud.load();
    b.enh_clarity     = g_params.enh_clarity.load();
    b.shortcuts       = g_params.shortcuts.load() ? 1 : 0;
}

// Wgraj blob do globalnych g_params (waliduje magic/version).
inline bool paramsFromBlob(const PresetBlob &b) {
    if (b.magic != BKBR_MAGIC || b.version < 1) return false;
    g_params.algo.store(b.algo == ALGO_SOUNDTOUCH ? ALGO_SOUNDTOUCH : ALGO_BUNGEE);
    g_params.enabled.store(b.enabled != 0);
    g_params.tempo.store(b.tempo);
    g_params.pitch.store(pitchCentsToMul(b.pitch_cents));
    g_params.rate.store(b.rate);
    g_params.st_quickseek.store(b.st_quickseek != 0);
    g_params.st_aafilter.store(b.st_aafilter != 0);
    g_params.st_aa_len.store(b.st_aa_len);
    g_params.st_sequence_ms.store(b.st_sequence_ms);
    g_params.st_seekwindow_ms.store(b.st_seekwindow_ms);
    g_params.st_overlap_ms.store(b.st_overlap_ms);
    g_params.bg_hop_adjust.store(b.bg_hop_adjust);
    g_params.enh_loud.store(b.enh_loud);
    g_params.enh_clarity.store(b.enh_clarity);
    g_params.shortcuts.store(b.shortcuts != 0);
    g_params.bumpGen();
    return true;
}

} // namespace bookbar
