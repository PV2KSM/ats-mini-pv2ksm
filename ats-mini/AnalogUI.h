#pragma once
//
// "Analog" main screen (320x170): big amber frequency panel and two analog dials (S-meter and SNR) with
// smoothed needles. Header-only, written only against LovyanGFX, so the same code runs on the radio and in the
// PC preview. Reuses CkIn / CkColors from CockpitUI.h.
//
#include "CockpitUI.h"

namespace an
{
  static const uint16_t cream = 0xF738;          // Dial face (240,228,196)
  static const uint16_t ink   = 0x28E2;          // Dial ink (42,30,18)
  static const uint16_t red   = 0xB964;          // (190,44,34)

  // Angle in degrees from 12 o'clock, clockwise
  static inline void pol(float cx, float cy, float r, float a, float *x, float *y)
  {
    const float t = a * 3.14159265f / 180.f;
    *x = cx + r * sinf(t);
    *y = cy - r * cosf(t);
  }

  template<class G>
  static void arcBand(G &g, float cx, float cy, float r0, float r1, float a0, float a1, uint16_t col)
  {
    g.fillArc((int)lroundf(cx), (int)lroundf(cy), (int)lroundf(r0), (int)lroundf(r1), a0 + 270.f, a1 + 270.f, col);
  }

  template<class G>
  static void tick(G &g, float cx, float cy, float r0, float r1, float a, uint16_t col)
  {
    float x0, y0, x1, y1;
    pol(cx, cy, r0, a, &x0, &y0);
    pol(cx, cy, r1, a, &x1, &y1);
    g.drawLine((int)lroundf(x0), (int)lroundf(y0), (int)lroundf(x1), (int)lroundf(y1), col);
  }

  template<class G>
  static void needle(G &g, float cx, float cy, float r, float a, uint16_t col, uint16_t shadow)
  {
    float x, y;
    pol(cx, cy, r, a, &x, &y);
    g.drawWedgeLine(cx + 1.5f, cy + 1.5f, x + 1.5f, y + 1.5f, 1.8f, 0.4f, shadow);
    g.drawWedgeLine(cx, cy, x, y, 1.8f, 0.4f, col);
  }

  template<class G>
  static void txt(G &g, const char *s, int x, int y, uint16_t col, int datum, const lgfx::IFont *f = &lgfx::fonts::Font0)
  {
    g.setTextDatum((lgfx::textdatum_t)datum);
    g.setTextColor(col);
    g.drawString(s, x, y, f);
  }

  // Position (0..1) of each S scale label along the arc
  static const char *const sLabels[8] = {"1", "3", "5", "7", "9", "+20", "+40", "+60"};
  static const float sPosition[8] = {0.05f, 0.17f, 0.29f, 0.41f, 0.55f, 0.70f, 0.85f, 1.0f};

  template<class G>
  static void dial(G &g, int cx, int top, bool isS, float v, const char *title, const char *value, uint16_t accent, int h = 98)
  {
    const float cy = top + h + 16, R = 96;
    const float a0 = -44, a1 = 44, r1 = R, r0 = R - 5;
    g.setClipRect(cx - 76, top, 152, h);
    g.fillRoundRect(cx - 76, top, 152, h, 8, cream);
    g.drawRoundRect(cx - 76, top, 152, h, 8, 0x7B07);
    if(isS)
    {
      const float split = a0 + (a1 - a0) * 0.55f;
      arcBand(g, cx, cy, r0, r1, a0, split, ink);
      arcBand(g, cx, cy, r0, r1, split, a1, red);
      for(int i = 0 ; i <= 40 ; i++) tick(g, cx, cy, r0 - 3, r0, a0 + (a1 - a0) * i / 40.f, ink);
      for(int i = 0 ; i < 8 ; i++)
      {
        const float a = a0 + (a1 - a0) * sPosition[i];
        const uint16_t col = i >= 4 ? red : ink;
        tick(g, cx, cy, r0 - 6, r1, a, col);
        float x, y;
        pol(cx, cy, r0 - 13, a, &x, &y);
        txt(g, sLabels[i], (int)x, (int)y, col, 4);
      }
    }
    else
    {
      arcBand(g, cx, cy, r0, r1, a0, a1, ink);
      for(int i = 0 ; i <= 8 ; i++)
      {
        const float a = a0 + (a1 - a0) * i / 8.f;
        tick(g, cx, cy, r0 - (i & 1 ? 3 : 6), r1, a, ink);
        if(!(i & 1))
        {
          char b[6];
          snprintf(b, sizeof(b), "%d", i * 5);
          float x, y;
          pol(cx, cy, r0 - 13, a, &x, &y);
          txt(g, b, (int)x, (int)y, ink, 4);
        }
      }
    }
    txt(g, title, cx - 66, top + h - 16, ink, 0, &lgfx::fonts::Font2);
    txt(g, value, cx + 66, top + h - 16, ink, 2, &lgfx::fonts::Font2);
    if(v < 0) v = 0;
    if(v > 1) v = 1;
    needle(g, cx, cy, R + 4, a0 + (a1 - a0) * v, 0x1861, 0xA46C);
    g.fillRect(cx - 12, top + h - 8, 24, 8, 0x28E2);
    g.clearClipRect();
  }
  // Horizontal tuning scale with a red pointer at the current frequency. FM in MHz, other modes in kHz.
  template<class G>
  static void tuneScale(G &g, const CkIn &in, int x0, int x1, int y, int h, uint16_t ink, uint16_t ptr)
  {
    const bool fm = in.mode == 0;
    const double div = fm ? 1e6 : 1e3;
    double lo = in.minFreq / div, hi = in.maxFreq / div, f = in.freq / div;
    if(hi <= lo) hi = lo + 1;
    const int xa = x0 + 8, xb = x1 - 8;
    const double ppu = (xb - xa) / (hi - lo);
    static const double steps[] = {0.1, 0.2, 0.5, 1, 2, 4, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000};
    double major = steps[16];
    const double minPx = fm ? 25 : 34;
    for(double st : steps) if(st * ppu >= minPx) { major = st; break; }
    double minor = (major == 5 || major == 50 || major == 500 || major == 0.5 || major == 5000) ? major / 5 : major / 4;
    if(major == 1 || major == 2 || major == 10 || major == 100 || major == 1000 || major == 0.1) minor = major / 5;
    if(minor * ppu < 3) minor = major / 2;
    for(double v = ceil(lo / minor - 1e-9) * minor ; v <= hi + 1e-9 ; v += minor)
    {
      const int x = xa + (int)lround((v - lo) * ppu);
      const double q = v / major;
      const bool isMaj = fabs(q - round(q)) < 1e-6;
      const int th = isMaj ? 13 : 5;
      g.drawFastVLine(x, y + 2, th, ink);
      if(isMaj)
      {
        g.drawFastVLine(x + 1, y + 2, th, ink);
        if(x > xa + 10 && x < xb - 10)
        {
          char b[12];
          if(major < 1) snprintf(b, sizeof(b), "%.1f", v); else snprintf(b, sizeof(b), "%d", (int)lround(v));
          txt(g, b, x, y + h - 8, ink, 4);
        }
      }
    }
    int px = xa + (int)lround((f - lo) * ppu);
    if(px < xa) px = xa;
    if(px > xb) px = xb;
    g.fillRect(px - 1, y, 2, h, ptr);
    g.fillTriangle(px - 4, y, px + 4, y, px, y + 5, ptr);
  }
}

