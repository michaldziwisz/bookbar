// foobar_glue.cpp — most miedzy SDK foobar2000 a czystym Win32 GUI (gui.cpp).
// Tu zyja typy foobara (dsp_preset, mainmenu_commands); gui.cpp pozostaje czysty
// Win32 i nie ciagnie naglowkow SDK. Zawiera:
//   1) showConfigPopup() — modalne okno konfiguracji + zapis presetu przez callback,
//   2) komendy mainmenu (Tempo/Pitch/Rate +/- , Reset) do podpiecia pod klawisze
//      w Preferences > Keyboard Shortcuts,
//   3) uchwyt modulu (HINSTANCE) wtyczki dla zasobow dialogow.
#include <SDK/foobar2000.h>
#include <windows.h>
#include "params.h"
#include "preset.h"
#include "mapping.h"

namespace bookbar {

// z gui.cpp:
bool runConfigModal(HINSTANCE hInst, HWND parent);
// z gui.cpp (nudge dziala tez gdy okno zamkniete — modyfikuje g_params):
void nudgeTempo(double deltaX);
void nudgeRate(double deltaX);
void nudgePitchCents(int delta);
void resetSlidersToNeutral();
void applyShortcutsSetting();
void removeShortcuts();

// Uchwyt modulu wtyczki (z core_api) — potrzebny do zasobow dialogow (RC).
static HINSTANCE moduleInst() { return core_api::get_my_instance(); }

// --- Okno konfiguracji wolane przez dsp_bookbar::g_show_config_popup ---
void showConfigPopup(const dsp_preset & data, HWND parent, dsp_preset_edit_callback & cb) {
    // Wgraj przekazany preset do g_params, by okno pokazalo AKTUALNY stan tego DSP.
    if (data.get_data_size() == sizeof(PresetBlob)) {
        PresetBlob b; memcpy(&b, data.get_data(), sizeof(b));
        paramsFromBlob(b);
    }
    bool ok = runConfigModal(moduleInst(), parent);
    if (ok) {
        // Zbuduj nowy preset z g_params i zglos hostowi.
        PresetBlob nb; blobFromParams(nb);
        dsp_preset_impl np;
        np.set_owner(data.get_owner());
        np.set_data(&nb, sizeof(nb));
        cb.on_preset_changed(np);
    } else {
        // Anulowano — przywroc oryginal.
        if (data.get_data_size() == sizeof(PresetBlob)) {
            PresetBlob b; memcpy(&b, data.get_data(), sizeof(b));
            paramsFromBlob(b);
        }
        cb.on_preset_changed(data);
    }
}

// ============================================================================
//  Komendy menu glownego (Playback) — user podpina wlasne klawisze w
//  Preferences > Keyboard Shortcuts. Dzialaja niezaleznie od okna konfiguracji.
// ============================================================================

// GUID-y komend (stale).
// {A1B2C3D4-0001-4E5F-8A9B-0C1D2E3F4A01} itd.
static const GUID guid_cmd_tempo_down = { 0xa1b2c3d4, 0x0001, 0x4e5f, {0x8a,0x9b,0x0c,0x1d,0x2e,0x3f,0x4a,0x01} };
static const GUID guid_cmd_tempo_up   = { 0xa1b2c3d4, 0x0002, 0x4e5f, {0x8a,0x9b,0x0c,0x1d,0x2e,0x3f,0x4a,0x02} };
static const GUID guid_cmd_pitch_down = { 0xa1b2c3d4, 0x0003, 0x4e5f, {0x8a,0x9b,0x0c,0x1d,0x2e,0x3f,0x4a,0x03} };
static const GUID guid_cmd_pitch_up   = { 0xa1b2c3d4, 0x0004, 0x4e5f, {0x8a,0x9b,0x0c,0x1d,0x2e,0x3f,0x4a,0x04} };
static const GUID guid_cmd_rate_down  = { 0xa1b2c3d4, 0x0005, 0x4e5f, {0x8a,0x9b,0x0c,0x1d,0x2e,0x3f,0x4a,0x05} };
static const GUID guid_cmd_rate_up    = { 0xa1b2c3d4, 0x0006, 0x4e5f, {0x8a,0x9b,0x0c,0x1d,0x2e,0x3f,0x4a,0x06} };
static const GUID guid_cmd_reset      = { 0xa1b2c3d4, 0x0007, 0x4e5f, {0x8a,0x9b,0x0c,0x1d,0x2e,0x3f,0x4a,0x07} };

class mainmenu_bookbar : public mainmenu_commands {
public:
    enum { CMD_TEMPO_DOWN=0, CMD_TEMPO_UP, CMD_PITCH_DOWN, CMD_PITCH_UP,
           CMD_RATE_DOWN, CMD_RATE_UP, CMD_RESET, CMD_COUNT };

