// enhance.cpp — modul wzbogacania dzwieku (loudness limiter + clarity phase rotator).
// Zobacz enhance.h. Sygnal float interleaved [-1,1]. Wolany PO time-stretchu.
//
// LOUDNESS: BIT-EXACT port automatu LoudMax (Maximizer::process z la_LoudMax64.so).
//   Zrodlo: re_loudmax/AUTOMAT_BITEXACT.md (goto-machine, zweryfikowane -148 dB vs
//   wyrocznia .so w re_loudmax/sim_full.py). Detektor ISP 12-tap x4 fazy, delay 61,
//   obwiednia (peak-hold okna + bank + attack/release), gain=1/gEnv*outGain (mantysa &~1).
//   Prog wchodzi WYLACZNIE przez stan reset (pola *thrLin); outGain stale=31.90625763.
#include "enhance.h"
#include "isp_fir.h"
#include <cmath>
#include <cstring>
#include <cstdint>
#include <algorithm>

namespace bookbar {

// ============================================================================
//  CLARITY — bateria filtrow allpass 2. rzedu (phase rotator, wzor Orban/Omnia)
// ============================================================================
namespace {
struct ApSpec { float f0; float bw; int count; };

const std::vector<ApSpec>& claritySpec(int level) {
    static const std::vector<ApSpec> L1 = { {700.f, 2.0f, 2} };
    static const std::vector<ApSpec> L2 = { {700.f, 2.0f, 4} };
    static const std::vector<ApSpec> L3 = { {700.f, 2.0f, 4}, {300.f, 1.2f, 2} };
    static const std::vector<ApSpec> L4 = { {700.f, 2.0f, 6}, {300.f, 1.2f, 3} };
    static const std::vector<ApSpec> L5 = { {600.f, 2.0f, 8}, {400.f, 1.7f, 4} };
    static const std::vector<ApSpec> OFF;
    switch (level) {
        case 1: return L1; case 2: return L2; case 3: return L3;
        case 4: return L4; case 5: return L5; default: return OFF;
    }
}
} // anon

void Enhancer::buildClarity() {
    ap_.clear();
    if (clar_ <= 0 || srate_ <= 0) return;
    const double fs = (double)srate_;
    for (const ApSpec& s : claritySpec(clar_)) {
        double w0 = 2.0 * M_PI * s.f0 / fs;
        double alpha = std::sin(w0) * std::sinh(std::log(2.0) / 2.0 * s.bw * w0 / std::sin(w0));
        double a0 = 1.0 + alpha;
        Biquad bq;
        bq.b0 = (float)((1.0 - alpha) / a0);
        bq.b1 = (float)((-2.0 * std::cos(w0)) / a0);
        bq.b2 = 1.0f;
        bq.a1 = (float)((-2.0 * std::cos(w0)) / a0);
        bq.a2 = (float)((1.0 - alpha) / a0);
        for (int i = 0; i < s.count; ++i) { bq.clear(); ap_.push_back(bq); }
    }
}

// ============================================================================
//  LOUDNESS — BIT-EXACT automat LoudMax (Maximizer::process, la_LoudMax64.so)
// ============================================================================
// Ponizej rdzen 1:1 z AUTOMAT_BITEXACT.md. Cala logika w float(32)/double(64)
// DOKLADNIE jak w oryginalnym kodzie maszynowym.
namespace {

static inline float f32(double x){ return (float)x; }
static inline float f32(float  x){ return x; }
static inline float hexf(uint32_t u){ float f; std::memcpy(&f,&u,4); return f; }

// ---- ISP FIR: 12 tapow x 4 fazy (z rodata @0xaaa0) ----
static const float LM_FIR[12][4] = {
  {-0.022066593170166016f,-0.03212738037109375f,-0.0291595458984375f,-0.016592025756835938f},
  { 0.04205727577209473f, 0.060472726821899414f, 0.0530773401260376f, 0.02850043773651123f},
  {-0.07229435443878174f,-0.10719501972198486f,-0.09681558609008789f,-0.05324435234069824f},
  { 0.109519362449646f,   0.16516852378845215f, 0.15256524085998535f, 0.08616626262664795f},
  {-0.17800593376159668f,-0.25662314891815186f,-0.23108232021331787f,-0.12913477420806885f},
  { 0.9532511234283447f,  0.7870337963104248f,  0.534685492515564f,   0.2518436908721924f},
  { 0.2518436908721924f,  0.534685492515564f,   0.7870337963104248f,  0.9532511234283447f},
  {-0.12913477420806885f,-0.23108232021331787f,-0.25662314891815186f,-0.17800593376159668f},
  { 0.08616626262664795f, 0.15256524085998535f, 0.16516852378845215f, 0.109519362449646f},
  {-0.05324435234069824f,-0.09681558609008789f,-0.10719501972198486f,-0.07229435443878174f},
  { 0.02850043773651123f, 0.0530773401260376f,  0.060472726821899414f,0.04205727577209473f},
  {-0.016592025756835938f,-0.0291595458984375f,-0.03212738037109375f,-0.022066593170166016f},
};
static const float MUL32 = 31.999996185302734f;
static const float BIG   = 9.999999933815813e+36f;

static const double d70=0.018091201782226562, d78=0.9895446300506592;
static const double d80=0.9992637634277344,    d88=0.9999263286590576;
static const double DABB0=0.7483314773547883;
static const double DABA0=1.0000001676380634,  DABA8=1.4142135623730951;
static const double DABC0=0.9959786,            DABC8=2.7777777777777777;
static const double DABD0=0.996826171875,       DABD8=0.9785720620877001;
static const double DABE0=0.9170040432046712,   DABE8=0.8408964152537145;
static const double DABF0=0.7071067811865476,   DABF8=0.9576032806985737;
static const double DAC00=0.35999999940395355;
static const double DBANK08=0.4556253375000625;
static const double DABB8=1.0000001;
static const float  F_476e7=4.76837158203125e-07f, F_994987=0.994987428188324f;
static const float  F_010232=0.010232930071651936f, F_015625=0.015625f;
static const float  F_100000501=1.0000050067901611f, F_100097=1.0009765625f;
static const float  F_025=0.25f, F_999913=0.9999133348464966f, F_036=0.36000001430511475f;
static const float  F_999566=0.9995668530464172f, F_868497=0.8684974908828735f;
static const float  F_919945=0.9199450016021729f, F_953070=0.9530699849128723f;
static const float  F_0404=0.040400099009275436f, NEG1=-1.0f, F_1=1.0f;

// ---------------------------------------------------------------------------
//  LMCore — 1:1 automat. Stan = pola [rdi+off]. Metody detect()+envelope().
// ---------------------------------------------------------------------------
struct LMCore {
  float A[64], B[64], DL[64];
  float ispHist[12]; int ispPos;
  float ispHistR[12];   // drugi bufor ISP dla prawego kanalu (process2 @0x1a8)
  float o00,o04,o08,o0c,o10,o14,o1c;
  float o30,o34,o38,o3c,o40,o44,o48,o4c,o50,o54,o58,o68;
  float o98,o9c,oa0,oa4,oa8,oac,ob0,ob4;
  double d90;
  double dATK, dREL, dSLOW1, dSLOW2;
  uint16_t c20,c22,c24,c26,c28,c2a,c2c,cc0,cc2,cc4;
  uint8_t  c2e,c2f,b8,b9,ba,c8,c9,ca;
  int32_t  bc;
  int      sel;
  int32_t  HOLD;

