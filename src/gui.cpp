// gui.cpp — natywny GUI Bookamp (Win32, bez frameworkow).
// Okno sterowania: combo algorytmu, checkbox on/off, suwaki tempa/wysokosci,
// etykiety realnych wartosci (Unicode, czytane przez NVDA), reset, parametry zaawansowane.
#include <windows.h>
#include <commctrl.h>
#include <oleacc.h>
#include <cstdio>
#include <cwchar>
#include "resource.h"
#include "params.h"
#include "enhance.h"
#include "mapping.h"
#include "i18n.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "oleacc.lib")

namespace bookbar {

static HWND g_dlg = nullptr;
static bool g_modalMode = false;    // true = okno modalne (foobar config popup)

// Zaimplementowane w shortcuts.cpp — wlacza/wylacza globalny hook wg g_params.shortcuts.
void applyShortcutsSetting();

// --- Dostepnosc suwakow: czytnik ma mowic REALNA wartosc, nie procent 0-100. ---
// Formatuje wartosc suwaka (na podstawie pozycji) do napisu dla NVDA/JAWS.
typedef void (*ValueFmt)(int pos, wchar_t* out, int cch);

static void commaize(wchar_t* s) { for (; *s; ++s) if (*s == L'.') *s = L','; }
static void fmtTempo(int pos, wchar_t* o, int c) { swprintf(o, c, L"%.2fx", posToTempo(pos)); commaize(o); }
static void fmtRate (int pos, wchar_t* o, int c) { swprintf(o, c, L"%.2fx", posToRate(pos)); commaize(o); }
// wysokosc czytelna dla muzyka: poltony + centy (100 centow = 1 polton)
static void fmtPitch(int pos, wchar_t* o, int c) {
    int cents = posToPitchCents(pos);
    if (cents == 0) { swprintf(o, c, T(S_PITCH_ZERO)); return; }
    int semi = cents / 100, rem = cents % 100;
    if (rem == 0) swprintf(o, c, T(S_PITCH_SEMI), semi, cents);
    else          swprintf(o, c, T(S_PITCH_CENTS), cents);
}
static void fmtTaps (int pos, wchar_t* o, int c) { swprintf(o, c, L"%d taps", pos); }
static void fmtMs   (int pos, wchar_t* o, int c) { if (pos == 0) swprintf(o, c, L"auto"); else swprintf(o, c, L"%d ms", pos); }
static void fmtHop  (int pos, wchar_t* o, int c) { swprintf(o, c, L"%d", pos); }

// Nazwy 5 poziomow wzbogacania (0 = off). Wspolne dla obu suwakow — czytelne dla NVDA.
static const wchar_t* enhLevelName(int lvl) {
    switch (lvl) {
        case 0: return T(S_LV_OFF);
        case 1: return T(S_LV_SUBTLE);
        case 2: return T(S_LV_MODERATE);
        case 3: return T(S_LV_MEDIUM);
        case 4: return T(S_LV_STRONG);
        default: return T(S_LV_MAX);
    }
}
static void fmtEnh(int pos, wchar_t* o, int c) { swprintf(o, c, L"%s", enhLevelName(pos)); }

// Wrapper IAccessible: deleguje wszystko do standardowego obiektu suwaka
// z WYJATKIEM get_accValue, ktory zwraca nasza realna wartosc.
class SliderAcc : public IAccessible {
    LONG ref_ = 1;
    IAccessible* inner_;
    HWND hwnd_;
    ValueFmt fmt_;
public:
    SliderAcc(IAccessible* inner, HWND h, ValueFmt f) : inner_(inner), hwnd_(h), fmt_(f) {}
    ~SliderAcc() { if (inner_) inner_->Release(); }

    HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_IDispatch || riid == IID_IAccessible) {
            *ppv = static_cast<IAccessible*>(this); AddRef(); return S_OK;
        }
        return inner_->QueryInterface(riid, ppv);
    }
    ULONG __stdcall AddRef() override { return InterlockedIncrement(&ref_); }
    ULONG __stdcall Release() override { LONG r = InterlockedDecrement(&ref_); if (!r) delete this; return r; }

    HRESULT __stdcall GetTypeInfoCount(UINT* p) override { return inner_->GetTypeInfoCount(p); }
    HRESULT __stdcall GetTypeInfo(UINT i, LCID l, ITypeInfo** t) override { return inner_->GetTypeInfo(i, l, t); }
    HRESULT __stdcall GetIDsOfNames(REFIID r, LPOLESTR* n, UINT c, LCID l, DISPID* d) override { return inner_->GetIDsOfNames(r, n, c, l, d); }
    HRESULT __stdcall Invoke(DISPID d, REFIID r, LCID l, WORD f, DISPPARAMS* p, VARIANT* v, EXCEPINFO* e, UINT* a) override { return inner_->Invoke(d, r, l, f, p, v, e, a); }

    HRESULT __stdcall get_accParent(IDispatch** p) override { return inner_->get_accParent(p); }
    HRESULT __stdcall get_accChildCount(long* c) override { return inner_->get_accChildCount(c); }
    HRESULT __stdcall get_accChild(VARIANT v, IDispatch** d) override { return inner_->get_accChild(v, d); }
    HRESULT __stdcall get_accName(VARIANT v, BSTR* n) override { return inner_->get_accName(v, n); }
    HRESULT __stdcall get_accValue(VARIANT v, BSTR* val) override {
        if (v.vt == VT_I4 && v.lVal != CHILDID_SELF) return inner_->get_accValue(v, val);
        wchar_t buf[96]; fmt_((int)SendMessage(hwnd_, TBM_GETPOS, 0, 0), buf, 96);
        *val = SysAllocString(buf); return S_OK;
    }
    HRESULT __stdcall get_accDescription(VARIANT v, BSTR* d) override { return inner_->get_accDescription(v, d); }
    HRESULT __stdcall get_accRole(VARIANT v, VARIANT* r) override { return inner_->get_accRole(v, r); }
    HRESULT __stdcall get_accState(VARIANT v, VARIANT* s) override { return inner_->get_accState(v, s); }
    HRESULT __stdcall get_accHelp(VARIANT v, BSTR* h) override { return inner_->get_accHelp(v, h); }
    HRESULT __stdcall get_accHelpTopic(BSTR* f, VARIANT v, long* t) override { return inner_->get_accHelpTopic(f, v, t); }
    HRESULT __stdcall get_accKeyboardShortcut(VARIANT v, BSTR* s) override { return inner_->get_accKeyboardShortcut(v, s); }
    HRESULT __stdcall get_accFocus(VARIANT* v) override { return inner_->get_accFocus(v); }
    HRESULT __stdcall get_accSelection(VARIANT* v) override { return inner_->get_accSelection(v); }
    HRESULT __stdcall get_accDefaultAction(VARIANT v, BSTR* a) override { return inner_->get_accDefaultAction(v, a); }
    HRESULT __stdcall accSelect(long f, VARIANT v) override { return inner_->accSelect(f, v); }
    HRESULT __stdcall accLocation(long* l, long* t, long* w, long* h, VARIANT v) override { return inner_->accLocation(l, t, w, h, v); }
    HRESULT __stdcall accNavigate(long d, VARIANT s, VARIANT* e) override { return inner_->accNavigate(d, s, e); }
    HRESULT __stdcall accHitTest(long x, long y, VARIANT* v) override { return inner_->accHitTest(x, y, v); }
    HRESULT __stdcall accDoDefaultAction(VARIANT v) override { return inner_->accDoDefaultAction(v); }
    HRESULT __stdcall put_accName(VARIANT v, BSTR n) override { return inner_->put_accName(v, n); }
    HRESULT __stdcall put_accValue(VARIANT v, BSTR n) override { return inner_->put_accValue(v, n); }
};

static LRESULT CALLBACK sliderSubclass(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR ref) {
    if (m == WM_GETOBJECT && (DWORD)l == (DWORD)OBJID_CLIENT) {
        IAccessible* std = nullptr;
        if (SUCCEEDED(CreateStdAccessibleObject(h, OBJID_CLIENT, IID_PPV_ARGS(&std)))) {
            SliderAcc* wrap = new SliderAcc(std, h, (ValueFmt)ref);
            LRESULT r = LresultFromObject(IID_IAccessible, w, static_cast<IAccessible*>(wrap));
            wrap->Release();
            return r;
        }
    }
    return DefSubclassProc(h, m, w, l);
}

