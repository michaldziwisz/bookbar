// dsp_bookbar.cpp — wtyczka DSP foobar2000 (Bookbar).
// Wpina Processor (silniki time-stretch + wzbogacanie) w potok foobara.
// Model: on_chunk() bierze chunk float, przepuszcza przez Processor::processFloat
// (float natywny, BEZ limitu 2x), wynik zapisuje z powrotem do chunka. Gdy silnik
// jeszcze nic nie wyprodukowal (rozgrzewka FIFO) -> chunk usuwany (return false).
// Nadwyzka time-stretchu wychodzi w tym samym chunku (foobar nie ma sufitu wyjscia).
// C++/SDK foobar2000, x64. Licencja: GPLv3.
#include <SDK/foobar2000.h>
#include "processor.h"
#include "params.h"
#include "preset.h"
#include <vector>

// Most do naszego okna konfiguracji (gui.cpp / foobar_glue.cpp).
// Zdefiniowany w foobar_glue.cpp — ten plik nie ciagnie <windows.h> do potoku audio.
namespace bookbar {
    void showConfigPopup(const dsp_preset & data, HWND parent, dsp_preset_edit_callback & cb);
}

namespace {

// GUID wtyczki Bookbar (staly identyfikator DSP w lancuchu foobara).
// {8B1F4C2A-3D6E-4A91-9C2B-7E5F1A0D4C88}
static const GUID guid_bookbar_dsp =
{ 0x8b1f4c2a, 0x3d6e, 0x4a91, { 0x9c, 0x2b, 0x7e, 0x5f, 0x1a, 0x0d, 0x4c, 0x88 } };

using namespace bookbar;

class dsp_bookbar : public dsp_impl_base {
    Processor          m_proc;
    std::vector<float> m_in;     // wejscie skonwertowane do float (audio_sample=double na x64)
    std::vector<float> m_out;    // bufor wyjsciowy processFloat (reuzywany)
    std::vector<audio_sample> m_conv; // wynik skonwertowany z powrotem do audio_sample
    unsigned           m_srate = 0, m_nch = 0;

public:
    dsp_bookbar(const dsp_preset & in) {
        // Wgraj preset -> g_params (globalne, wspoldzielone z GUI/skrotami).
        PresetBlob b;
        if (in.get_data_size() == sizeof(PresetBlob)) {
            memcpy(&b, in.get_data(), sizeof(b));
            paramsFromBlob(b);
        }
    }

    static GUID g_get_guid() { return guid_bookbar_dsp; }
    static void g_get_name(pfc::string_base & out) { out = "Bookbar"; }

    // Zapisz float[] jako audio_sample[] do chunka (konwersja gdy audio_sample=double).
    void writeOut(audio_chunk * chunk, const float * data, t_size frames,
                  unsigned nch, unsigned srate) {
        const t_size n = frames * nch;
        m_conv.resize(n);
        for (t_size i = 0; i < n; ++i) m_conv[i] = (audio_sample)data[i];
        chunk->set_data(m_conv.data(), frames, nch, srate);
    }

    // ---- przetwarzanie chunka ----
    bool on_chunk(audio_chunk * chunk, abort_callback &) {
        const unsigned nch   = chunk->get_channels();
        const unsigned srate = chunk->get_srate();
        const t_size   frames = chunk->get_sample_count();
        if (nch == 0 || srate == 0 || frames == 0) return false;

        // get_data() zwraca interleaved [-1,1] w typie audio_sample (double na x64).
        // Rdzen Bookbar pracuje na float -> konwersja wejscia.
        const audio_sample * src = chunk->get_data();
        const t_size inN = frames * nch;
        m_in.resize(inN);
        for (t_size i = 0; i < inN; ++i) m_in[i] = (float)src[i];

        const int outFrames = m_proc.processFloat(
            m_in.data(), (int)frames, (int)nch, (int)srate, m_out, /*p_flush=*/false);

        m_srate = srate; m_nch = nch;

        if (outFrames <= 0) return false;  // rozgrzewka: brak wyjscia -> usun chunk

        // Zapisz wynik z powrotem (liczba ramek moze byc inna niz wejsciowa).
        writeOut(chunk, m_out.data(), (t_size)outFrames, nch, srate);
        return true;
    }

    void on_endofplayback(abort_callback &) {
        // Osusz ewentualny ogon zbuforowany w silniku i wypchnij jako nowy chunk.
        flushTail();
    }
    void on_endoftrack(abort_callback &) {
        // need_track_change_mark()==false -> ta metoda i tak zwykle nie jest wolana.
    }

    void flush() {
        // Seek/flush: porzuc caly stan strumienia (bez odtwarzania ogona).
        m_proc.resetStream();
    }

    double get_latency() {
        return m_proc.latencySeconds(m_srate ? m_srate : 44100);
    }

    bool need_track_change_mark() { return false; }

    static bool g_get_default_preset(dsp_preset & out) {
        // Domyslne = biezace globalne parametry (albo fabryczne przy pierwszym uzyciu).
        PresetBlob b; blobFromParams(b);
        out.set_owner(g_get_guid());
        out.set_data(&b, sizeof(b));
        return true;
    }

    // Konfiguracja: nasze wlasne okno Win32 (gui.cpp). Most w foobar_glue.cpp.
    static bool g_have_config_popup() { return true; }
    static void g_show_config_popup(const dsp_preset & data, HWND parent,
                                    dsp_preset_edit_callback & cb) {
        bookbar::showConfigPopup(data, parent, cb);
    }

private:
    void flushTail() {
        // Wypchnij cokolwiek zostalo w FIFO (bez nowego wejscia).
        int outFrames = m_proc.processFloat(nullptr, 0, (int)(m_nch ? m_nch : 2),
                                            (int)(m_srate ? m_srate : 44100),
                                            m_out, /*p_flush=*/true);
        if (outFrames > 0 && m_nch && m_srate) {
            audio_chunk * c = insert_chunk((t_size)outFrames * m_nch);
            writeOut(c, m_out.data(), (t_size)outFrames, m_nch, m_srate);
        }
    }
};

// Rejestracja: dsp_factory_t (z presetem/konfiguracja).
static dsp_factory_t<dsp_bookbar> g_dsp_bookbar_factory;

} // anonymous namespace