  void reset(float threshInit, float outGainInit, float prevPeakInit,
             float aEnvInit, float o10Init, uint16_t bp, uint16_t b22) {
    std::memset(A,0,sizeof(A)); std::memset(B,0,sizeof(B)); std::memset(DL,0,sizeof(DL));
    std::memset(ispHist,0,sizeof(ispHist)); std::memset(ispHistR,0,sizeof(ispHistR)); ispPos=0;
    o00=o04=threshInit; o08=o0c=prevPeakInit; o10=o10Init; o14=-1.0f;
    o1c=outGainInit;
    o30=o34=o38=o3c=o40=o54=o68=ob0=ob4=threshInit;
    o44=o48=1.0f; o4c=32.0f; o50=32.0f; o58=1.0f;
    o98=-1.0f; o9c=-1.0f; oa0=-1.0f;
    oa4=oa8=aEnvInit; oac=32.0f;
    d90=1.52587890625e-05;
    dATK=d70; dREL=d78; dSLOW1=d80; dSLOW2=d88;
    c20=bp; c22=b22; c24=0; c26=90; c28=0; c2a=540; c2c=0;
    cc0=0; cc2=0; cc4=0;
    c2e=200; c2f=0; b8=0; b9=0; ba=0; c8=0; c9=0; ca=0;
    bc=0; sel=0;
    HOLD=(int32_t)(((int)bp*5)<<5);
  }
  inline float selVal() const {
    if(sel==0) return o30; if(sel==1) return o54; return oac;
  }