// Podepnij dostepnosc realnej wartosci do suwaka.
static void attachSliderAcc(HWND dlg, int id, ValueFmt fmt) {
    SetWindowSubclass(GetDlgItem(dlg, id), sliderSubclass, (UINT_PTR)id, (DWORD_PTR)fmt);
}
// Powiadom czytnik, ze wartosc suwaka sie zmienila (re-odczyt accValue).
static void announceValue(HWND slider) {
    NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, slider, OBJID_CLIENT, CHILDID_SELF);
}

// Ustaw dostepny tekst kontrolki (Unicode) — NVDA odczyta z ogonkami.
static void setTextW(HWND dlg, int id, const wchar_t* s) {
    SetWindowTextW(GetDlgItem(dlg, id), s);
}

static void updateTempoLabel(HWND dlg, int pos) {
    g_params.tempo.store(posToTempo(pos));
    wchar_t buf[64]; fmtTempo(pos, buf, 64);
    setTextW(dlg, IDC_TEMPO_LABEL, buf);
}

static void updatePitchLabel(HWND dlg, int pos) {
    g_params.pitch.store(pitchCentsToMul(posToPitchCents(pos)));
    wchar_t buf[80]; fmtPitch(pos, buf, 80);   // poltony/centy, spojne z odczytem NVDA
    setTextW(dlg, IDC_PITCH_LABEL, buf);
}

static void updateRateLabel(HWND dlg, int pos) {
    g_params.rate.store(posToRate(pos));
    wchar_t buf[64]; fmtRate(pos, buf, 64);
    setTextW(dlg, IDC_RATE_LABEL, buf);
}

static void updateEnhLoudLabel(HWND dlg, int pos) {
    g_params.enh_loud.store(pos);
    setTextW(dlg, IDC_ENH_LOUD_LBL, enhLevelName(pos));
}
static void updateEnhClarLabel(HWND dlg, int pos) {
    g_params.enh_clarity.store(pos);
    setTextW(dlg, IDC_ENH_CLAR_LBL, enhLevelName(pos));
}

static void initSlider(HWND dlg, int id, int lo, int hi, int line, int page, int startPos) {
    HWND s = GetDlgItem(dlg, id);
    SendMessage(s, TBM_SETRANGE, TRUE, MAKELONG(lo, hi));
    SendMessage(s, TBM_SETLINESIZE, 0, line);
    SendMessage(s, TBM_SETPAGESIZE, 0, page);
    SendMessage(s, TBM_SETPOS, TRUE, startPos);
}

static void fillAlgoCombo(HWND dlg) {
    HWND c = GetDlgItem(dlg, IDC_ALGO);
    SendMessageW(c, CB_ADDSTRING, 0, (LPARAM)T(S_ALGO_ST));
    SendMessageW(c, CB_ADDSTRING, 0, (LPARAM)T(S_ALGO_BG));
    SendMessage(c, CB_SETCURSEL, g_params.algo.load(), 0);
}

// zewnetrzna funkcja resetu suwakow (wywolywana tez skrotem)
void resetSlidersToNeutral() {
    if (g_dlg) {
        SendMessage(GetDlgItem(g_dlg, IDC_TEMPO_SLIDER), TBM_SETPOS, TRUE, tempoNeutralPos());
        SendMessage(GetDlgItem(g_dlg, IDC_PITCH_SLIDER), TBM_SETPOS, TRUE, pitchNeutralPos());
        SendMessage(GetDlgItem(g_dlg, IDC_RATE_SLIDER),  TBM_SETPOS, TRUE, rateNeutralPos());
        updateTempoLabel(g_dlg, tempoNeutralPos());
        updatePitchLabel(g_dlg, pitchNeutralPos());
        updateRateLabel(g_dlg, rateNeutralPos());
        announceValue(GetDlgItem(g_dlg, IDC_TEMPO_SLIDER));
    } else {
        g_params.tempo.store(1.0);
        g_params.pitch.store(1.0);
        g_params.rate.store(1.0);
    }
}

