#pragma once
//
// PV2KSM v0: more main screens (modern, retro, futuristic). Same inputs and helpers as NewUI.h.
//
#include "NewUI.h"

namespace nu
{
  // ---- shared pieces -----------------------------------------------------------------------------------------
  static const uint8_t seg7Mask[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

  // Seven segment digit. mask bit order a b c d e f g. off = colour of unlit segments (0 = draw nothing)
  template<class G> static void digit7(G &g, int x, int y, int w, int h, int th, int mask, uint16_t on, uint16_t off)
  {
    const int yG = y + (h - th) / 2, yD = y + h - th;
    const int vt0 = y + th + 1, vth = yG - vt0 - 1, vb0 = yG + th + 1, vbh = yD - vb0 - 1;
    struct S { int x, y, w, h; };
    const S s[7] = {{x + th + 1, y, w - 2 * th - 2, th}, {x + w - th, vt0, th, vth}, {x + w - th, vb0, th, vbh}, {x + th + 1, yD, w - 2 * th - 2, th},
                    {x, vb0, th, vbh}, {x, vt0, th, vth}, {x + th + 1, yG, w - 2 * th - 2, th}};
    for(int i = 0 ; i < 7 ; i++)
    {
      const bool lit = (mask >> i) & 1;
      if(!lit && !off) continue;
      g.fillRect(s[i].x, s[i].y, s[i].w, s[i].h, lit ? on : off);
      if(i == 0 || i == 3 || i == 6)   // chamfered ends of the horizontal segments
      {
        const uint16_t c = lit ? on : off;
        g.fillTriangle(s[i].x - th / 2, s[i].y + th / 2, s[i].x, s[i].y, s[i].x, s[i].y + th, c);
        g.fillTriangle(s[i].x + s[i].w + th / 2, s[i].y + th / 2, s[i].x + s[i].w, s[i].y, s[i].x + s[i].w, s[i].y + th, c);
      }
    }
  }

  // Frequency in seven segments, right aligned at xr. Returns the left edge.
  template<class G> static int freq7(G &g, const CkIn &in, int xr, int y, int w, int h, int th, int gap, uint16_t on, uint16_t off, uint16_t cSel = 0)
  {
    char fs[16]; freqStr(in, fs, sizeof(fs));
    const int n = (int)strlen(fs);
    int total = 0;
    for(int i = 0 ; i < n ; i++) total += fs[i] == '.' ? th + gap : w + gap;
    int x = xr - total + gap;
    int xs[16] = {0}, xe[16] = {0};
    for(int i = 0 ; i < n && i < 16 ; i++)
    {
      xs[i] = x;
      if(fs[i] == '.') { g.fillRect(x, y + h - th, th, th, on); x += th + gap; xe[i] = x - gap; }
      else { digit7(g, x, y, w, h, th, seg7Mask[fs[i] - '0'], on, off); x += w + gap; xe[i] = x - gap; }
    }
    if(cSel) digitCursorSpans(g, (uint8_t)in.hl, fs, xs, xe, y + h + 4, on, cSel);
    return xr - total + gap;
  }

  template<class G> static void chip(G &g, int x, int y, int w, int h, const char *s, uint16_t col, uint16_t fill = 0)
  {
    if(fill) g.fillSmoothRoundRect(x, y, w, h, h / 2, fill);
    rrAA(g, x, y, w, h, h / 2, col);
    text(g, FontB612Bold12, s, x + w / 2, y + h / 2, col, D_MC);
  }

  // Segmented horizontal bar, v 0..1
  template<class G> static void segBar(G &g, int x, int y, int w, int h, int n, float v, uint16_t on, uint16_t off, uint16_t hot = 0)
  {
    const int sw = (w - (n - 1) * 2) / n;
    for(int i = 0 ; i < n ; i++) g.fillRect(x + i * (sw + 2), y, sw, h, (i + 0.5f) / n <= v ? ((hot && i >= n * 3 / 4) ? hot : on) : off);
  }

  // ---- 1. Minimal (modern): one huge frequency, thin signal line ------------------------------------------------
  template<class G> static void layoutMinimal(G &g, const NuIn &n, const NuPal &p)
  {
    const CkIn &in = n.k;
    bgFill(g, p);
    chip(g, 8, 6, 40, 18, in.modeName, p.acc);
    text(g, FontB612Bold12, in.band ? in.band : "", 56, 15, p.txt2, D_ML);
    char fs[16]; const char *unit = freqStr(in, fs, sizeof(fs));
    float sc = 1.9f;
    g.setFont(&FontB612Big); g.setTextSize(sc); int fw = g.textWidth(fs);
    if(fw > 255) { sc = 1.9f * 255.f / fw; g.setTextSize(sc); fw = g.textWidth(fs); }     // long frequencies (SW/AM) must fit the screen
    text(g, FontB612Big, fs, 160 - fw / 2, 74, p.txt, D_ML);
    digitCursorFont(g, FontB612Big, sc, in.hl, fs, 160 - fw / 2, 74, p.acc, p.needle, -6);
    g.setTextSize(1.0f);
    text(g, FontB612Bold12, unit, 160 + fw / 2 + 4, 98, p.acc, D_ML);
    if(in.station && in.station[0]) nameFit(g, p, FontB612Bold16, in.station, 20, 118, 280, p.txt2, n.ms, D_MC);
    // signal line: thin track with a glowing head
    const int x0 = 20, x1 = 300, y = 142;
    g.fillSmoothRoundRect(x0, y - 1, x1 - x0, 3, 1, ckMix(p.bg1, 0xFFFF, 40));
    const int hx = x0 + (int)(n.sN * (x1 - x0));
    g.fillSmoothRoundRect(x0, y - 1, hx - x0, 3, 1, p.acc);
    glowCircle(g, hx, y, 4, p.acc, p.bg1); g.fillSmoothCircle(hx, y, 4, p.txt);
    char sv[10], nv[12]; nuSText(sv, sizeof(sv), ckStrength(in.rssi, in.mode == 0)); snprintf(nv, sizeof(nv), "%d dB", in.snr);
    text(g, FontB612Bold12, sv, x0, 157, p.txt, D_ML); text(g, FontB612Reg10, in.step ? in.step : "", 160, 157, p.muted, D_MC); text(g, FontB612Bold12, nv, x1, 157, p.txt, D_MR);
  }

  // ---- 2. Glass (modern): cards on a soft background ---------------------------------------------------------------
  template<class G> static void layoutGlass(G &g, const NuIn &n, const NuPal &p)
  {
    const CkIn &in = n.k;
    bgFill(g, p);
    card(g, 6, 24, 206, 76, 10, p);
    chip(g, 14, 31, 40, 18, in.modeName, p.acc);
    if(in.station && in.station[0]) nameFit(g, p, FontB612Bold12, in.station, 62, 40, 148, p.txt2, n.ms);
    char fs[16]; const char *unit = freqStr(in, fs, sizeof(fs));
    text(g, FontB612Big, fs, 14, 78, p.txt, D_ML);
    g.setFont(&FontB612Big); const int fw = g.textWidth(fs);
    text(g, FontB612Reg10, unit, 14 + fw + 4, 88, p.acc, D_ML);
    // two vertical meters
    for(int i = 0 ; i < 2 ; i++)
    {
      const int x = 220 + i * 50; const float v = i ? n.nN : n.sN;
      card(g, x, 24, 44, 76, 10, p);
      const int bx = x + 17, by = 30, bh = 40;
      g.fillSmoothRoundRect(bx, by, 10, bh, 4, ckMix(p.bg1, 0xFFFF, 36));
      const int fh = (int)(v * bh);
      if(fh > 1) g.fillSmoothRoundRect(bx, by + bh - fh, 10, fh, 4, i ? p.good : p.acc);
      text(g, FontB612Reg10, i ? "SNR" : "S", x + 22, 79, p.muted, D_MC);
      char b[10]; if(i) snprintf(b, sizeof(b), "%d", in.snr); else nuSText(b, sizeof(b), ckStrength(in.rssi, in.mode == 0));
      text(g, FontB612Bold12, b, x + 22, 91, p.txt, D_MC);
    }
    card(g, 6, 106, 308, 58, 10, p);
    const int px = scaleWalk(in, 18, 302, [&](int x, bool maj, double v, double major)
    {
      g.drawFastVLine(x, 116, maj ? 11 : 5, maj ? p.txt2 : p.muted);
      if(maj) { char b[12]; valStr(b, sizeof(b), v, major); if(x > 30 && x < 290) text(g, FontB612Reg10, b, x, 140, p.muted, D_MC); }
    });
    g.fillTriangle(px - 4, 112, px + 4, 112, px, 119, p.needle); g.drawFastVLine(px, 118, 14, p.needle);
    text(g, FontB612Reg10, in.step ? in.step : "", 14, 156, p.muted, D_ML);
    char vol[10]; snprintf(vol, sizeof(vol), "VOL %d", in.volume); text(g, FontB612Reg10, vol, 306, 156, p.muted, D_MR);
  }

  // ---- 3. Nixie (retro): glowing orange digits in glass tubes ---------------------------------------------------------
  template<class G> static void layoutNixie(G &g, const NuIn &n, const NuPal &p)
  {
    (void)p; const CkIn &in = n.k;
    const uint16_t wood1 = nuRgb(58, 34, 20), wood2 = nuRgb(26, 14, 8), orange = nuRgb(255, 150, 40), orangeDim = nuRgb(120, 56, 16), glass = nuRgb(18, 10, 8);
    for(int y = 0 ; y < 170 ; y++) g.drawFastHLine(0, y, 320, ckMix(wood1, wood2, y * 256 / 170 ));
    for(int y = 3 ; y < 170 ; y += 6) g.drawFastHLine(0, y, 320, ckMix(wood2, wood1, 60));   // wood grain
    char fs[16]; const char *unit = freqStr(in, fs, sizeof(fs));
    const int cnt = (int)strlen(fs), tw = in.mode == 0 ? 50 : 34, th = 84;
    int totalW = 0; for(int i = 0 ; i < cnt ; i++) totalW += fs[i] == '.' ? 14 : tw + 6;
    int x = 160 - totalW / 2; const int y = 28;
    for(int i = 0 ; i < cnt ; i++)
    {
      if(fs[i] == '.') { g.fillSmoothCircle(x + 7, y + th - 14, 6, ckMix(wood2, orange, 90)); g.fillSmoothCircle(x + 7, y + th - 14, 4, orange); x += 14; continue; }
      gradRound(g, x, y, tw, th, 12, nuRgb(40, 26, 22), glass, 40);
      rrAA(g, x, y, tw, th, 12, nuRgb(110, 80, 60));
      for(int k = 0 ; k < 3 ; k++) g.drawFastHLine(x + 6, y + th - 12 + k * 3, tw - 12, nuRgb(70, 50, 40));   // base
      char d[2] = {fs[i], 0};
      g.setFont(&FontB612Big); g.setTextSize(1.45f);
      text(g, FontB612Big, d, x + tw / 2 + 2, y + th / 2 - 2, orangeDim, D_MC);
      text(g, FontB612Big, d, x + tw / 2, y + th / 2 - 4, orange, D_MC);
      g.setTextSize(1.0f);
      x += tw + 6;
    }
    text(g, FontB612Bold12, unit, 160, 120, orange, D_MC);
    // pilot lamps
    text(g, FontB612Bold12, in.modeName, 14, 136, orange, D_ML);
    nameFit(g, p, FontB612Reg10, in.station, 14, 154, 132, orangeDim, n.ms);
    for(int i = 0 ; i < 10 ; i++) { const bool on = (i + 0.5f) / 10 <= n.sN; g.fillSmoothCircle(150 + i * 15, 142, 5, on ? orange : nuRgb(52, 28, 14)); if(on) ringAA(g, 150 + i * 15, 142, 6, ckMix(wood2, orange, 80)); }
    text(g, FontB612Reg10, "SIGNAL", 150, 158, orangeDim, D_ML);
    char nv[12]; snprintf(nv, sizeof(nv), "%d dB", in.snr); text(g, FontB612Bold12, nv, 308, 154, orange, D_MR);
  }

  // ---- 4/5. VFD and green LCD (retro): seven segments ---------------------------------------------------------------
  template<class G> static void layoutSeg(G &g, const NuIn &n, bool vfd, const NuPal &p)
  {
    const CkIn &in = n.k;
    const uint16_t bg = vfd ? nuRgb(4, 10, 12) : nuRgb(150, 168, 112), on = vfd ? nuRgb(110, 255, 230) : nuRgb(26, 34, 18);
    const uint16_t off = vfd ? nuRgb(7, 24, 26) : nuRgb(141, 159, 105), dim = vfd ? nuRgb(40, 120, 120) : nuRgb(70, 84, 52);
    const uint16_t bez = vfd ? nuRgb(30, 34, 38) : nuRgb(46, 50, 46);
    g.fillRect(0, 0, 320, 170, bez);
    g.fillSmoothRoundRect(6, 4, 308, 162, 10, bg);
    rrAA(g, 6, 4, 308, 162, 10, vfd ? nuRgb(20, 60, 60) : nuRgb(110, 126, 80));
    text(g, FontB612Bold12, in.modeName, 16, 14, on, D_ML);
    text(g, FontB612Reg10, in.band ? in.band : "", 56, 14, dim, D_ML);
    text(g, FontB612Reg10, "STEREO", 110, 14, in.stereo ? on : off, D_ML);
    const bool fm = in.mode == 0;
    const int w = fm ? 40 : 26, h = fm ? 74 : 56, th = fm ? 7 : 5;
    const int left = freq7(g, in, 266, 28, w, h, th, fm ? 9 : 8, on, off, vfd ? nuRgb(255, 120, 90) : nuRgb(150, 30, 20));
    (void)left;
    text(g, FontB612Bold12, fm ? "MHz" : "kHz", 306, 28 + h - 8, on, D_MR);
    nameFit(g, p, FontB612Bold12, in.station, 16, 28 + h + 12, 280, on, n.ms);
    // bars
    text(g, FontB612Reg10, "S", 16, 134, dim, D_ML); segBar(g, 30, 128, 140, 11, 20, n.sN, on, off, vfd ? nuRgb(255, 120, 90) : on);
    text(g, FontB612Reg10, "SNR", 180, 134, dim, D_ML); segBar(g, 206, 128, 100, 11, 14, n.nN, on, off);
    char b[24]; snprintf(b, sizeof(b), "STEP %s", in.step ? in.step : ""); text(g, FontB612Reg10, b, 16, 154, dim, D_ML);
    snprintf(b, sizeof(b), "VOL %d", in.volume); text(g, FontB612Reg10, b, 306, 154, dim, D_MR);
  }
  template<class G> static void layoutVfd(G &g, const NuIn &n, const NuPal &p) { layoutSeg(g, n, true, p); }
  template<class G> static void layoutLcd(G &g, const NuIn &n, const NuPal &p) { layoutSeg(g, n, false, p); }

  // ---- 6. Ring (futuristic): tuning ring with the frequency in the middle ------------------------------------------------
  template<class G> static void layoutRing(G &g, const NuIn &n, const NuPal &p)
  {
    const CkIn &in = n.k;
    bgFill(g, p);
    const int cx = 160, cy = 86, R = 80;
    // tuning ring: 240 degrees, tick marks, pointer at the position of the frequency
    const double lo = in.minFreq, hi = in.maxFreq > in.minFreq ? in.maxFreq : in.minFreq + 1;
    const float pos = (float)((in.freq - lo) / (hi - lo));
    g.fillArc(cx, cy, R - 3, R, 150.f, 390.f, ckMix(p.bg1, 0xFFFF, 40));
    g.fillArc(cx, cy, R - 3, R, 150.f, 150.f + 240.f * pos, p.acc);
    for(int i = 0 ; i <= 48 ; i++)
    {
      const float a = (150.f + 240.f * i / 48) * 3.14159f / 180.f, r1 = R - 6, r0 = R - (i % 6 == 0 ? 14 : 10);
      g.drawLine(cx + (int)(r0 * cosf(a)), cy + (int)(r0 * sinf(a)), cx + (int)(r1 * cosf(a)), cy + (int)(r1 * sinf(a)), i % 6 == 0 ? p.txt2 : p.muted);
    }
    const float pa = (150.f + 240.f * pos) * 3.14159f / 180.f;
    g.fillSmoothCircle(cx + (int)((R - 1.5f) * cosf(pa)), cy + (int)((R - 1.5f) * sinf(pa)), 5, p.needle);
    g.fillSmoothCircle(cx + (int)((R - 1.5f) * cosf(pa)), cy + (int)((R - 1.5f) * sinf(pa)), 2, 0xFFFF);
    // inner arcs: S and SNR
    g.fillArc(cx, cy, 58, 62, 150.f, 390.f, ckMix(p.bg1, 0xFFFF, 24));
    g.fillArc(cx, cy, 58, 62, 150.f, 150.f + 240.f * n.sN, p.good);
    char fs[16]; const char *unit = freqStr(in, fs, sizeof(fs));
    g.setFont(&FontB612Big); const int fw = g.textWidth(fs);
    text(g, FontB612Big, fs, cx - fw / 2, cy - 4, p.txt, D_ML);
    text(g, FontB612Bold12, unit, cx, cy + 22, p.acc, D_MC);
    if(in.station && in.station[0]) nameFit(g, p, FontB612Reg10, in.station, cx - 62, cy + 36, 124, p.txt2, n.ms, D_MC);
    chip(g, 8, 8, 40, 18, in.modeName, p.acc);
    char sv[10], nv[12]; nuSText(sv, sizeof(sv), ckStrength(in.rssi, in.mode == 0)); snprintf(nv, sizeof(nv), "%d dB", in.snr);
    text(g, FontB612Reg10, "S", 8, 126, p.muted, D_ML); text(g, FontB612Bold16, sv, 8, 142, p.txt, D_ML);
    text(g, FontB612Reg10, "SNR", 312, 126, p.muted, D_MR); text(g, FontB612Bold16, nv, 312, 142, p.txt, D_MR);
    text(g, FontB612Reg10, in.step ? in.step : "", 56, 17, p.muted, D_ML);
  }

  // ---- 7. Neon (futuristic): chamfered panels, cyan and magenta -------------------------------------------------------------
  template<class G> static void chamfer(G &g, int x, int y, int w, int h, int c, uint16_t line, uint16_t fill)
  {
    g.fillTriangle(x + c, y, x + w, y, x + w, y + h - c, fill); g.fillTriangle(x + c, y, x + w, y + h - c, x, y + h - c, fill);
    g.fillTriangle(x, y + c, x + c, y, x, y + h - c, fill); g.fillTriangle(x, y + h - c, x + w, y + h - c, x + w - c, y + h, fill);
    g.fillRect(x + 1, y + c, w - 1, h - 2 * c + 1, fill);
    g.drawLine(x + c, y, x + w, y, line); g.drawLine(x + w, y, x + w, y + h - c, line); g.drawLine(x + w, y + h - c, x + w - c, y + h, line);
    g.drawLine(x + w - c, y + h, x, y + h, line); g.drawLine(x, y + h, x, y + c, line); g.drawLine(x, y + c, x + c, y, line);
  }
  template<class G> static void layoutNeon(G &g, const NuIn &n, const NuPal &p)
  {
    const CkIn &in = n.k;
    const uint16_t cy_ = nuRgb(60, 240, 255), mg = nuRgb(255, 60, 200), bg = nuRgb(8, 6, 20), pan = nuRgb(16, 10, 36);
    g.fillRect(0, 0, 320, 170, bg);
    for(int y = 0 ; y < 170 ; y += 4) g.drawFastHLine(0, y, 320, nuRgb(14, 10, 30));      // scan lines
    chamfer(g, 6, 22, 308, 66, 12, mg, pan); chamfer(g, 4, 20, 308, 66, 12, cy_, 0);
    char fs[16]; const char *unit = freqStr(in, fs, sizeof(fs));
    g.setFont(&FontB612Big); g.setTextSize(1.15f); const int fw = g.textWidth(fs);
    text(g, FontB612Big, fs, 150 - fw / 2 - 2, 54, mg, D_ML); text(g, FontB612Big, fs, 150 - fw / 2 + 2, 52, cy_, D_ML); text(g, FontB612Big, fs, 150 - fw / 2, 53, 0xFFFF, D_ML);
    g.setTextSize(1.0f);
    digitCursorFont(g, FontB612Big, 1.15f, in.hl, fs, 150 - fw / 2, 53, 0xFFFF, mg, -2);
    text(g, FontB612Bold12, unit, 150 + fw / 2 + 8, 63, cy_, D_ML);
    text(g, FontB612Bold12, in.modeName, 20, 33, cy_, D_ML);
    { char hb[32]; snprintf(hb, sizeof(hb), "STEP %s   VOL %d", in.step ? in.step : "", in.volume); text(g, FontB612Reg10, hb, 20, 79, nuRgb(130, 200, 220), D_ML); }
    if(in.station && in.station[0]) nameFit(g, p, FontB612Bold12, in.station, 20, 10, 196, mg, n.ms);
    // data row
    char sv[10], nv[12]; nuSText(sv, sizeof(sv), ckStrength(in.rssi, in.mode == 0)); snprintf(nv, sizeof(nv), "%d dB", in.snr);
    chamfer(g, 6, 96, 150, 66, 9, cy_, pan);
    text(g, FontB612Reg10, "SIGNAL", 16, 106, nuRgb(130, 200, 220), D_ML); text(g, FontB612Bold16, sv, 148, 106, 0xFFFF, D_MR);
    for(int i = 0 ; i < 18 ; i++) { const bool on = (i + 0.5f) / 18 <= n.sN; g.fillRect(16 + i * 7, 150 - 4 - i * 1, 5, 4 + i, on ? (i > 13 ? mg : cy_) : nuRgb(30, 24, 56)); }
    chamfer(g, 164, 96, 150, 66, 9, mg, pan);
    text(g, FontB612Reg10, "SNR", 174, 106, nuRgb(230, 140, 210), D_ML); text(g, FontB612Bold16, nv, 306, 106, 0xFFFF, D_MR);
    for(int i = 0 ; i < 18 ; i++) { const bool on = (i + 0.5f) / 18 <= n.nN; g.fillRect(174 + i * 7, 150 - 4 - i * 1, 5, 4 + i, on ? (i > 13 ? cy_ : mg) : nuRgb(30, 24, 56)); }
  }

  // ---- 8. Radar (futuristic): sweep with memories as blips ------------------------------------------------------------------
  template<class G> static void layoutRadar(G &g, const NuIn &n, const NuPal &p)
  {
    const CkIn &in = n.k;
    bgFill(g, p);
    const int cx = 84, cy = 88, R = 70;
    g.fillSmoothCircle(cx, cy, R, ckMix(p.bg1, p.acc, 18));
    for(int k = 1 ; k <= 4 ; k++) ringAA(g, cx, cy, R * k / 4, ckMix(p.bg1, p.acc, 70));
    g.drawFastHLine(cx - R, cy, 2 * R + 1, ckMix(p.bg1, p.acc, 60)); g.drawFastVLine(cx, cy - R, 2 * R + 1, ckMix(p.bg1, p.acc, 60));
    // sweep with a fading tail
    const float t = in.t * 1.6f;
    for(int k = 0 ; k < 28 ; k++)
    {
      const float a = t - k * 0.045f; g.drawWedgeLine(cx, cy, cx + R * cosf(a), cy + R * sinf(a), 1.2f, 1.2f, ckMix(ckMix(p.bg1, p.acc, 18), p.acc, 230 - k * 8));
    }
    // memories as blips: angle by position in the band, distance by index
    const double lo = in.minFreq, hi = in.maxFreq > in.minFreq ? in.maxFreq : in.minFreq + 1;
    if(n.mem) for(int i = 0 ; i < n.nMem ; i++)
    {
      const double f = n.mem[i].freq; if(f < lo || f > hi) continue;
      const float a = (float)((f - lo) / (hi - lo)) * 6.2832f - 1.5708f, r = R * (0.45f + 0.12f * (i % 4));
      const bool cur = fabs(f - in.freq) < 1.0;
      g.fillSmoothCircle(cx + (int)(r * cosf(a)), cy + (int)(r * sinf(a)), cur ? 4 : 2, cur ? p.needle : p.acc);
    }
    const float pa = (float)((in.freq - lo) / (hi - lo)) * 6.2832f - 1.5708f;
    g.drawWedgeLine(cx, cy, cx + (R - 2) * cosf(pa), cy + (R - 2) * sinf(pa), 1.6f, 0.4f, p.needle);
    g.fillSmoothCircle(cx, cy, 3, p.txt);
    chip(g, 164, 8, 40, 18, in.modeName, p.acc);
    if(in.station && in.station[0]) nameFit(g, p, FontB612Bold12, in.station, 212, 17, 100, p.txt2, n.ms);
    char fs[16]; const char *unit = freqStr(in, fs, sizeof(fs));
    text(g, FontB612Big, fs, 164, 56, p.txt, D_ML);
    g.setFont(&FontB612Big); const int fw = g.textWidth(fs);
    text(g, FontB612Reg10, unit, 164 + fw + 4, 66, p.acc, D_ML);
    char sv[10], nv[12]; nuSText(sv, sizeof(sv), ckStrength(in.rssi, in.mode == 0)); snprintf(nv, sizeof(nv), "%d dB", in.snr);
    text(g, FontB612Reg10, "S", 164, 96, p.muted, D_ML); segBar(g, 184, 91, 100, 9, 14, n.sN, p.acc, ckMix(p.bg1, 0xFFFF, 30)); text(g, FontB612Bold12, sv, 312, 96, p.txt, D_MR);
    text(g, FontB612Reg10, "SNR", 164, 116, p.muted, D_ML); segBar(g, 190, 111, 94, 9, 14, n.nN, p.good, ckMix(p.bg1, 0xFFFF, 30)); text(g, FontB612Bold12, nv, 312, 116, p.txt, D_MR);
    char b[24]; snprintf(b, sizeof(b), "STEP %s", in.step ? in.step : ""); text(g, FontB612Reg10, b, 164, 146, p.muted, D_ML);
    snprintf(b, sizeof(b), "VOL %d", in.volume); text(g, FontB612Reg10, b, 312, 146, p.muted, D_MR);
  }
}