  // ---------- detektor ISP (BIT-EXACT dla wszystkich wpos 0..11). .so ma rozwiniete
  //  SSE z RÓZNYM drzewem sumowania FIR per-pozycja bufora pierscieniowego (punkt
  //  zawijania t==wpos zmienia grupowanie dodawan f32). Odtworzone 1:1 z ASM
  //  process_dis_full.txt @0x36f7-0x4939 (glowna + 10 galezi zawijania). ----------
  float detect(float xi){
    ispHist[ispPos]=xi;
    const int wpos=ispPos;
    const float b0=ispHist[0],b1=ispHist[1],b2=ispHist[2],b3=ispHist[3];
    const float b4=ispHist[4],b5=ispHist[5],b6=ispHist[6],b7=ispHist[7];
    const float b8=ispHist[8],b9=ispHist[9],b10=ispHist[10],b11=ispHist[11];
    auto A=[](float a,float b){ return f32(a+b); };
    auto M=[](float a,float b){ return f32(a*b); };
    float a0=0,a1=0,a2=0,a3=0;
        switch(wpos){
        case 0:
          a0=A(A(M(b2,LM_FIR[10][0]),A(A(M(b4,LM_FIR[8][0]),A(M(b5,LM_FIR[7][0]),A(M(b6,LM_FIR[6][0]),A(A(A(M(b9,LM_FIR[3][0]),A(M(b10,LM_FIR[2][0]),A(M(b0,LM_FIR[0][0]),M(b11,LM_FIR[1][0])))),M(b8,LM_FIR[4][0])),M(b7,LM_FIR[5][0]))))),M(b3,LM_FIR[9][0]))),M(b1,LM_FIR[11][0]));
          a1=A(A(M(b2,LM_FIR[10][1]),A(A(M(b4,LM_FIR[8][1]),A(M(b5,LM_FIR[7][1]),A(M(b6,LM_FIR[6][1]),A(A(A(M(b9,LM_FIR[3][1]),A(M(b10,LM_FIR[2][1]),A(M(b0,LM_FIR[0][1]),M(b11,LM_FIR[1][1])))),M(b8,LM_FIR[4][1])),M(b7,LM_FIR[5][1]))))),M(b3,LM_FIR[9][1]))),M(b1,LM_FIR[11][1]));
          a2=A(A(M(b2,LM_FIR[10][2]),A(A(M(b4,LM_FIR[8][2]),A(M(b5,LM_FIR[7][2]),A(M(b6,LM_FIR[6][2]),A(A(A(M(b9,LM_FIR[3][2]),A(M(b10,LM_FIR[2][2]),A(M(b0,LM_FIR[0][2]),M(b11,LM_FIR[1][2])))),M(b8,LM_FIR[4][2])),M(b7,LM_FIR[5][2]))))),M(b3,LM_FIR[9][2]))),M(b1,LM_FIR[11][2]));
          a3=A(A(M(b2,LM_FIR[10][3]),A(A(M(b4,LM_FIR[8][3]),A(M(b5,LM_FIR[7][3]),A(M(b6,LM_FIR[6][3]),A(A(A(M(b9,LM_FIR[3][3]),A(M(b10,LM_FIR[2][3]),A(M(b0,LM_FIR[0][3]),M(b11,LM_FIR[1][3])))),M(b8,LM_FIR[4][3])),M(b7,LM_FIR[5][3]))))),M(b3,LM_FIR[9][3]))),M(b1,LM_FIR[11][3]));
          break;
        case 1:
          a0=A(A(M(b3,LM_FIR[10][0]),A(A(M(b5,LM_FIR[8][0]),A(M(b6,LM_FIR[7][0]),A(M(b7,LM_FIR[6][0]),A(A(A(A(M(b1,LM_FIR[0][0]),M(b0,LM_FIR[1][0])),A(M(b11,LM_FIR[2][0]),M(b10,LM_FIR[3][0]))),M(b9,LM_FIR[4][0])),M(b8,LM_FIR[5][0]))))),M(b4,LM_FIR[9][0]))),M(b2,LM_FIR[11][0]));
          a1=A(A(M(b3,LM_FIR[10][1]),A(A(M(b5,LM_FIR[8][1]),A(M(b6,LM_FIR[7][1]),A(M(b7,LM_FIR[6][1]),A(A(A(A(M(b1,LM_FIR[0][1]),M(b0,LM_FIR[1][1])),A(M(b11,LM_FIR[2][1]),M(b10,LM_FIR[3][1]))),M(b9,LM_FIR[4][1])),M(b8,LM_FIR[5][1]))))),M(b4,LM_FIR[9][1]))),M(b2,LM_FIR[11][1]));
          a2=A(A(M(b3,LM_FIR[10][2]),A(A(M(b5,LM_FIR[8][2]),A(M(b6,LM_FIR[7][2]),A(M(b7,LM_FIR[6][2]),A(A(A(A(M(b1,LM_FIR[0][2]),M(b0,LM_FIR[1][2])),A(M(b11,LM_FIR[2][2]),M(b10,LM_FIR[3][2]))),M(b9,LM_FIR[4][2])),M(b8,LM_FIR[5][2]))))),M(b4,LM_FIR[9][2]))),M(b2,LM_FIR[11][2]));
          a3=A(A(M(b3,LM_FIR[10][3]),A(A(M(b5,LM_FIR[8][3]),A(M(b6,LM_FIR[7][3]),A(M(b7,LM_FIR[6][3]),A(A(A(A(M(b1,LM_FIR[0][3]),M(b0,LM_FIR[1][3])),A(M(b11,LM_FIR[2][3]),M(b10,LM_FIR[3][3]))),M(b9,LM_FIR[4][3])),M(b8,LM_FIR[5][3]))))),M(b4,LM_FIR[9][3]))),M(b2,LM_FIR[11][3]));
          break;
        case 2:
          a0=A(A(M(b4,LM_FIR[10][0]),A(A(M(b6,LM_FIR[8][0]),A(M(b7,LM_FIR[7][0]),A(M(b8,LM_FIR[6][0]),A(A(A(M(b10,LM_FIR[4][0]),M(b11,LM_FIR[3][0])),A(M(b0,LM_FIR[2][0]),A(M(b2,LM_FIR[0][0]),M(b1,LM_FIR[1][0])))),M(b9,LM_FIR[5][0]))))),M(b5,LM_FIR[9][0]))),M(b3,LM_FIR[11][0]));
          a1=A(A(M(b4,LM_FIR[10][1]),A(A(M(b6,LM_FIR[8][1]),A(M(b7,LM_FIR[7][1]),A(M(b8,LM_FIR[6][1]),A(A(A(M(b10,LM_FIR[4][1]),M(b11,LM_FIR[3][1])),A(M(b0,LM_FIR[2][1]),A(M(b2,LM_FIR[0][1]),M(b1,LM_FIR[1][1])))),M(b9,LM_FIR[5][1]))))),M(b5,LM_FIR[9][1]))),M(b3,LM_FIR[11][1]));
          a2=A(A(M(b4,LM_FIR[10][2]),A(A(M(b6,LM_FIR[8][2]),A(M(b7,LM_FIR[7][2]),A(M(b8,LM_FIR[6][2]),A(A(A(M(b10,LM_FIR[4][2]),M(b11,LM_FIR[3][2])),A(M(b0,LM_FIR[2][2]),A(M(b2,LM_FIR[0][2]),M(b1,LM_FIR[1][2])))),M(b9,LM_FIR[5][2]))))),M(b5,LM_FIR[9][2]))),M(b3,LM_FIR[11][2]));
          a3=A(A(M(b4,LM_FIR[10][3]),A(A(M(b6,LM_FIR[8][3]),A(M(b7,LM_FIR[7][3]),A(M(b8,LM_FIR[6][3]),A(A(A(M(b10,LM_FIR[4][3]),M(b11,LM_FIR[3][3])),A(M(b0,LM_FIR[2][3]),A(M(b2,LM_FIR[0][3]),M(b1,LM_FIR[1][3])))),M(b9,LM_FIR[5][3]))))),M(b5,LM_FIR[9][3]))),M(b3,LM_FIR[11][3]));
          break;
        case 3:
          a0=A(A(M(b5,LM_FIR[10][0]),A(A(M(b7,LM_FIR[8][0]),A(M(b8,LM_FIR[7][0]),A(M(b9,LM_FIR[6][0]),A(A(M(b0,LM_FIR[3][0]),A(M(b1,LM_FIR[2][0]),A(M(b3,LM_FIR[0][0]),M(b2,LM_FIR[1][0])))),A(M(b11,LM_FIR[4][0]),M(b10,LM_FIR[5][0])))))),M(b6,LM_FIR[9][0]))),M(b4,LM_FIR[11][0]));
          a1=A(A(M(b5,LM_FIR[10][1]),A(A(M(b7,LM_FIR[8][1]),A(M(b8,LM_FIR[7][1]),A(M(b9,LM_FIR[6][1]),A(A(M(b0,LM_FIR[3][1]),A(M(b1,LM_FIR[2][1]),A(M(b3,LM_FIR[0][1]),M(b2,LM_FIR[1][1])))),A(M(b11,LM_FIR[4][1]),M(b10,LM_FIR[5][1])))))),M(b6,LM_FIR[9][1]))),M(b4,LM_FIR[11][1]));
          a2=A(A(M(b5,LM_FIR[10][2]),A(A(M(b7,LM_FIR[8][2]),A(M(b8,LM_FIR[7][2]),A(M(b9,LM_FIR[6][2]),A(A(M(b0,LM_FIR[3][2]),A(M(b1,LM_FIR[2][2]),A(M(b3,LM_FIR[0][2]),M(b2,LM_FIR[1][2])))),A(M(b11,LM_FIR[4][2]),M(b10,LM_FIR[5][2])))))),M(b6,LM_FIR[9][2]))),M(b4,LM_FIR[11][2]));
          a3=A(A(M(b5,LM_FIR[10][3]),A(A(M(b7,LM_FIR[8][3]),A(M(b8,LM_FIR[7][3]),A(M(b9,LM_FIR[6][3]),A(A(M(b0,LM_FIR[3][3]),A(M(b1,LM_FIR[2][3]),A(M(b3,LM_FIR[0][3]),M(b2,LM_FIR[1][3])))),A(M(b11,LM_FIR[4][3]),M(b10,LM_FIR[5][3])))))),M(b6,LM_FIR[9][3]))),M(b4,LM_FIR[11][3]));
          break;
        case 4:
          a0=A(A(M(b6,LM_FIR[10][0]),A(A(M(b8,LM_FIR[8][0]),A(M(b9,LM_FIR[7][0]),A(A(A(M(b1,LM_FIR[3][0]),A(M(b2,LM_FIR[2][0]),A(M(b4,LM_FIR[0][0]),M(b3,LM_FIR[1][0])))),M(b0,LM_FIR[4][0])),A(M(b11,LM_FIR[5][0]),M(b10,LM_FIR[6][0]))))),M(b7,LM_FIR[9][0]))),M(b5,LM_FIR[11][0]));
          a1=A(A(M(b6,LM_FIR[10][1]),A(A(M(b8,LM_FIR[8][1]),A(M(b9,LM_FIR[7][1]),A(A(A(M(b1,LM_FIR[3][1]),A(M(b2,LM_FIR[2][1]),A(M(b4,LM_FIR[0][1]),M(b3,LM_FIR[1][1])))),M(b0,LM_FIR[4][1])),A(M(b11,LM_FIR[5][1]),M(b10,LM_FIR[6][1]))))),M(b7,LM_FIR[9][1]))),M(b5,LM_FIR[11][1]));
          a2=A(A(M(b6,LM_FIR[10][2]),A(A(M(b8,LM_FIR[8][2]),A(M(b9,LM_FIR[7][2]),A(A(A(M(b1,LM_FIR[3][2]),A(M(b2,LM_FIR[2][2]),A(M(b4,LM_FIR[0][2]),M(b3,LM_FIR[1][2])))),M(b0,LM_FIR[4][2])),A(M(b11,LM_FIR[5][2]),M(b10,LM_FIR[6][2]))))),M(b7,LM_FIR[9][2]))),M(b5,LM_FIR[11][2]));
          a3=A(A(M(b6,LM_FIR[10][3]),A(A(M(b8,LM_FIR[8][3]),A(M(b9,LM_FIR[7][3]),A(A(A(M(b1,LM_FIR[3][3]),A(M(b2,LM_FIR[2][3]),A(M(b4,LM_FIR[0][3]),M(b3,LM_FIR[1][3])))),M(b0,LM_FIR[4][3])),A(M(b11,LM_FIR[5][3]),M(b10,LM_FIR[6][3]))))),M(b7,LM_FIR[9][3]))),M(b5,LM_FIR[11][3]));
          break;
        case 5:
          a0=A(A(M(b7,LM_FIR[10][0]),A(A(M(b9,LM_FIR[8][0]),A(A(M(b10,LM_FIR[7][0]),M(b11,LM_FIR[6][0])),A(A(A(M(b2,LM_FIR[3][0]),A(M(b3,LM_FIR[2][0]),A(M(b5,LM_FIR[0][0]),M(b4,LM_FIR[1][0])))),M(b1,LM_FIR[4][0])),M(b0,LM_FIR[5][0])))),M(b8,LM_FIR[9][0]))),M(b6,LM_FIR[11][0]));
          a1=A(A(M(b7,LM_FIR[10][1]),A(A(M(b9,LM_FIR[8][1]),A(A(M(b10,LM_FIR[7][1]),M(b11,LM_FIR[6][1])),A(A(A(M(b2,LM_FIR[3][1]),A(M(b3,LM_FIR[2][1]),A(M(b5,LM_FIR[0][1]),M(b4,LM_FIR[1][1])))),M(b1,LM_FIR[4][1])),M(b0,LM_FIR[5][1])))),M(b8,LM_FIR[9][1]))),M(b6,LM_FIR[11][1]));
          a2=A(A(M(b7,LM_FIR[10][2]),A(A(M(b9,LM_FIR[8][2]),A(A(M(b10,LM_FIR[7][2]),M(b11,LM_FIR[6][2])),A(A(A(M(b2,LM_FIR[3][2]),A(M(b3,LM_FIR[2][2]),A(M(b5,LM_FIR[0][2]),M(b4,LM_FIR[1][2])))),M(b1,LM_FIR[4][2])),M(b0,LM_FIR[5][2])))),M(b8,LM_FIR[9][2]))),M(b6,LM_FIR[11][2]));
          a3=A(A(M(b7,LM_FIR[10][3]),A(A(M(b9,LM_FIR[8][3]),A(A(M(b10,LM_FIR[7][3]),M(b11,LM_FIR[6][3])),A(A(A(M(b2,LM_FIR[3][3]),A(M(b3,LM_FIR[2][3]),A(M(b5,LM_FIR[0][3]),M(b4,LM_FIR[1][3])))),M(b1,LM_FIR[4][3])),M(b0,LM_FIR[5][3])))),M(b8,LM_FIR[9][3]))),M(b6,LM_FIR[11][3]));
          break;
        case 6:
          a0=A(A(M(b8,LM_FIR[10][0]),A(M(b9,LM_FIR[9][0]),A(A(M(b11,LM_FIR[7][0]),M(b10,LM_FIR[8][0])),A(M(b0,LM_FIR[6][0]),A(A(A(M(b3,LM_FIR[3][0]),A(M(b4,LM_FIR[2][0]),A(M(b6,LM_FIR[0][0]),M(b5,LM_FIR[1][0])))),M(b2,LM_FIR[4][0])),M(b1,LM_FIR[5][0])))))),M(b7,LM_FIR[11][0]));
          a1=A(A(M(b8,LM_FIR[10][1]),A(M(b9,LM_FIR[9][1]),A(A(M(b11,LM_FIR[7][1]),M(b10,LM_FIR[8][1])),A(M(b0,LM_FIR[6][1]),A(A(A(M(b3,LM_FIR[3][1]),A(M(b4,LM_FIR[2][1]),A(M(b6,LM_FIR[0][1]),M(b5,LM_FIR[1][1])))),M(b2,LM_FIR[4][1])),M(b1,LM_FIR[5][1])))))),M(b7,LM_FIR[11][1]));
          a2=A(A(M(b8,LM_FIR[10][2]),A(M(b9,LM_FIR[9][2]),A(A(M(b11,LM_FIR[7][2]),M(b10,LM_FIR[8][2])),A(M(b0,LM_FIR[6][2]),A(A(A(M(b3,LM_FIR[3][2]),A(M(b4,LM_FIR[2][2]),A(M(b6,LM_FIR[0][2]),M(b5,LM_FIR[1][2])))),M(b2,LM_FIR[4][2])),M(b1,LM_FIR[5][2])))))),M(b7,LM_FIR[11][2]));
          a3=A(A(M(b8,LM_FIR[10][3]),A(M(b9,LM_FIR[9][3]),A(A(M(b11,LM_FIR[7][3]),M(b10,LM_FIR[8][3])),A(M(b0,LM_FIR[6][3]),A(A(A(M(b3,LM_FIR[3][3]),A(M(b4,LM_FIR[2][3]),A(M(b6,LM_FIR[0][3]),M(b5,LM_FIR[1][3])))),M(b2,LM_FIR[4][3])),M(b1,LM_FIR[5][3])))))),M(b7,LM_FIR[11][3]));
          break;
        case 7:
          a0=A(A(M(b9,LM_FIR[10][0]),A(A(M(b11,LM_FIR[8][0]),M(b10,LM_FIR[9][0])),A(M(b0,LM_FIR[7][0]),A(M(b1,LM_FIR[6][0]),A(A(A(M(b4,LM_FIR[3][0]),A(M(b5,LM_FIR[2][0]),A(M(b7,LM_FIR[0][0]),M(b6,LM_FIR[1][0])))),M(b3,LM_FIR[4][0])),M(b2,LM_FIR[5][0])))))),M(b8,LM_FIR[11][0]));
          a1=A(A(M(b9,LM_FIR[10][1]),A(A(M(b11,LM_FIR[8][1]),M(b10,LM_FIR[9][1])),A(M(b0,LM_FIR[7][1]),A(M(b1,LM_FIR[6][1]),A(A(A(M(b4,LM_FIR[3][1]),A(M(b5,LM_FIR[2][1]),A(M(b7,LM_FIR[0][1]),M(b6,LM_FIR[1][1])))),M(b3,LM_FIR[4][1])),M(b2,LM_FIR[5][1])))))),M(b8,LM_FIR[11][1]));
          a2=A(A(M(b9,LM_FIR[10][2]),A(A(M(b11,LM_FIR[8][2]),M(b10,LM_FIR[9][2])),A(M(b0,LM_FIR[7][2]),A(M(b1,LM_FIR[6][2]),A(A(A(M(b4,LM_FIR[3][2]),A(M(b5,LM_FIR[2][2]),A(M(b7,LM_FIR[0][2]),M(b6,LM_FIR[1][2])))),M(b3,LM_FIR[4][2])),M(b2,LM_FIR[5][2])))))),M(b8,LM_FIR[11][2]));
          a3=A(A(M(b9,LM_FIR[10][3]),A(A(M(b11,LM_FIR[8][3]),M(b10,LM_FIR[9][3])),A(M(b0,LM_FIR[7][3]),A(M(b1,LM_FIR[6][3]),A(A(A(M(b4,LM_FIR[3][3]),A(M(b5,LM_FIR[2][3]),A(M(b7,LM_FIR[0][3]),M(b6,LM_FIR[1][3])))),M(b3,LM_FIR[4][3])),M(b2,LM_FIR[5][3])))))),M(b8,LM_FIR[11][3]));
          break;
        case 8:
          a0=A(M(b9,LM_FIR[11][0]),A(A(M(b10,LM_FIR[10][0]),M(b11,LM_FIR[9][0])),A(M(b0,LM_FIR[8][0]),A(M(b1,LM_FIR[7][0]),A(M(b2,LM_FIR[6][0]),A(A(A(M(b5,LM_FIR[3][0]),A(M(b6,LM_FIR[2][0]),A(M(b8,LM_FIR[0][0]),M(b7,LM_FIR[1][0])))),M(b4,LM_FIR[4][0])),M(b3,LM_FIR[5][0])))))));
          a1=A(M(b9,LM_FIR[11][1]),A(A(M(b10,LM_FIR[10][1]),M(b11,LM_FIR[9][1])),A(M(b0,LM_FIR[8][1]),A(M(b1,LM_FIR[7][1]),A(M(b2,LM_FIR[6][1]),A(A(A(M(b5,LM_FIR[3][1]),A(M(b6,LM_FIR[2][1]),A(M(b8,LM_FIR[0][1]),M(b7,LM_FIR[1][1])))),M(b4,LM_FIR[4][1])),M(b3,LM_FIR[5][1])))))));
          a2=A(M(b9,LM_FIR[11][2]),A(A(M(b10,LM_FIR[10][2]),M(b11,LM_FIR[9][2])),A(M(b0,LM_FIR[8][2]),A(M(b1,LM_FIR[7][2]),A(M(b2,LM_FIR[6][2]),A(A(A(M(b5,LM_FIR[3][2]),A(M(b6,LM_FIR[2][2]),A(M(b8,LM_FIR[0][2]),M(b7,LM_FIR[1][2])))),M(b4,LM_FIR[4][2])),M(b3,LM_FIR[5][2])))))));
          a3=A(M(b9,LM_FIR[11][3]),A(A(M(b10,LM_FIR[10][3]),M(b11,LM_FIR[9][3])),A(M(b0,LM_FIR[8][3]),A(M(b1,LM_FIR[7][3]),A(M(b2,LM_FIR[6][3]),A(A(A(M(b5,LM_FIR[3][3]),A(M(b6,LM_FIR[2][3]),A(M(b8,LM_FIR[0][3]),M(b7,LM_FIR[1][3])))),M(b4,LM_FIR[4][3])),M(b3,LM_FIR[5][3])))))));
          break;
        case 9:
          a0=A(A(M(b11,LM_FIR[10][0]),A(A(M(b1,LM_FIR[8][0]),A(M(b2,LM_FIR[7][0]),A(M(b3,LM_FIR[6][0]),A(A(A(M(b6,LM_FIR[3][0]),A(M(b7,LM_FIR[2][0]),A(M(b9,LM_FIR[0][0]),M(b8,LM_FIR[1][0])))),M(b5,LM_FIR[4][0])),M(b4,LM_FIR[5][0]))))),M(b0,LM_FIR[9][0]))),M(b10,LM_FIR[11][0]));
          a1=A(A(M(b11,LM_FIR[10][1]),A(A(M(b1,LM_FIR[8][1]),A(M(b2,LM_FIR[7][1]),A(M(b3,LM_FIR[6][1]),A(A(A(M(b6,LM_FIR[3][1]),A(M(b7,LM_FIR[2][1]),A(M(b9,LM_FIR[0][1]),M(b8,LM_FIR[1][1])))),M(b5,LM_FIR[4][1])),M(b4,LM_FIR[5][1]))))),M(b0,LM_FIR[9][1]))),M(b10,LM_FIR[11][1]));
          a2=A(A(M(b11,LM_FIR[10][2]),A(A(M(b1,LM_FIR[8][2]),A(M(b2,LM_FIR[7][2]),A(M(b3,LM_FIR[6][2]),A(A(A(M(b6,LM_FIR[3][2]),A(M(b7,LM_FIR[2][2]),A(M(b9,LM_FIR[0][2]),M(b8,LM_FIR[1][2])))),M(b5,LM_FIR[4][2])),M(b4,LM_FIR[5][2]))))),M(b0,LM_FIR[9][2]))),M(b10,LM_FIR[11][2]));
          a3=A(A(M(b11,LM_FIR[10][3]),A(A(M(b1,LM_FIR[8][3]),A(M(b2,LM_FIR[7][3]),A(M(b3,LM_FIR[6][3]),A(A(A(M(b6,LM_FIR[3][3]),A(M(b7,LM_FIR[2][3]),A(M(b9,LM_FIR[0][3]),M(b8,LM_FIR[1][3])))),M(b5,LM_FIR[4][3])),M(b4,LM_FIR[5][3]))))),M(b0,LM_FIR[9][3]))),M(b10,LM_FIR[11][3]));
          break;
        case 10:
          a0=A(A(M(b0,LM_FIR[10][0]),A(A(M(b2,LM_FIR[8][0]),A(M(b3,LM_FIR[7][0]),A(M(b4,LM_FIR[6][0]),A(A(A(M(b7,LM_FIR[3][0]),A(M(b8,LM_FIR[2][0]),A(M(b10,LM_FIR[0][0]),M(b9,LM_FIR[1][0])))),M(b6,LM_FIR[4][0])),M(b5,LM_FIR[5][0]))))),M(b1,LM_FIR[9][0]))),M(b11,LM_FIR[11][0]));
          a1=A(A(M(b0,LM_FIR[10][1]),A(A(M(b2,LM_FIR[8][1]),A(M(b3,LM_FIR[7][1]),A(M(b4,LM_FIR[6][1]),A(A(A(M(b7,LM_FIR[3][1]),A(M(b8,LM_FIR[2][1]),A(M(b10,LM_FIR[0][1]),M(b9,LM_FIR[1][1])))),M(b6,LM_FIR[4][1])),M(b5,LM_FIR[5][1]))))),M(b1,LM_FIR[9][1]))),M(b11,LM_FIR[11][1]));
          a2=A(A(M(b0,LM_FIR[10][2]),A(A(M(b2,LM_FIR[8][2]),A(M(b3,LM_FIR[7][2]),A(M(b4,LM_FIR[6][2]),A(A(A(M(b7,LM_FIR[3][2]),A(M(b8,LM_FIR[2][2]),A(M(b10,LM_FIR[0][2]),M(b9,LM_FIR[1][2])))),M(b6,LM_FIR[4][2])),M(b5,LM_FIR[5][2]))))),M(b1,LM_FIR[9][2]))),M(b11,LM_FIR[11][2]));
          a3=A(A(M(b0,LM_FIR[10][3]),A(A(M(b2,LM_FIR[8][3]),A(M(b3,LM_FIR[7][3]),A(M(b4,LM_FIR[6][3]),A(A(A(M(b7,LM_FIR[3][3]),A(M(b8,LM_FIR[2][3]),A(M(b10,LM_FIR[0][3]),M(b9,LM_FIR[1][3])))),M(b6,LM_FIR[4][3])),M(b5,LM_FIR[5][3]))))),M(b1,LM_FIR[9][3]))),M(b11,LM_FIR[11][3]));
          break;
        case 11:
          a0=A(A(M(b1,LM_FIR[10][0]),A(A(M(b3,LM_FIR[8][0]),A(M(b4,LM_FIR[7][0]),A(M(b5,LM_FIR[6][0]),A(A(A(M(b8,LM_FIR[3][0]),A(M(b9,LM_FIR[2][0]),A(M(b11,LM_FIR[0][0]),M(b10,LM_FIR[1][0])))),M(b7,LM_FIR[4][0])),M(b6,LM_FIR[5][0]))))),M(b2,LM_FIR[9][0]))),M(b0,LM_FIR[11][0]));
          a1=A(A(M(b1,LM_FIR[10][1]),A(A(M(b3,LM_FIR[8][1]),A(M(b4,LM_FIR[7][1]),A(M(b5,LM_FIR[6][1]),A(A(A(M(b8,LM_FIR[3][1]),A(M(b9,LM_FIR[2][1]),A(M(b11,LM_FIR[0][1]),M(b10,LM_FIR[1][1])))),M(b7,LM_FIR[4][1])),M(b6,LM_FIR[5][1]))))),M(b2,LM_FIR[9][1]))),M(b0,LM_FIR[11][1]));
          a2=A(A(M(b1,LM_FIR[10][2]),A(A(M(b3,LM_FIR[8][2]),A(M(b4,LM_FIR[7][2]),A(M(b5,LM_FIR[6][2]),A(A(A(M(b8,LM_FIR[3][2]),A(M(b9,LM_FIR[2][2]),A(M(b11,LM_FIR[0][2]),M(b10,LM_FIR[1][2])))),M(b7,LM_FIR[4][2])),M(b6,LM_FIR[5][2]))))),M(b2,LM_FIR[9][2]))),M(b0,LM_FIR[11][2]));
          a3=A(A(M(b1,LM_FIR[10][3]),A(A(M(b3,LM_FIR[8][3]),A(M(b4,LM_FIR[7][3]),A(M(b5,LM_FIR[6][3]),A(A(A(M(b8,LM_FIR[3][3]),A(M(b9,LM_FIR[2][3]),A(M(b11,LM_FIR[0][3]),M(b10,LM_FIR[1][3])))),M(b7,LM_FIR[4][3])),M(b6,LM_FIR[5][3]))))),M(b2,LM_FIR[9][3]))),M(b0,LM_FIR[11][3]));
          break;
        }
    a0=std::fabs(a0); a1=std::fabs(a1); a2=std::fabs(a2); a3=std::fabs(a3);
    int kc=(wpos+6)%12;
    float center=std::fabs(ispHist[kc]);
    float m=std::max(std::max(a0,a1),std::max(a2,std::max(a3,center)));
    ispPos = (ispPos>10)?0:ispPos+1;
    return (m>BIG)? m : f32(m*MUL32);
  }