// --- Nudge z klawiatury (skroty). Dzialaja tez gdy okno GUI zamkniete. ---
void nudgeTempo(double deltaX) {
    double t = stepToward(g_params.tempo.load(), deltaX, TEMPO_MID);
    if (t < TEMPO_MIN) t = TEMPO_MIN; if (t > TEMPO_MAX) t = TEMPO_MAX;
    if (g_dlg) {
        HWND s = GetDlgItem(g_dlg, IDC_TEMPO_SLIDER);
        SendMessage(s, TBM_SETPOS, TRUE, tempoToPos(t));
        updateTempoLabel(g_dlg, (int)SendMessage(s, TBM_GETPOS, 0, 0));
        announceValue(s);
    } else {
        g_params.tempo.store(t);
    }
}
void nudgeRate(double deltaX) {
    double r = stepToward(g_params.rate.load(), deltaX, RATE_MID);
    if (r < RATE_MIN) r = RATE_MIN; if (r > RATE_MAX) r = RATE_MAX;
    if (g_dlg) {
        HWND s = GetDlgItem(g_dlg, IDC_RATE_SLIDER);
        SendMessage(s, TBM_SETPOS, TRUE, rateToPos(r));
        updateRateLabel(g_dlg, (int)SendMessage(s, TBM_GETPOS, 0, 0));
        announceValue(s);
    } else {
        g_params.rate.store(r);
    }
}
void nudgePitchCents(int deltaCents) {
    if (g_dlg) {
        HWND s = GetDlgItem(g_dlg, IDC_PITCH_SLIDER);
        int cents = posToPitchCents((int)SendMessage(s, TBM_GETPOS, 0, 0)) + deltaCents;
        int pos = pitchCentsToPos(cents);
        SendMessage(s, TBM_SETPOS, TRUE, pos);
        updatePitchLabel(g_dlg, pos);
        announceValue(s);
    } else {
        int cents = mulToPitchCents(g_params.pitch.load()) + deltaCents;
        if (cents < -PITCH_CENTS) cents = -PITCH_CENTS; if (cents > PITCH_CENTS) cents = PITCH_CENTS;
        g_params.pitch.store(pitchCentsToMul(cents));
    }
}

// --- Okno zaawansowane: suwaki z live-labelami (dostepne dla NVDA), sekcja wg algorytmu ---

// aktualizacja jednej etykiety wartosci: label pokazuje realna liczbe + jednostke,
// a takze nazwa dostepna (accName) samego suwaka jest ustawiana na "opis: wartosc".
static void advSetLabel(HWND h, int lblId, const wchar_t* text) {
    SetWindowTextW(GetDlgItem(h, lblId), text);
}

static void advRefreshAll(HWND h) {
    wchar_t b[96];
    fmtTaps((int)SendMessage(GetDlgItem(h, IDC_ADV_AALEN), TBM_GETPOS, 0, 0), b, 96);     advSetLabel(h, IDC_ADV_AALEN_LBL, b);
    fmtMs  ((int)SendMessage(GetDlgItem(h, IDC_ADV_SEQUENCE), TBM_GETPOS, 0, 0), b, 96);  advSetLabel(h, IDC_ADV_SEQ_LABEL, b);
    fmtMs  ((int)SendMessage(GetDlgItem(h, IDC_ADV_SEEKWIN), TBM_GETPOS, 0, 0), b, 96);   advSetLabel(h, IDC_ADV_SEEK_LBL, b);
    fmtMs  ((int)SendMessage(GetDlgItem(h, IDC_ADV_OVERLAP), TBM_GETPOS, 0, 0), b, 96);   advSetLabel(h, IDC_ADV_OVL_LBL, b);
    fmtHop ((int)SendMessage(GetDlgItem(h, IDC_ADV_HOP), TBM_GETPOS, 0, 0), b, 96);       advSetLabel(h, IDC_ADV_HOP_LBL, b);
}