template<class G>
static void anDraw(G &g, const CkIn &in, const CkColors &c, float sNeedle, float nNeedle)
{
  // Background: slight vertical gradient from the theme background
  for(int y = 0 ; y < 170 ; y++) g.fillRect(0, y, 320, 1, ckMix(c.bg, 0xFFFF, y / 8));
  g.drawRect(0, 0, 320, 170, ckMix(c.bg, c.line, 128));

  // Header: mode, stereo flag, RDS station name, digital frequency
  const int mw = (int)strlen(in.modeName) * 9 + 14;
  ckText(g, FontB612Bold12, in.modeName, 8, 3, c.label);
  int nx = mw;
  if(in.stereo)
  {
    g.drawCircle(mw + 2, 10, 4, c.label);
    g.drawCircle(mw + 7, 10, 4, c.label);
    nx = mw + 18;
  }
  if(in.station && in.station[0]) ckText(g, FontB612Bold12, in.station, nx, 3, c.text);

  const int w = FontB612Bold16.glyph['0' - 32].xAdvance;
  char fs[16];
  int rightEdge;
  const int rightX = 198;
  if(in.mode == 0)
  {
    snprintf(fs, sizeof(fs), "%u.%02u", (unsigned)(in.freq / 1000000), (unsigned)((in.freq % 1000000) / 10000));
    rightEdge = rightX;
    ckText(g, FontB612Bold16, fs, rightX - (int)strlen(fs) * w, 1, c.freq);
    ckText(g, FontB612Reg10, "MHz", rightX + 3, 6, c.unit);
  }
  else
  {
    snprintf(fs, sizeof(fs), "%u.%03u", (unsigned)(in.freq / 1000), (unsigned)(in.freq % 1000));
    rightEdge = rightX - 4 * w;
    ckText(g, FontB612Bold16, fs, rightX - (int)strlen(fs) * w, 1, c.freq);
    ckText(g, FontB612Reg10, "kHz", rightX + 3, 6, c.unit);
  }
  ckDigitUnderline(g, in, c, rightEdge, w, w, w, 18);

  // Tuning glass
  g.fillRoundRect(6, 20, 308, 46, 6, an::cream);
  g.drawRoundRect(6, 20, 308, 46, 6, 0x7B07);
  an::tuneScale(g, in, 6, 314, 22, 42, an::ink, 0xE0E3);

  // Dials
  char sv[8], nv[8];
  snprintf(sv, sizeof(sv), "%d", in.rssi);
  snprintf(nv, sizeof(nv), "%d dB", in.snr);
  g.setClipRect(0, 70, 320, 100);
  an::dial(g, 80, 72, true, sNeedle, "S", sv, c.freq);
  an::dial(g, 240, 72, false, nNeedle, "SNR", nv, c.freq);
  g.clearClipRect();
}

// Needle targets 0..1 from the receiver values
static inline float anSTarget(const CkIn &in) { return ckStrength(in.rssi, in.mode == 0) / 49.0f; }
static inline float anNTarget(const CkIn &in) { float v = in.snr / 40.0f; return v > 1 ? 1 : (v < 0 ? 0 : v); }