  // ---------- detektor STEREO (process2 @0x4942). DWA bufory ISP (L,R), WSPOLNY
  //  wpos. FIR liczony ROWNOLEGLE dla L i R, ale sumowanie SEKWENCYJNE (fold-left
  //  tap 0..11) — RÓZNI sie od mono (bushy tree) o ~1 ULP, wiec MUSI byc odtworzone
  //  dokladnie. det = max(|FIR_L 4fazy|, |FIR_R 4fazy|, |centerL|, |centerR|)*MUL32.
  //  Odtworzone z p2_dis.txt @0x4a32-0x4cd7. ----------
  float detect2(float xiL, float xiR){
    ispHist[ispPos]=xiL; ispHistR[ispPos]=xiR;
    const int wpos=ispPos;
    auto A=[](float a,float b){ return f32(a+b); };
    auto M=[](float a,float b){ return f32(a*b); };
    float aL[4]={0,0,0,0}, aR[4]={0,0,0,0};
    for(int t=0;t<12;++t){
      int bi=(wpos - t + 12) % 12;
      float sL=ispHist[bi], sR=ispHistR[bi];
      for(int ph=0;ph<4;++ph){
        aL[ph]=A(aL[ph], M(sL, LM_FIR[t][ph]));
        aR[ph]=A(aR[ph], M(sR, LM_FIR[t][ph]));
      }
    }
    float m=0;
    for(int ph=0;ph<4;++ph){ m=std::max(m,std::fabs(aL[ph])); m=std::max(m,std::fabs(aR[ph])); }
    int kc=(wpos+6)%12;
    m=std::max(m,std::fabs(ispHist[kc]));
    m=std::max(m,std::fabs(ispHistR[kc]));
    ispPos = (ispPos>10)?0:ispPos+1;
    return (m>BIG)? m : f32(m*MUL32);
  }