    t_uint32 get_command_count() override { return CMD_COUNT; }

    GUID get_command(t_uint32 i) override {
        switch (i) {
        case CMD_TEMPO_DOWN: return guid_cmd_tempo_down;
        case CMD_TEMPO_UP:   return guid_cmd_tempo_up;
        case CMD_PITCH_DOWN: return guid_cmd_pitch_down;
        case CMD_PITCH_UP:   return guid_cmd_pitch_up;
        case CMD_RATE_DOWN:  return guid_cmd_rate_down;
        case CMD_RATE_UP:    return guid_cmd_rate_up;
        case CMD_RESET:      return guid_cmd_reset;
        default:             return pfc::guid_null;
        }
    }

    void get_name(t_uint32 i, pfc::string_base & out) override {
        switch (i) {
        case CMD_TEMPO_DOWN: out = "Bookbar: Tempo down"; break;
        case CMD_TEMPO_UP:   out = "Bookbar: Tempo up"; break;
        case CMD_PITCH_DOWN: out = "Bookbar: Pitch down (semitone)"; break;
        case CMD_PITCH_UP:   out = "Bookbar: Pitch up (semitone)"; break;
        case CMD_RATE_DOWN:  out = "Bookbar: Rate (tape) down"; break;
        case CMD_RATE_UP:    out = "Bookbar: Rate (tape) up"; break;
        case CMD_RESET:      out = "Bookbar: Reset tempo/pitch/rate"; break;
        default: out = ""; break;
        }
    }

    bool get_description(t_uint32 i, pfc::string_base & out) override {
        out = "Bookbar audiobook DSP control (assign a keyboard shortcut in Preferences).";
        return true;
    }

    GUID get_parent() override {
        // Umiesc w menu glownym "Playback".
        return mainmenu_groups::playback;
    }

    void execute(t_uint32 i, service_ptr_t<service_base>) override {
        switch (i) {
        case CMD_TEMPO_DOWN: nudgeTempo(-TEMPO_STEP); break;
        case CMD_TEMPO_UP:   nudgeTempo(+TEMPO_STEP); break;
        case CMD_PITCH_DOWN: nudgePitchCents(-100); break;
        case CMD_PITCH_UP:   nudgePitchCents(+100); break;
        case CMD_RATE_DOWN:  nudgeRate(-RATE_STEP); break;
        case CMD_RATE_UP:    nudgeRate(+RATE_STEP); break;
        case CMD_RESET:      resetSlidersToNeutral(); break;
        }
    }
};

static mainmenu_commands_factory_t<mainmenu_bookbar> g_mainmenu_bookbar_factory;

// --- initquit: wlacz domyslne skroty przy starcie, sprzataj przy zamknieciu ---
class initquit_bookbar : public initquit {
public:
    void on_init() override { applyShortcutsSetting(); }  // domyslnie wlaczone
    void on_quit() override { removeShortcuts(); }
};
static initquit_factory_t<initquit_bookbar> g_initquit_bookbar_factory;

} // namespace bookbar