// pokaz/ukryj sekcje wg algorytmu (pkt 2: automatycznie dla wybranego algorytmu)
static void advShowSection(HWND h, int algo) {
    int stIds[] = { IDC_ADV_QUICKSEEK, IDC_ADV_AAFILTER, IDC_ADV_AALEN_CAP, IDC_ADV_AALEN, IDC_ADV_AALEN_LBL,
                    IDC_ADV_SEQ_CAP, IDC_ADV_SEQUENCE, IDC_ADV_SEQ_LABEL, IDC_ADV_SEEK_CAP, IDC_ADV_SEEKWIN,
                    IDC_ADV_SEEK_LBL, IDC_ADV_OVL_CAP, IDC_ADV_OVERLAP, IDC_ADV_OVL_LBL };
    int bgIds[] = { IDC_ADV_HOP_CAP, IDC_ADV_HOP, IDC_ADV_HOP_LBL };
    int stShow = (algo == ALGO_SOUNDTOUCH) ? SW_SHOW : SW_HIDE;
    int bgShow = (algo == ALGO_BUNGEE) ? SW_SHOW : SW_HIDE;
    for (int id : stIds) ShowWindow(GetDlgItem(h, id), stShow);
    for (int id : bgIds) ShowWindow(GetDlgItem(h, id), bgShow);
    setTextW(h, IDC_ADV_INFO, algo == ALGO_SOUNDTOUCH ? T(S_ADV_INFO_ST) : T(S_ADV_INFO_BG));
}