  // ---------- rdzen obwiedni: 1 probka -> gEnv ----------
  float envelope(float det, float xi){
    float x0=det, x1=0, x2=0, x9=xi, x11, x12, x13=oac, x14=0, x15=0, x6=0;
    double x1d, x11d, x13d, x12d;
    uint16_t idx=cc4; int r9=idx;
    x11=A[idx];
    c24=(uint16_t)(c24+1); uint16_t ax=c24;
    x12=o68;
    bool window=(ax>c20);
    if(!window){ if(x12>selVal()) window=true; }
    int al=0;

    if(!window){ x2=o40; x1=x13; goto L397e; }
    else       { goto L3bb4; }

  L3bb4:
    c24=0; o30=x12; x14=o40;
    if(x13<x14) goto L3c6e;
    if(b8==0)   goto L4442;
    x1=f32(F_015625*x13);
    if(x11<=x1) goto L4614;
    b8=0;
    if(ca!=0 && x13>o54){ ca=0; o50=f32(F_100000501*o50); }
    goto L3c26;
  L4614:
    if(b8<=0x13) goto L3c6e;
    if(ca==0)    goto L3c26;
    if(x13<=o54) goto L3c26;
    ca=0; o50=f32(F_100000501*o50); goto L3c26;
  L4442:
    if(ca==0) goto L3c55;
    sel=0;
    if(x12<x0)  goto L3c6e;
    if(x12<=x0) goto L472c;
    b8=1; x2=o54; goto L3c73;
  L472c:
    if(x13<=o34) goto L3c6e;
    b8=1; x2=o54; goto L3c73;
  L3c55:
    b8=1;
    if(sel!=0) goto L3c6e;
    sel=1; goto L3c6e;
  L3c26:
    x1=DL[cc4]; x2=o04;
    if(x2>x1){ x2=f32(o04*F_100097); sel=2; o54=x2; goto L3c73; }
    x2=f32(F_100097*x1); sel=1; o54=x2; goto L3c73;
  L3c6e:
    x2=o54; goto L3c73;
  L3c73:
    if(x12<=x2) goto L3ca8;
    o54=x12;
    if(x12>=x14) goto L3c96;
    if(x14==x13) goto L3c96;
    if(x13<=o34) goto L46a3;
  L3c96:
    o40=x12; sel=0; x2=x12; goto L3ca8;
  L46a3:
    if(x11>x12) goto L3cbc;
    x1=x13; x2=x12; goto L3d1e;
  L3ca8:
    if(x11<=x2) goto L4045;
    if(x13<x11) goto L4478;
    goto L3cbc;
  L4045:
    x1=x13; goto L3d0d_prep;
  L4478:
    o98=NEG1; x2=o54; x1=oac; goto L3d0d_prep;
  L3cbc:
    { double t=(double)x11*DABA0; x2=f32(t); o54=x2; sel=0;
      if(x2<=x13){ } else { o54=x13; x2=x13; } }
    if(x13==o40) o40=x2;
    if(x2>o50)   o50=x2;
    x1=oac; goto L3d0d_prep;
  L3d0d_prep:
    if(x13>o34) goto L3dbc;
  L3d1e:                       // ASM 3d1e: comiss x2,[0x40]; ja 3dbc; else x2=[0x40]
    x14=o40;
    if(x2>x14) goto L3dbc;
    x2=x14; goto L397e;
  L3dbc:
    o40=x2; goto L397e;

  L397e:
    c28=(uint16_t)(c28+1); ax=c28;
    if(ax>c26) goto L399b;
    x12=o34;
    if(x0<=x12) goto L39e4;
    goto L399b;
  L399b:
    c28=1; o34=x0;
    x12=f32(std::max(o0c,x0)); o3c=x12;
    x13=o38;
    if(x13<=x12) goto L3d7c;
    if(x13>ob0)  goto L3d7c;
    o3c=x13; x12=x0; goto L39e4_eax1;
  L3d7c:
    x12=x0; goto L39e4_eax1;
  L39e4_eax1:
    c2c=(uint16_t)(c2c+1);
    if(c2c>c2a){ c2c=1; o38=x12; goto L3a09; }
    if(x0<o38) goto L3a09;
    c2c=1; o38=x12; goto L3a09;
  L39e4:
    c2c=(uint16_t)(c2c+1);
    if(c2c>c2a){ c2c=(uint16_t)ax; o38=x12; goto L3a09; }
    if(x0<o38) goto L3a09;
    c2c=(uint16_t)ax; o38=x12; goto L3a09;
  L3a09:
    if(x2<=x1) goto L3d31;   // RELEASE
    if(x2!=o98) goto L3f23;
    x1d=(double)x1; x13d=d90; goto L3a2c_dbl;
  L3f23:
    x13d=(double)x2;
    { double t=x13d*DABA8; x14=f32(t); x15=(double)x14; }
    x1d=(double)x1; x12d=d90;
    { double t=x13d-x1d; t=t*dATK; t=t+(double)x15;
      float x13f=f32(t); x13f=f32(x13f-x14); x13d=(double)x13f; }
    if(x13d>x12d) { d90=x13d; }
    else if(x12d>=x1d) { d90=x13d; }
    else x13d=x12d;
    { float x12f=f32(F_476e7*x2); double x12dd=(double)x12f;
      if(x12dd>x13d){ d90=x12dd; x13d=x12dd; } }
    o50=x2; o98=x2; o4c=f32(F_994987*x2);
    { float v=f32(F_010232*x2); o9c=v; oa0=v; }
    x13d=d90;
    goto L3a2c_dbl;
  L3a2c_dbl:
    {
      double s=x1d+x13d; float sf=f32(s); oac=sf; x1=sf;
      if(sf<x2){ }
      else {
        o50=x2; oac=x2;
        double t=x13d*DABB0; float tf=f32(t); d90=(double)tf;
        o98=NEG1; x1=x2;
      }
    }
    goto L3a7b;
  L3a7b:
    b8=0; ca=1; sel=0;
    goto L3a91;

  L3d31:
    if(x2<=x11) goto L3ee5;
    if(x11<=oa0) goto L441b;
    x12d=(double)x2; goto L3d53;
  L3ee5:
    if(x1<=x11) goto L3a91;
    if(x1<o4c)  goto L3f14;
    { int eax=(r9+1)&0xffff; if(c20<=eax) eax=0;
      if(x11<=A[eax]) goto L3a91; }
    goto L3f14;
  L3f14:
    x12d=(double)x11; goto L3d53;
  L3d53:
    { double t=(double)x1; t=t-x12d; t=t*dREL; double r=x12d+t;
      float rf=f32(r); oac=rf; x1=rf; }
    goto L3a91;
  L441b:
    if(x1>=o50) goto L3a91;
    { double t=(double)x1*DABB8; float tf=f32(t); oac=tf; x1=tf; }
    goto L3a91;

  L3a91:
    if(x11<=x1) goto L3adb;
    oac=x11; b8=0; ca=1;
    if(x11>x2) o40=x11;
    if(x11>o50) o50=x11;
    if(c24==0 && x11>o54) o54=x11;
    goto L3adb;

  L3adb:
    o68=x0; A[cc4]=x0;
    x1=o3c; x11=ob0;
    if(x1>x11) goto L3d8a;
    if(x0>oa8) goto L400f;
    { double t=(double)x11*DABB8; float tf=f32(t); ob0=tf; x0=tf; }
    if(bc!=0) bc--;
    goto L3b3c;
  L400f:
    { double p=(double)x1, a=(double)x11; a=a-p; a=a*dSLOW2; a=a+p;
      float f=f32(a); ob0=f; x0=f; }
    goto L3b3c;
  L3d8a:
    { double p=(double)x1, a=(double)x11; a=a-p; a=a*dSLOW1; double r=p+a;
      float f=f32(r); ob0=f; x0=f; }
    goto L3b3c;

  L3b3c:
    DL[cc4]=x0; B[cc4]=x9;
    if(r9>c22) goto L404e;
    cc4=(uint16_t)((r9+1)&0xffff);
    goto LOUT;

  L404e:
    cc4=0;
    { int e=(b8-1)&0xff; if(e<=0x12) b8=(uint8_t)((b8+1)&0xff); }
    x9=o34; x13=o9c;
    if(x9<=x13) goto L44c3;
    b9=0x64; goto L4091;
  L44c3:
    if(b9==0){ oa0=NEG1; goto L40a4; }
    b9=(uint8_t)((b9-1)&0xff);
    if(b9!=0) goto L4091;
    oa0=NEG1; goto L40a4;
  L4091:
    if(cc0>0xf) oa0=x13;
    goto L40a4;
  L40a4:
    x1=oa4;
    if(x9<=x1) goto L4494;
    ba=0x64; goto L40bd;
  L4494:
    if(ba==0){ al=0; goto L43d3; }
    ba=(uint8_t)((ba-1)&0xff); al=ba;
    x2=ob0;
    if(x9>x2) bc=HOLD;
    goto L40dc;
  L43d3:
    al=0; x2=ob0;
    if(x9>x2){ bc=HOLD; goto L40dc; }
    goto L43e5;
  L40bd:
    x2=ob0;
    if(x9<=x2) goto L40e4;
    al=0x64; bc=HOLD;
    goto L40dc;
  L40dc:
    if((al&0xff)==0) goto L43e5;
    goto L40e4;
  L40e4:
    if(bc==0) goto L43e5;
    x0=x1; oa8=x0;
    x12=o3c; x11d=(double)x12;
    { double t=(double)ob4 - x11d; t=t*DABC0; t=t+x11d; ob4=f32(t); }
    goto L413c;
  L413c:
    { int e=(c2f+1)&0xff; c2f=(uint8_t)e; int r8=e;
      x14=o38;
      if(r8>c2e) goto L4335;
      x15=o48;
      if(x14>x15) goto L4335;
      goto L416b; }
  L4335:
    c2f=1; o48=x14;
    { double t=(double)x14*DAC00; o14=f32(t); }
    if(x9<=x2) goto L4174;
    x15=o44;
    { float x6f=f32(F_025*x15);
      if(x6f<=x9){ }
      else { x15=f32(x9+x9); o44=x15; } }
    goto L438b;
  L438b:
    if(c9==0) goto L417a;
    if(cc2!=0) goto L4188;
    c9=0;
    { float x7=f32(F_025*o58); if(x7<=x9){} else o58=x9; }
    goto L4194;
  L416b:
    x0=o14; x14=x15; x15=o44; goto L417a;
  L4174:
    x15=o44; goto L417a;
  L417a:
    if(cc2!=0) cc2=(uint16_t)((cc2-1)&0xffff);
    goto L4194;
  L4188:
    cc2=(uint16_t)((cc2-1)&0xffff); goto L4194;
  L4194:
    x2=ob0; x0=o14;
    if(x2<=x0) goto L44ee;
    if(x12<=x2) goto L462e;
    { float x6f=f32(F_0404*x0); x6=x6f;
      if(x6f<=x1) goto L46ba; }
    oa4=x6; oa8=x6; o58=x12; cc2=0x18f;
    if(x14>=x12) o58=x14;
    goto L41ea;
  L462e:
    x1=o58;
    if(x14>=x1){ o58=x14; goto L41ea; }
    if(cc2!=0) goto L41ea;
    x1=f32(x1*F_999913); o58=x1; goto L41ea;
  L46ba:
    x1=o58;
    if(x12<=x1) goto L4633;
    oa4=x6; oa8=x6; o58=x12; cc2=0x18f;
    if(x14>=x12) o58=x14;
    goto L41ea;
  L4633:
    x1=o58;
    if(x14>=x1){ o58=x14; goto L41ea; }
    if(cc2!=0) goto L41ea;
    x1=f32(x1*F_999913); o58=x1; goto L41ea;
  L44ee:
    if(x12<=x2){ x1=o04; goto L450e; }
    { float x0t=f32(F_0404*x2);
      if(x0t>x1) goto L46ce; }
    x1=o04; goto L450e;
  L46ce:
    { float x0t=f32(F_0404*x2); oa4=x0t; oa8=x0t; }
    { double t=(double)x2*DABC8; float f=f32(t); if(f>o58) o58=f; }
    cc2=0x18f; x1=o04; goto L450e;
  L41ea:
    x1=o04;
    if(x0<=x1) goto L450e;
    { float x6f=f32(F_036*x2); float b4=ob4;
      if(b4<=x6f) goto L470b; }
    o04=x0; c8=0;
    { double t=(double)x0*DBANK08; o08=f32(t); x1=o08; }
    cc0=0x190; goto L424a;
  L470b:
    c2f=c2e; goto L450e;  // ASM 0x470b: c2f=al, gdzie al=c2e (zaladowane @0x4148, nietkniete do 0x470b)
  L450e:
    x1=o04;
    if(x1<=x14) goto L426b;
    if(x1<=DL[0]) goto L426b;
    if(x1<=o00) goto L426b;
    goto L4530;
  L424a:
    if(x0<=x14) goto L42a3;
    if(x0<=DL[0]) goto L45ce;
    x1=x0;
    if(x1>o00) goto L4530;
    if(c8==0) goto L45ce;
    if(x9<=x2) goto L45ce;
    c8=0;
    { double t=(double)o04*DBANK08; o08=f32(t); x1=o08; }
    goto L42a3;
  L4530:
    if(x9<=oa8) goto L4569;
    { float x0m=f32(std::max(o10,x12)); double x0dd=(double)x0m;
      double t=(double)x1 - x0dd; t=t*DABD0; double r=x0dd+t;
      float f=f32(r); o04=f; x1=f; }
    goto L4569;
  L4569:
    { float x0t=o00; if(x0t>x1){ o04=x0t; x1=x0t; } }
    if(cc0!=0) goto L4659;
    cc0=0xf; c8=0;
    { double t=(double)x1*DBANK08; o08=f32(t); x1=o08; }
    goto L45ae;
  L4659:
    c8=1; cc0=(uint16_t)((cc0-1)&0xffff); goto L426b;
  L45ae:
    if(o9c<=x9) goto L42a3;
    b9=0; oa0=NEG1; goto L42a3;
  L426b:
    if(c8==0) goto L45ce;
    if(x9<=x2) goto L45ce;
    c8=0;
    { double t=(double)o04*DBANK08; o08=f32(t); x1=o08; }
    goto L42a3;
  L45ce:
    x1=o08; goto L42a3;
  L42a3:
    if(x14>=x15){} else { x15=f32(x15*F_999566); x14=x15; }
    o44=x14;
    if(x12<=x1) goto LGAINOUT;
    x0=o0c;
    { float x9c9=f32(F_868497*x14);
      if(x2<=x9c9) goto L45d8; }
    { float x9c9=f32(F_919945*x14);
      if(x2<=x9c9) goto L4683; }
    x14=f32(x14*F_953070);
    if(x2<=x14) goto L4713;
    { double t=x11d*DABD8; o0c=f32(t); }
    goto L45ec;
  L45d8:
    { double t=x11d*DABF0; o0c=f32(t); } goto L45ec;
  L4683:
    { double t=x11d*DABE8; o0c=f32(t); } goto L45ec;
  L4713:
    { double t=x11d*DABE0; o0c=f32(t); } goto L45ec;
  L45ec:
    { float x11f=o0c;
      if(x1<=x11f) goto LGAINOUT;
      double t=(double)x0*DABF8; float f=f32(t);
      float v=f32(std::max(x1,f)); o0c=v; }
    goto LGAINOUT;
  L43e5:
    c9=1; x0=NEG1; oa8=x0;
    x12=o3c; x11d=(double)x12;
    { double t=(double)ob4 - x11d; t=t*DABC0; t=t+x11d; ob4=f32(t); }
    goto L413c;

  LGAINOUT:
  LOUT:
    return oac;
  }

