#pragma once
//
// "Cessna" main screen (320x170): general-aviation radio stack look - dark metal frame, red 7-segment LED
// frequency with ghost segments, mode push-button strip, LED bar graphs and a knurled knob.
// Header-only, written only against LovyanGFX (also used by the PC preview).
//
#include "AnalogUI.h"

namespace cs
{
  static const uint16_t metalHi = 0x7BEF, metalMid = 0x39E7, metalLo = 0x2104, black = 0x0000;
  static const uint16_t ledOn = 0xF986, ledOff = 0x2800;        // red LED (255,48,50) and its ghost

  // a b c d e f g
  static const uint8_t segMask[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

  template<class G>
  static void poly(G &g, const float *p, int n, uint16_t col)
  {
    for(int i = 1 ; i + 1 < n ; i++)
      g.fillTriangle((int)lroundf(p[0]), (int)lroundf(p[1]), (int)lroundf(p[2 * i]), (int)lroundf(p[2 * i + 1]),
                     (int)lroundf(p[2 * i + 2]), (int)lroundf(p[2 * i + 3]), col);
  }

  // One italic 7-segment digit. (x, y) is the top-left of the cell, w x h its size, t the segment thickness.
  template<class G>
  static void digit(G &g, int x, int y, int w, int h, int t, int mask, uint16_t on, uint16_t off)
  {
    const float sl = 0.13f;                                      // italic slant
    auto X = [&](float px, float py) { return x + px + sl * (h - py); };
    auto hseg = [&](float cy, uint16_t col)
    {
      const float ht = t * 0.5f;
      float p[12] = {X(t * 0.5f, cy), cy, X(t, cy - ht), cy - ht, X(w - t, cy - ht), cy - ht,
                     X(w - t * 0.5f, cy), cy, X(w - t, cy + ht), cy + ht, X(t, cy + ht), cy + ht};
      float q[12];
      for(int i = 0 ; i < 6 ; i++) { q[2 * i] = p[2 * i]; q[2 * i + 1] = y + p[2 * i + 1]; }
      poly(g, q, 6, col);
    };
    auto vseg = [&](float cx, float cy0, float cy1, uint16_t col)
    {
      const float ht = t * 0.5f;
      const float ys[6] = {cy0, cy0 + ht, cy1 - ht, cy1, cy1 - ht, cy0 + ht};
      const float xs[6] = {cx, cx + ht, cx + ht, cx, cx - ht, cx - ht};
      float q[12];
      for(int i = 0 ; i < 6 ; i++) { q[2 * i] = X(xs[i], ys[i]); q[2 * i + 1] = y + ys[i]; }
      poly(g, q, 6, col);
    };
    const float mid = h / 2.0f, gap = 1.5f;
    hseg(t * 0.5f, (mask & 1) ? on : off);                        // a
    vseg(w - t * 0.5f, gap + t * 0.5f, mid - gap, (mask & 2) ? on : off);       // b
    vseg(w - t * 0.5f, mid + gap, h - gap - t * 0.5f, (mask & 4) ? on : off);   // c
    hseg(h - t * 0.5f, (mask & 8) ? on : off);                    // d
    vseg(t * 0.5f, mid + gap, h - gap - t * 0.5f, (mask & 16) ? on : off);      // e
    vseg(t * 0.5f, gap + t * 0.5f, mid - gap, (mask & 32) ? on : off);          // f
    hseg(mid, (mask & 64) ? on : off);                            // g
  }

  // Number string such as "103.90" or "7205.123" drawn right-aligned ending at xRight; '.' attaches to the previous cell.
  template<class G>
  static void number(G &g, const char *s, int ghostCells, int dotAfter, int xLeft, int y, int w, int h, int t, int pitch, uint16_t on, uint16_t off)
  {
    // Collect digits
    int digs[12], n = 0, dotPos = -1;
    for(const char *p = s ; *p && n < 12 ; p++)
    {
      if(*p == '.') dotPos = n;                                   // dot after digit index dotPos-1
      else digs[n++] = (*p >= '0' && *p <= '9') ? *p - '0' : -1;
    }
    for(int i = 0 ; i < ghostCells ; i++)
    {
      const int src = i - (ghostCells - n);
      const int mask = (src >= 0 && digs[src] >= 0) ? segMask[digs[src]] : 0;
      const int x = xLeft + i * pitch;
      digit(g, x, y, w, h, t, mask, on, off);
      // decimal point after this cell
      if(dotAfter >= 0 && i == dotAfter)
      {
        const bool lit = dotPos >= 0;
        g.fillRect(x + w + 2, y + h - t + 1, t - 1, t - 1, lit ? on : off);
      }
    }
  }

  template<class G>
  static void pill(G &g, int x, int y, int w, const char *label, bool lit, uint16_t litCol)
  {
    g.fillRoundRect(x, y, w, 13, 3, lit ? litCol : 0xC618);
    g.drawRoundRect(x, y, w, 13, 3, 0x52AA);
    g.fillRect(x + 2, y + 11, w - 4, 1, 0x8410);
    g.setFont(&lgfx::fonts::Font0);
    g.setTextDatum((lgfx::textdatum_t)5);
    g.setTextColor(0x0000);
    g.drawString(label, x + w / 2, y + 6);
  }

  template<class G>
  static void screw(G &g, int x, int y)
  {
    g.fillCircle(x, y, 3, 0x8410);
    g.drawCircle(x, y, 3, 0x4208);
    g.drawLine(x - 2, y + 1, x + 2, y - 1, 0x2104);
  }

  template<class G>
  static void knob(G &g, int cx, int cy, int r, float angle)
  {
    g.fillCircle(cx, cy, r + 3, 0x1082);
    for(int i = 0 ; i < 28 ; i++)
    {
      float a = i * 360.0f / 28;
      float x0, y0, x1, y1;
      an::pol(cx, cy, r - 3, a, &x0, &y0);
      an::pol(cx, cy, r + 2, a, &x1, &y1);
      g.drawLine((int)lroundf(x0), (int)lroundf(y0), (int)lroundf(x1), (int)lroundf(y1), (i & 1) ? 0x4A69 : 0x2104);
    }
    g.fillCircle(cx, cy, r - 3, 0x5AEB);
    g.fillCircle(cx, cy, r - 6, 0x2965);
    g.fillCircle(cx, cy, r - 9, 0x3186);
    float px, py;
    an::pol(cx, cy, r - 14, angle, &px, &py);
    g.fillCircle((int)px, (int)py, 2, 0xC618);
  }

  // LED bar graph: n segments, first 'green' ones green, then amber, then red
  template<class G>
  static void bar(G &g, int x, int y, int n, int pitch, int sw, int sh, float v, int greenN, int amberN)
  {
    const int lit = (int)lroundf(v * n);
    for(int i = 0 ; i < n ; i++)
    {
      uint16_t on = i < greenN ? 0x4E6A : (i < greenN + amberN ? 0xFD80 : 0xF986);
      uint16_t off = i < greenN ? 0x0A41 : (i < greenN + amberN ? 0x3100 : 0x2800);
      g.fillRect(x + i * pitch, y, sw, sh, i < lit ? on : off);
    }
  }
}

// variant 0: two LED bars below the display; variant 1: slim tuning scale plus thin bars
template<class G>
static void csDraw(G &g, const CkIn &in, const CkColors &c, float sV, float nV, int variant)
{
  // ---- Metal frame ----
  g.fillSprite(cs::metalMid);
  g.drawRect(0, 0, 320, 170, cs::metalHi);
  g.drawRect(1, 1, 318, 168, 0x6B4D);
  g.drawFastHLine(2, 168, 316, cs::metalLo);
  g.drawFastVLine(318, 2, 166, cs::metalLo);
  cs::screw(g, 5, 5); cs::screw(g, 314, 5); cs::screw(g, 5, 164); cs::screw(g, 314, 164);

  // ---- Mode push-button strip ----
  g.fillRoundRect(12, 4, 192, 19, 3, 0x1082);
  g.drawRoundRect(12, 4, 192, 19, 3, cs::metalHi);
  const char *names[] = {"FM", "LSB", "USB", "AM", "ST", "RDS"};
  const bool lit[] = {in.mode == 0, in.mode == 1, in.mode == 2, in.mode == 3, in.stereo, in.station && in.station[0]};
  for(int i = 0 ; i < 6 ; i++) cs::pill(g, 14 + i * 31, 7, 29, names[i], lit[i], i < 4 ? 0x6FF0 : 0xFFE0);

  // ---- Display ----
  const int dx = 8, dy = 27, dw = 246, dh = 76;
  g.fillRoundRect(dx, dy, dw, dh, 4, cs::black);
  g.drawRoundRect(dx, dy, dw, dh, 4, cs::metalLo);
  g.drawRoundRect(dx - 1, dy - 1, dw + 2, dh + 2, 5, cs::metalHi);
  char fs[16];
  if(in.mode == 0)
  {
    snprintf(fs, sizeof(fs), "%u.%02u", (unsigned)(in.freq / 1000000), (unsigned)((in.freq % 1000000) / 10000));
    cs::number(g, fs, 5, 2, dx + 8, dy + 6, 20, 40, 4, 28, cs::ledOn, cs::ledOff);
    ckText(g, FontB612Reg10, "MHz", dx + 8 + 5 * 28 + 6, dy + 36, 0xC618);
  }
  else
  {
    snprintf(fs, sizeof(fs), "%u.%03u", (unsigned)(in.freq / 1000), (unsigned)(in.freq % 1000));
    cs::number(g, fs, 8, 4, dx + 8, dy + 6, 16, 40, 3, 21, cs::ledOn, cs::ledOff);
    ckText(g, FontB612Reg10, "kHz", dx + 8 + 8 * 21 + 8, dy + 36, 0xC618);
  }
  // Selected-digit underline (frequency edit): slots count from the right-most digit
  if(in.hl != 100 && (in.hl & 0x7F) != 100)
  {
    const int ncell = in.mode == 0 ? 5 : 8, pitch = in.mode == 0 ? 28 : 21, cw = in.mode == 0 ? 20 : 16;
    const int hl = in.hl & 0x7F, slot = hl / 2;
    if(slot < ncell)
    {
      int c0 = ncell - 1 - slot, c1 = c0;
      if((hl & 1) && c0 > 0) c0--;
      const int x0 = dx + 8 + c0 * pitch, x1 = dx + 8 + c1 * pitch + cw;
      g.fillRect(x0, dy + 6 + 40 + 3, x1 - x0 + 3, 3, (in.hl & 0x80) ? 0xFFE0 : 0xC618);
    }
  }
  ckText(g, FontB612Reg10, "USE", dx + 10, dy + 59, 0xC618);
  if(in.station && in.station[0]) ckNameFit(g, FontB612Bold12, in.station, dx + 62, dy + 57, dw - 108, 0xFD80);
  // small RSSI / SNR readouts
  char b[8];
  snprintf(b, sizeof(b), "%02d", in.rssi > 99 ? 99 : in.rssi);
  cs::number(g, b, 2, -1, dx + dw - 40, dy + 8, 11, 20, 3, 17, cs::ledOn, cs::ledOff);
  ckText(g, FontB612Reg10, "RSSI", dx + dw - 40, dy + 31, 0xC618);
  snprintf(b, sizeof(b), "%02d", in.snr > 99 ? 99 : (in.snr < 0 ? 0 : in.snr));
  cs::number(g, b, 2, -1, dx + dw - 40, dy + 42, 11, 20, 3, 17, 0xFD80, 0x1880);
  ckText(g, FontB612Reg10, "SNR", dx + dw - 40, dy + 65, 0xC618);

  // ---- Knob ----
  cs::knob(g, 286, 62, 21, (float)((in.freq / 10000) % 36) * 10.0f);

  // ---- Lower area ----
  if(variant == 0)
  {
    ckText(g, FontB612Bold12, "S", 12, 107, 0xC618);
    cs::bar(g, 40, 108, 32, 7, 5, 10, sV, 18, 0);
    ckText(g, FontB612Bold12, "SNR", 8, 134, 0xC618);
    cs::bar(g, 40, 135, 32, 7, 5, 10, nV, 32, 0);
    for(int i = 0 ; i < 8 ; i++)
    {
      const int x = 40 + (int)(an::sPosition[i] * 218);
      an::txt(g, an::sLabels[i], x + 2, 124, i >= 4 ? 0xF986 : 0xC618, 5);
    }
    for(int i = 0 ; i < 5 ; i++) { char t[6]; snprintf(t, sizeof(t), "%d", i * 10); an::txt(g, t, 42 + i * 55, 152, 0xC618, 5); }
  }
  else
  {
    g.fillRoundRect(8, 108, 246, 32, 4, cs::black);
    g.drawRoundRect(8, 108, 246, 32, 4, cs::metalLo);
    an::tuneScale(g, in, 8, 254, 109, 30, 0xC618, 0xF986);
    ckText(g, FontB612Bold12, "S", 14, 145, 0xC618);
    cs::bar(g, 30, 146, 36, 5, 4, 7, sV, 21, 7);
    ckText(g, FontB612Bold12, "N", 14, 156, 0xC618);
    cs::bar(g, 30, 157, 36, 5, 4, 5, nV, 36, 0);
  }
}
