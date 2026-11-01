#pragma once
//
// PV2KSM: Scan screen for the new layouts. Replaces the main screen while a scan is shown (currentCmd == CMD_SCAN):
// frequency, RSSI and SNR on top, a large graph below. One drawing for every layout, in the layout's own colours.
//
#include "NewUI.h"
#include "NewUI2.h"

float scanGetRSSI(uint32_t freq);
float scanGetSNR(uint32_t freq);

namespace nu
{
  enum { SCS_ANALOG = 0, SCS_SMETER, SCS_VINT, SCS_CLOCK, SCS_NAME, SCS_THIN, SCS_VFD, SCS_LCD, SCS_NEON, SCS_COCKPIT, SCS_CREAM, SCS_AVION };
  static_assert(SCS_COCKPIT == 9 && SCS_CREAM == 10 && SCS_AVION == 11, "keep in sync with Draw.h");

  template<class G> static void layoutScan(G &g, const NuIn &n, const NuPal &p, int style)
  {
    const CkIn &in = n.k;
    // Each layout has its own Scan colours (background, curves, text)
    uint16_t bg1, bg2, acc, snr, txt, mut, grid, ptr;
    const bool seg = style == SCS_VFD || style == SCS_LCD || style == SCS_COCKPIT;
    switch(style)
    {
      case SCS_SMETER: bg1 = nuRgb(16, 24, 32); bg2 = nuRgb(8, 12, 18); acc = nuRgb(91, 227, 125); snr = nuRgb(255, 211, 74); txt = 0xFFFF; mut = nuRgb(127, 160, 168); grid = nuRgb(39, 64, 74); ptr = nuRgb(255, 77, 77); break;
      case SCS_VINT:   bg1 = nuRgb(232, 217, 176); bg2 = nuRgb(205, 185, 138); acc = nuRgb(122, 42, 16); snr = nuRgb(31, 90, 58); txt = nuRgb(46, 28, 12); mut = nuRgb(110, 85, 48); grid = nuRgb(165, 143, 95); ptr = nuRgb(192, 24, 24); break;
      case SCS_CLOCK:  bg1 = nuRgb(26, 16, 51); bg2 = nuRgb(11, 6, 24); acc = nuRgb(179, 136, 255); snr = nuRgb(94, 242, 193); txt = 0xFFFF; mut = nuRgb(154, 140, 200); grid = nuRgb(53, 40, 94); ptr = nuRgb(255, 77, 109); break;
      case SCS_NAME:   bg1 = nuRgb(18, 38, 31); bg2 = nuRgb(8, 20, 15); acc = nuRgb(61, 220, 151); snr = nuRgb(245, 197, 66); txt = 0xFFFF; mut = nuRgb(128, 184, 162); grid = nuRgb(36, 73, 59); ptr = nuRgb(255, 77, 77); break;
      case SCS_THIN:   bg1 = bg2 = 0x0000; acc = 0xFFFF; snr = nuRgb(136, 136, 136); txt = 0xFFFF; mut = nuRgb(119, 119, 119); grid = nuRgb(51, 51, 51); ptr = nuRgb(255, 77, 77); break;
      case SCS_VFD:    bg1 = bg2 = nuRgb(4, 10, 12); acc = nuRgb(110, 255, 230); snr = nuRgb(255, 200, 70); txt = acc; mut = nuRgb(40, 120, 120); grid = nuRgb(20, 60, 60); ptr = nuRgb(255, 120, 90); break;
      case SCS_LCD:    bg1 = bg2 = nuRgb(150, 168, 112); acc = snr = txt = nuRgb(26, 34, 18); mut = nuRgb(70, 84, 52); grid = nuRgb(120, 136, 90); ptr = nuRgb(150, 30, 20); break;
      case SCS_COCKPIT: bg1 = 0x0000; bg2 = nuRgb(8, 8, 8); acc = nuRgb(255, 48, 50); snr = nuRgb(255, 176, 32); txt = acc; mut = nuRgb(160, 42, 42); grid = nuRgb(74, 20, 20); ptr = 0xFFFF; break;
      case SCS_CREAM:  bg1 = nuRgb(240, 228, 196); bg2 = nuRgb(226, 211, 168); acc = nuRgb(42, 30, 18); snr = nuRgb(190, 44, 34); txt = acc; mut = nuRgb(110, 90, 58); grid = nuRgb(180, 160, 112); ptr = snr; break;
      case SCS_AVION:  bg1 = nuRgb(27, 32, 38); bg2 = nuRgb(16, 20, 24); acc = nuRgb(91, 227, 125); snr = 0xFFFF; txt = nuRgb(242, 245, 248); mut = nuRgb(138, 151, 166); grid = nuRgb(52, 64, 74); ptr = nuRgb(255, 159, 28); break;
      case SCS_NEON:   bg1 = nuRgb(18, 10, 36); bg2 = nuRgb(7, 4, 15); acc = nuRgb(0, 229, 255); snr = nuRgb(255, 61, 242); txt = 0xFFFF; mut = nuRgb(138, 127, 214); grid = nuRgb(42, 42, 90); ptr = nuRgb(255, 77, 109); break;
      default:         bg1 = nuRgb(14, 32, 54); bg2 = nuRgb(7, 17, 29); acc = nuRgb(255, 159, 28); snr = nuRgb(76, 208, 255); txt = 0xFFFF; mut = nuRgb(127, 152, 181); grid = nuRgb(38, 64, 94); ptr = nuRgb(255, 77, 77); break;
    }
    for(int y = 0 ; y < 170 ; y++) g.drawFastHLine(0, y, 320, ckMix(bg1, bg2, y * 256 / 170));
    if(style == SCS_COCKPIT)
    {
      for(int i = 0 ; i < 5 ; i++) { const uint16_t m = ckMix(nuRgb(123, 125, 131), nuRgb(33, 35, 40), 255 * (i < 3 ? i : 4 - i) / 4 * 0 + i * 50); g.drawRect(i, i, 320 - 2 * i, 170 - 2 * i, m); }
    }
    else if(style == SCS_CREAM) { g.drawRect(3, 3, 314, 164, acc); g.drawRect(6, 6, 308, 158, ckMix(bg1, acc, 120)); }
    else if(style == SCS_AVION) { g.drawRoundRect(1, 1, 318, 168, 10, nuRgb(89, 99, 111)); g.drawRoundRect(2, 2, 316, 166, 9, nuRgb(89, 99, 111)); }
    else g.drawFastHLine(0, 0, 320, acc);

    // ---- Header: frequency, mode and SCAN chips, RSSI and SNR ----
    char fs[16]; const char *unit = freqStr(in, fs, sizeof(fs));
    const lgfx::GFXfont *ff = &FontB612Big;
    g.setFont(ff); g.setTextSize(0.8f);
    if(g.textWidth(fs) > 138) { g.setTextSize(1.0f); ff = &FontB612Bold16; }
    g.setFont(ff);
    const int fw = g.textWidth(fs);
    g.setTextDatum((lgfx::textdatum_t)D_ML); g.setTextColor(txt); g.drawString(fs, 8, 21);
    g.setTextSize(1.0f);
    text(g, FontB612Reg10, unit, 8 + fw + 3, 25, mut, D_ML);
    const int cx0 = 8 + fw + 3 + 26;
    chip(g, cx0, 11, 28, 15, in.modeName, acc);
    chip(g, cx0 + 32, 11, 38, 15, "SCAN", snr);
    char b[24];
    text(g, FontB612Reg10, "RSSI", 238, 25, mut, D_ML); snprintf(b, sizeof(b), "%d", in.rssi); text(g, FontB612Bold12, b, 284, 25, acc, D_MR);
    text(g, FontB612Reg10, "dBuV", 286, 25, mut, D_ML);
    text(g, FontB612Reg10, "SNR", 238, 39, mut, D_ML); snprintf(b, sizeof(b), "%d", in.snr); text(g, FontB612Bold12, b, 284, 39, snr, D_MR);
    text(g, FontB612Reg10, "dB", 286, 39, mut, D_ML);

    // Row under the frequency: S-meter bar, station name or the time, depending on the layout
    if(style == SCS_SMETER)
    {
      char sv[10]; nuSText(sv, sizeof(sv), ckStrength(in.rssi, in.mode == 0));
      segBar(g, 8, 33, 150, 7, 24, n.sN, acc, ckMix(bg1, 0xFFFF, 30));
      text(g, FontB612Bold12, sv, 166, 36, txt, D_ML);
    }
    else if(style == SCS_NAME && in.station && in.station[0]) nameFit(g, p, FontB612Bold12, in.station, 8, 36, 220, acc, n.ms);
    else if(style == SCS_CLOCK && in.haveTime) { snprintf(b, sizeof(b), "%02d:%02d", in.hh, in.mm); text(g, FontB612Bold12, b, 8, 36, mut, D_ML); }

    // ---- Graph ----
    const int X0 = 6, X1 = 314, Y0 = 46, Y1 = 138, W = X1 - X0, H = Y1 - Y0;
    const float cell = W / 40.0f;
    const uint32_t step = in.mode == 0 ? 100000 : 10000;
    const Band *band = getCurrentBand();
    const uint32_t minF = band->minimumFreq, maxF = band->maximumFreq;
    const int64_t base = ((int64_t)(in.freq / step) - 20) * (int64_t)step;
    const float frac = (float)(in.freq % step) / step;
    float vr[42], vs[42]; bool ok[42];
    float best = -1; int64_t bestF = 0;
    for(int i = 0 ; i < 42 ; i++)
    {
      const int64_t f = base + (int64_t)i * step;
      ok[i] = f >= (int64_t)minF && f <= (int64_t)maxF;
      vr[i] = ok[i] ? scanGetRSSI((uint32_t)f) : 0; vs[i] = ok[i] ? scanGetSNR((uint32_t)f) : 0;
      if(vr[i] > 1) vr[i] = 1;
      if(vs[i] > 1) vs[i] = 1;
      if(ok[i] && vr[i] > best && i >= 1 && i <= 40) { best = vr[i]; bestF = f; }
    }
    auto px = [&](int i) { return X0 + (int)(i * cell - frac * cell + 0.5f); };
    auto py = [&](float v) { return Y1 - (int)(v * H); };

    g.fillRect(X0, Y0, W, H, ckMix(bg1, 0x0000, style == SCS_LCD ? 20 : 60));
    if(style != SCS_THIN)
    {
      for(int k = 0 ; k <= 4 ; k++) { const int y = Y1 - H * k / 4; for(int x = X0 ; x < X1 ; x += 3) g.drawPixel(x, y, grid); }
    }
    g.setClipRect(X0, Y0 - 1, W, H + 2);
    // Vertical marks every 5 steps and their labels
    g.setFont(&FontB612Reg10);
    for(int i = 0 ; i < 42 ; i++)
    {
      const int64_t f = base + (int64_t)i * step;
      if(f % (5 * (int64_t)step) != 0 || f < (int64_t)minF || f > (int64_t)maxF) continue;
      const int x = px(i);
      if(style != SCS_THIN) for(int y = Y0 ; y < Y1 ; y += 3) g.drawPixel(x, y, grid);
    }
    if(seg)
    {
      const int rows = H / 6;
      for(int i = 0 ; i < 41 ; i++)
      {
        if(!ok[i]) continue;
        const int x = px(i) + 1, w = (int)cell - 2;
        const int nseg = (int)(vr[i] * rows + 0.5f);
        for(int j = 0 ; j < nseg ; j++) g.fillRect(x, Y1 - (j + 1) * 6 + 1, w, 5, acc);
        g.fillRect(x, py(vs[i]) - 1, w, 2, snr);
      }
    }
    else
    {
      // Area under RSSI, then the two lines (interpolated per pixel column)
      for(int i = 0 ; i < 41 ; i++)
      {
        if(!ok[i] || !ok[i + 1]) continue;
        const int xa = px(i), xb = px(i + 1);
        const int a = py(vr[i]), c = py(vr[i + 1]);
        if(style != SCS_THIN)
          for(int x = xa ; x < xb ; x++) { const int y = a + (c - a) * (x - xa) / (xb - xa > 0 ? xb - xa : 1); g.drawFastVLine(x, y, Y1 - y, ckMix(bg1, acc, 40)); }
        if(style == SCS_NEON)
        {
          const uint16_t gl = ckMix(bg1, acc, 90), gs = ckMix(bg1, snr, 90);
          g.drawLine(xa, a + 1, xb, c + 1, gl); g.drawLine(xa, a - 1, xb, c - 1, gl);
          g.drawLine(xa, py(vs[i]) + 1, xb, py(vs[i + 1]) + 1, gs); g.drawLine(xa, py(vs[i]) - 1, xb, py(vs[i + 1]) - 1, gs);
        }
        g.drawLine(xa, py(vs[i]), xb, py(vs[i + 1]), snr);
        g.drawLine(xa, a, xb, c, acc);
        if(style != SCS_THIN) g.drawLine(xa, a - 1, xb, c - 1, acc);
      }
    }
    g.clearClipRect();
    // Pointer on the current frequency
    const int cxp = px(20);
    g.drawFastVLine(cxp, Y0 - 2, H + 2, ptr);
    g.fillTriangle(cxp - 4, Y0 - 6, cxp + 4, Y0 - 6, cxp, Y0, ptr);

    // Scale labels
    for(int i = 0 ; i < 42 ; i++)
    {
      const int64_t f = base + (int64_t)i * step;
      if(f % (5 * (int64_t)step) != 0 || f < (int64_t)minF || f > (int64_t)maxF) continue;
      const int x = px(i);
      if(x < X0 + 12 || x > X1 - 12) continue;
      if(in.mode == 0) snprintf(b, sizeof(b), "%u.%u", (unsigned)(f / 1000000), (unsigned)((f % 1000000) / 100000));
      else snprintf(b, sizeof(b), "%u", (unsigned)(f / 1000));
      text(g, FontB612Reg10, b, x, Y1 + 9, mut, D_MC);
    }

    // Legend and peak
    g.fillRect(8, 156, 8, 3, acc); text(g, FontB612Reg10, "RSSI", 19, 158, mut, D_ML);
    g.fillRect(52, 156, 8, 3, snr); text(g, FontB612Reg10, "SNR", 63, 158, mut, D_ML);
    if(best > 0)
    {
      if(in.mode == 0) snprintf(b, sizeof(b), uiLang == LANG_PT ? "Pico %u.%02u MHz" : uiLang == LANG_ES ? "Pico %u.%02u MHz" : "Peak %u.%02u MHz", (unsigned)(bestF / 1000000), (unsigned)((bestF % 1000000) / 10000));
      else snprintf(b, sizeof(b), uiLang == LANG_PT ? "Pico %u.%03u kHz" : uiLang == LANG_ES ? "Pico %u.%03u kHz" : "Peak %u.%03u kHz", (unsigned)(bestF / 1000), (unsigned)(bestF % 1000));
      text(g, FontB612Reg10, b, 312, 158, mut, D_MR);
    }
  }
}