  // ---------- pelny proces jednej probki (z delayem 61) ----------
  static const int DELAY=61;
  float dly[64]; int dlyPos;
  void resetDelay(){ std::memset(dly,0,sizeof(dly)); dlyPos=0; }

  // Zwraca masked gain (1/gEnv * outGain, mantysa &~1) po advancie stanu obwiedni.
  float lastGEnv=0.f; // diagnostyka bit-exact
  float stepGain(float xi){
    float det=detect(xi);
    float gEnv=envelope(det, xi);
    lastGEnv=gEnv;
    float g=f32(F_1/gEnv); g=f32(g*o1c);
    uint32_t gi; std::memcpy(&gi,&g,4); gi&=0xfffffffeu; std::memcpy(&g,&gi,4);
    return g;
  }

  // STEREO: det z detect2(L,R), wspolny envelope, wspolny masked gain (process2).
  //  xi w envelope trafia tylko do martwego B[] (nie wplywa na gEnv) — przekazujemy L.
  float stepGain2(float xiL, float xiR){
    float det=detect2(xiL, xiR);
    float gEnv=envelope(det, xiL);
    lastGEnv=gEnv;
    float g=f32(F_1/gEnv); g=f32(g*o1c);
    uint32_t gi; std::memcpy(&gi,&g,4); gi&=0xfffffffeu; std::memcpy(&g,&gi,4);
    return g;
  }
};

// per-level BIT-EXACT reset state — ODCZYTANE DOKLADNIE z Maximizer::setThresh
//   (la_LoudMax64.so @0x3344, gala-init branch). setThresh jest wolany na 1. run()
//   i wpieka prog do stanu: o00,o04=thr; o08,o0c=prevPeak; o10; o30..o68,ob0,ob4=thr;
//   oa4,oa8=aEnv. Wczesniej liczono default*thrLin (float) co dawalo 1-ULP rozjazd
//   w prevPeak/o10 (bit-exact dla lvl2/3, ale kumulacja bledu @lvl4). Teraz hex jest
//   DOKLADNIE stanem po setThresh(db) — pelny bit-exact wszystkich progow.
struct LevelInit { uint32_t thr, prevPeak, o10, aEnv; };
// index 1..5 -> dB -6/-12/-18/-24/-30. index 0 (off) = activate-default (thrLin=1).
static const LevelInit LM_LEVEL[6] = {
  /*0 off  */ {0x42000000u,0x416947b9u,0x41f40000u,0x3fa57a93u}, // activate defaults
  /*1 -6dB */ {0x41804dd0u,0x40e9d58au,0x41749460u,0x3f25df2cu},
  /*2 -12dB*/ {0x41009bc0u,0x406a6394u,0x40f52900u,0x3ea643eeu},
  /*3 -18dB*/ {0x4080ea00u,0x3feaf231u,0x4075be00u,0x3e26a917u},
  /*4 -24dB*/ {0x40013880u,0x3f6b8142u,0x3ff65400u,0x3da70e93u},
  /*5 -30dB*/ {0x3f818700u,0x3eec1052u,0x3f76ea00u,0x3d27740fu},
};
static const uint32_t LM_OUTGAIN = 0x41ff4004u; // 31.90625763, stale (thr-niezalezne)

} // anon (LMCore)

// ---------------------------------------------------------------------------
//  Enhancer::Limiter — interfejs wtyczki opakowujacy LMCore (bit-exact automat).
// ---------------------------------------------------------------------------
struct Enhancer::Limiter {
    int   srate = 0, nch = 1;
    int   level = 0;                 // 1..5 (prog), 0=off
    LMCore core;
    // per-kanal linia opozniajaca wejscia (DELAY=61 probek), rownolegle do rdzenia
    float dlyL[64] = {0}, dlyR[64] = {0};
    int   dlyPos = 0;