static void advLoadFromParams(HWND h) {
    // etykiety wg jezyka (w .rc goly ASCII dla pewnosci kodowania)
    SetWindowTextW(h, T(S_ADV_CAPTION));
    setTextW(h, IDC_ADV_QUICKSEEK, T(S_QUICKSEEK));
    setTextW(h, IDC_ADV_AAFILTER,  T(S_AAFILTER));
    setTextW(h, IDC_ADV_AALEN_CAP, T(S_AALEN_CAP));
    setTextW(h, IDC_ADV_SEQ_CAP,   T(S_SEQ_CAP));
    setTextW(h, IDC_ADV_SEEK_CAP,  T(S_SEEK_CAP));
    setTextW(h, IDC_ADV_OVL_CAP,   T(S_OVL_CAP));
    setTextW(h, IDC_ADV_HOP_CAP,   T(S_HOP_CAP));
    setTextW(h, IDC_ADV_DEFAULTS,  T(S_DEFAULTS_BTN));
    CheckDlgButton(h, IDC_ADV_QUICKSEEK, g_params.st_quickseek.load() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(h, IDC_ADV_AAFILTER, g_params.st_aafilter.load() ? BST_CHECKED : BST_UNCHECKED);
    initSlider(h, IDC_ADV_AALEN,   8, 128, 2, 16, g_params.st_aa_len.load());
    initSlider(h, IDC_ADV_SEQUENCE, 0, 200, 1, 10, g_params.st_sequence_ms.load());
    initSlider(h, IDC_ADV_SEEKWIN,  0, 100, 1, 10, g_params.st_seekwindow_ms.load());
    initSlider(h, IDC_ADV_OVERLAP,  0, 50,  1, 5,  g_params.st_overlap_ms.load());
    initSlider(h, IDC_ADV_HOP,     -2, 2,   1, 1,  g_params.bg_hop_adjust.load());
    // dostepnosc: czytnik mowi realna wartosc kazdego suwaka
    attachSliderAcc(h, IDC_ADV_AALEN, fmtTaps);
    attachSliderAcc(h, IDC_ADV_SEQUENCE, fmtMs);
    attachSliderAcc(h, IDC_ADV_SEEKWIN, fmtMs);
    attachSliderAcc(h, IDC_ADV_OVERLAP, fmtMs);
    attachSliderAcc(h, IDC_ADV_HOP, fmtHop);
    advRefreshAll(h);
}

static void advApplyToParams(HWND h) {
    g_params.st_quickseek.store(IsDlgButtonChecked(h, IDC_ADV_QUICKSEEK) == BST_CHECKED);
    g_params.st_aafilter.store(IsDlgButtonChecked(h, IDC_ADV_AAFILTER) == BST_CHECKED);
    g_params.st_aa_len.store((int)SendMessage(GetDlgItem(h, IDC_ADV_AALEN), TBM_GETPOS, 0, 0));
    g_params.st_sequence_ms.store((int)SendMessage(GetDlgItem(h, IDC_ADV_SEQUENCE), TBM_GETPOS, 0, 0));
    g_params.st_seekwindow_ms.store((int)SendMessage(GetDlgItem(h, IDC_ADV_SEEKWIN), TBM_GETPOS, 0, 0));
    g_params.st_overlap_ms.store((int)SendMessage(GetDlgItem(h, IDC_ADV_OVERLAP), TBM_GETPOS, 0, 0));
    g_params.bg_hop_adjust.store((int)SendMessage(GetDlgItem(h, IDC_ADV_HOP), TBM_GETPOS, 0, 0));
    g_params.bumpGen();   // Processor przekonfiguruje silnik
}

INT_PTR CALLBACK AdvancedProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITDIALOG:
        advLoadFromParams(h);
        advShowSection(h, g_params.algo.load());
        return TRUE;
    case WM_HSCROLL:
        advRefreshAll(h);
        announceValue((HWND)lp);   // czytnik re-odczyta realna wartosc
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDOK:
            advApplyToParams(h);
            EndDialog(h, IDOK);
            return TRUE;
        case IDCANCEL:
            EndDialog(h, IDCANCEL);
            return TRUE;
        case IDC_ADV_DEFAULTS:
            initSlider(h, IDC_ADV_AALEN,   8, 128, 2, 16, 32);
            initSlider(h, IDC_ADV_SEQUENCE, 0, 200, 1, 10, 0);
            initSlider(h, IDC_ADV_SEEKWIN,  0, 100, 1, 10, 0);
            initSlider(h, IDC_ADV_OVERLAP,  0, 50,  1, 5,  0);
            initSlider(h, IDC_ADV_HOP,     -2, 2,   1, 1,  0);
            CheckDlgButton(h, IDC_ADV_QUICKSEEK, BST_UNCHECKED);
            CheckDlgButton(h, IDC_ADV_AAFILTER, BST_CHECKED);
            advRefreshAll(h);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

INT_PTR CALLBACK MainProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITDIALOG: {
        g_dlg = h;
        SetWindowTextW(h, T(S_MAIN_CAPTION));   // tytul okna (dwujezyczny, z ogonkami)
        fillAlgoCombo(h);
        CheckDlgButton(h, IDC_ENABLED, g_params.enabled.load() ? BST_CHECKED : BST_UNCHECKED);
        setTextW(h, IDC_ENABLED, T(S_ENABLED));
        CheckDlgButton(h, IDC_SHORTCUTS, g_params.shortcuts.load() ? BST_CHECKED : BST_UNCHECKED);
        setTextW(h, IDC_SHORTCUTS, T(S_SHORTCUTS));
        setTextW(h, IDC_ALGO_CAPTION,  T(S_ALGO_CAP));
        setTextW(h, IDC_RESET,         T(S_RESET_BTN));
        setTextW(h, IDC_ADVANCED_BTN,  T(S_ADVANCED_BTN));
        // start od WCZYTANYCH wartosci (persystencja), nie zawsze neutral
        {
            int tPos = tempoToPos(g_params.tempo.load());
            int rPos = rateToPos(g_params.rate.load());
            int pPos = pitchCentsToPos(mulToPitchCents(g_params.pitch.load()));
            initSlider(h, IDC_TEMPO_SLIDER, 0, SLIDER_MAX, SLIDER_LINE, SLIDER_PAGE, tPos);
            initSlider(h, IDC_PITCH_SLIDER, 0, PITCH_SLIDER_MAX, PITCH_LINE, PITCH_PAGE, pPos);
            initSlider(h, IDC_RATE_SLIDER,  0, SLIDER_MAX, SLIDER_LINE, SLIDER_PAGE, rPos);
            updateTempoLabel(h, tPos);
            updatePitchLabel(h, pPos);
            updateRateLabel(h, rPos);
        }
        // suwaki wzbogacania: 0..5, strzalka=1, PageUp/Down=1, start z zapisanych wartosci
        {
            int lPos = g_params.enh_loud.load();
            int cPos = g_params.enh_clarity.load();
            initSlider(h, IDC_ENH_LOUD, 0, ENH_LEVELS, 1, 1, lPos);
            initSlider(h, IDC_ENH_CLAR, 0, ENH_LEVELS, 1, 1, cPos);
            updateEnhLoudLabel(h, lPos);
            updateEnhClarLabel(h, cPos);
            attachSliderAcc(h, IDC_ENH_LOUD, fmtEnh);
            attachSliderAcc(h, IDC_ENH_CLAR, fmtEnh);
            setTextW(h, IDC_ENH_LOUD_CAP, T(S_ENH_LOUD_CAP));
            setTextW(h, IDC_ENH_CLAR_CAP, T(S_ENH_CLAR_CAP));
        }
        // dostepne nazwy dla suwakow (NVDA)
        setTextW(h, IDC_TEMPO_CAPTION, T(S_TEMPO_CAP));
        setTextW(h, IDC_PITCH_CAPTION, T(S_PITCH_CAP));
        setTextW(h, IDC_RATE_CAPTION,  T(S_RATE_CAP));
        // czytnik ma mowic REALNA wartosc (nie procent) — wrapper accValue
        attachSliderAcc(h, IDC_TEMPO_SLIDER, fmtTempo);
        attachSliderAcc(h, IDC_PITCH_SLIDER, fmtPitch);
        attachSliderAcc(h, IDC_RATE_SLIDER, fmtRate);
        return TRUE;
    }
    case WM_HSCROLL: {
        HWND ctl = (HWND)lp;
        // Suwak rusza natywnie o 1 pozycje (strzalka) / 5% (page). Prosto i zawsze
        // dziala — bez "sprytnego" przeliczania, ktore gubilo krok przy krzywej.
        int pos = (int)SendMessage(ctl, TBM_GETPOS, 0, 0);
        if (ctl == GetDlgItem(h, IDC_TEMPO_SLIDER))      updateTempoLabel(h, pos);
        else if (ctl == GetDlgItem(h, IDC_RATE_SLIDER))  updateRateLabel(h, pos);
        else if (ctl == GetDlgItem(h, IDC_PITCH_SLIDER)) updatePitchLabel(h, pos);
        else if (ctl == GetDlgItem(h, IDC_ENH_LOUD))     updateEnhLoudLabel(h, pos);
        else if (ctl == GetDlgItem(h, IDC_ENH_CLAR))     updateEnhClarLabel(h, pos);
        announceValue(ctl);   // czytnik re-odczyta realna wartosc
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_ALGO:
            if (HIWORD(wp) == CBN_SELCHANGE)
                g_params.algo.store((int)SendMessage((HWND)lp, CB_GETCURSEL, 0, 0));
            return TRUE;
        case IDC_ENABLED:
            g_params.enabled.store(IsDlgButtonChecked(h, IDC_ENABLED) == BST_CHECKED);
            return TRUE;
        case IDC_SHORTCUTS:
            g_params.shortcuts.store(IsDlgButtonChecked(h, IDC_SHORTCUTS) == BST_CHECKED);
            applyShortcutsSetting();   // wlacz/wylacz hook natychmiast
            return TRUE;
        case IDC_RESET:
            resetSlidersToNeutral();
            return TRUE;
        case IDC_ADVANCED_BTN:
            DialogBoxParamW((HINSTANCE)GetWindowLongPtr(h, GWLP_HINSTANCE),
                            MAKEINTRESOURCEW(IDD_ADVANCED), h, AdvancedProc, 0);
            return TRUE;
        case IDOK:
            if (g_modalMode) { EndDialog(h, IDOK); return TRUE; }
            break;
        case IDCANCEL:
            if (g_modalMode) { EndDialog(h, IDCANCEL); return TRUE; }
            break;
        }
        break;
    case WM_CLOSE:
        if (g_modalMode) { EndDialog(h, IDCANCEL); return TRUE; }
        DestroyWindow(h);
        return TRUE;
    case WM_DESTROY:
        g_dlg = nullptr;
        if (!g_modalMode) PostQuitMessage(0);   // konczy petle komunikatow watku GUI (Winamp-mode)
        return TRUE;
    }
    return FALSE;
}

// --- foobar2000: okno konfiguracji jest MODALNE (glowny watek hosta). ---
// Uruchamia dialog IDD_MAIN modalnie. Zmiany parametrow trafiaja na biezaco do
// globalnych g_params (jak w Winampie). Zwraca true gdy user zatwierdzil (OK),
// false gdy anulowal. hInst = modul wtyczki (z resource.h dialogami).
bool runConfigModal(HINSTANCE hInst, HWND parent) {
    INITCOMMONCONTROLSEX ic{ sizeof(ic), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&ic);
    g_modalMode = true;
    INT_PTR r = DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_MAIN), parent, MainProc, 0);
    g_modalMode = false;
    g_dlg = nullptr;
    return (r == IDOK);
}

} // namespace bookbar