    void configure(int sr, int ch);
    void setLevel(int lvl);
    void reset();
    void process(float* x, int frames);
};

void Enhancer::Limiter::configure(int sr, int ch) {
    srate = sr; nch = std::max(1, std::min(2, ch));
    // (rdzen LoudMax ma stale wspolczynniki czasowe wpisane w reset(); sr wplywa
    //  tylko na dlugosc bufora look-ahead=55 i b22=53, ktore sa stale dla 44.1k.
    //  Oryginal jest zestrojony dla 44.1k; przy innym sr uzywamy tych samych stalych
    //  co daje najblizsze zachowanie — bit-exact zweryfikowany dla 44.1k.)
    setLevel(level);   // (re)bake stan dla biezacego poziomu + reset()
}

void Enhancer::Limiter::setLevel(int lvl) {
    level = std::max(0, std::min(5, lvl));
    reset();
}

void Enhancer::Limiter::reset() {
    const LevelInit& L = LM_LEVEL[level];
    core.reset(hexf(L.thr), hexf(LM_OUTGAIN), hexf(L.prevPeak),
               hexf(L.aEnv), hexf(L.o10), 55, 53);
    core.resetDelay();
    std::memset(dlyL,0,sizeof(dlyL)); std::memset(dlyR,0,sizeof(dlyR));
    dlyPos = 0;
}

// Mono: bit-exact 1:1 z Maximizer::process @0x3676. Stereo: bit-exact 1:1 z
// Maximizer::process2 @0x4942 — 2 bufory ISP (L,R), detektor max po obu kanalach,
// WSPOLNY envelope/gain, masked gain aplikowany do obu opoznionych probek.
void Enhancer::Limiter::process(float* x, int frames) {
    if (level <= 0) return;
    const int ch = nch;
    const int D = LMCore::DELAY;
    if (ch >= 2) {
        // STEREO — process2. Detektor+gain z obu kanalow, delay per kanal.
        for (int i = 0; i < frames; ++i) {
            float inL = x[i * ch];
            float inR = x[i * ch + 1];
            float g = core.stepGain2(inL, inR);   // masked gain (bit-exact z process2)
            float outL = f32(g * dlyL[dlyPos]);
            float outR = f32(g * dlyR[dlyPos]);
            dlyL[dlyPos] = inL;
            dlyR[dlyPos] = inR;
            x[i * ch] = outL;
            x[i * ch + 1] = outR;
            dlyPos = (dlyPos + 1) % D;
        }
        return;
    }
    // MONO — process. NIETKNIETE (bit-exact 83/83).
    for (int i = 0; i < frames; ++i) {
        float mono = x[i * ch];
        float g = core.stepGain(mono);   // masked gain (bit-exact z Winamp DSP / LADSPA)
        float inL = x[i * ch];
        float outL = f32(g * dlyL[dlyPos]);
        dlyL[dlyPos] = inL;
        x[i * ch] = outL;
        dlyPos = (dlyPos + 1) % D;
    }
}

// ============================================================================
//  Enhancer — sklejenie
// ============================================================================
Enhancer::~Enhancer() { delete lim_; }

void Enhancer::configure(int srate, int nch) {
    srate_ = srate; nch_ = std::max(1, std::min(2, nch));
    buildClarity();
    buildLoudness();
}

void Enhancer::buildLoudness() {
    if (loud_ <= 0 || srate_ <= 0) { delete lim_; lim_ = nullptr; return; }
    if (!lim_) lim_ = new Limiter();
    lim_->setLevel(loud_);           // ustaw poziom PRZED configure (bake w configure)
    lim_->configure(srate_, nch_);
}

void Enhancer::setLoudness(int level) {
    level = std::max(0, std::min(ENH_LEVELS, level));
    if (level == loud_) return;
    loud_ = level;
    buildLoudness();
}

void Enhancer::setClarity(int level) {
    level = std::max(0, std::min(ENH_LEVELS, level));
    if (level == clar_) return;
    clar_ = level;
    buildClarity();
}

void Enhancer::reset() {
    for (Biquad& b : ap_) b.clear();
    if (lim_) lim_->reset();
}

void Enhancer::process(float* x, int frames) {
    if (!active()) return;
    const int ch = nch_;
    if (clar_ > 0 && !ap_.empty()) {
        for (int i = 0; i < frames; ++i) {
            for (int c = 0; c < ch; ++c) {
                float s = x[i * ch + c];
                for (Biquad& b : ap_) s = b.run(c, s);
                x[i * ch + c] = s;
            }
        }
    }
    if (lim_) lim_->process(x, frames);
}

} // namespace bookbar
