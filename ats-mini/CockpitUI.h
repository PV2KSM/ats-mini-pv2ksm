#pragma once
//
// "Cockpit" main screen (320x170): FMC/CDU inspired, B612 Mono bitmap font, animated weather card on the
// left, large frequency, tuning parameters and a segmented S-meter / SNR bar with peak hold.
// Header-only and written only against LovyanGFX so the same code runs on the radio and in a PC preview.
//
#include <LovyanGFX.hpp>
#include "I18n.h"
#ifndef WX_DATUM
#define WX_DATUM(d) ((lgfx::textdatum_t)(d))
#endif
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "WeatherUI.h"
#include "FontB612.h"

struct CkColors
{
  uint16_t bg, text, muted, warn;       // Basic palette
  uint16_t label;                       // Cyan-like small labels
  uint16_t freq, unit, hl, hlSel;       // Frequency digits and their selector
  uint16_t station;                     // Station name / radio text
  uint16_t bar, barPlus, barEmpty;      // S-meter
  uint16_t snr;                         // SNR bar
  uint16_t line, pointer;               // Separators, band position marker
  uint16_t box;                         // Panel background (Analog layout)
};

struct CkIn
{
  uint32_t freq;                        // Hz
  int mode;                             // 0 FM, 1 LSB, 2 USB, 3 AM
  const char *band;                     // "VHF"
  const char *modeName;                 // "FM"
  char vfo;                             // 'A' or 'B'
  const char *station;                  // Station name, may be ""
  const char *radioText;                // First radio text line, may be ""
  int rssi, snr;
  bool stereo;
  const char *step;                     // "100k"
  const char *bw;                       // "Auto"
  const char *agcLabel;                 // "AGC" or "ATT"
  const char *agc;                      // "On" or the attenuation
  int volume;
  bool haveTime;
  int hh, mm;
  uint32_t minFreq, maxFreq;            // Band limits (Hz)
  int hl;                               // Selected frequency digit (0x80 = selector active), 100 = none
  // Weather card
  bool haveWx;
  WxData wx;
  const char *place;                    // Short place label, e.g. "SJK"
  const char *wxStatus;                 // Shown when there is no data ("Sem Wi-Fi", ...)
  float t;                              // Seconds, for animations
  uint32_t stby;                        // Other VFO frequency in Hz (737 style layout)
  uint8_t squelch;                      // Squelch threshold (0 = off)
  bool sqlSnr;                          // Squelch works on SNR instead of RSSI
  bool muted;                           // Main audio mute
};

static inline uint16_t ckMix(uint16_t a, uint16_t b, int k256)   // RGB565 blend: a + (b-a)*k/256
{
  int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
  int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  int r = ar + ((br - ar) * k256 >> 8), g = ag + ((bg - ag) * k256 >> 8), bl = ab + ((bb - ab) * k256 >> 8);
  return (uint16_t)((r << 11) | (g << 5) | bl);
}

// Signal strength (0..49) from RSSI, same curve as the S-Meter layout
static inline int ckStrength(int rssi, bool fm)
{
  static const int amT[] = {1, 2, 3, 4, 10, 16, 22, 28, 34, 44, 54, 64, 74, 84, 94, 95, 96};
  static const int amV[] = {1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46, 49};
  static const int fmT[] = {1, 2, 8, 14, 24, 34, 44, 54, 64, 74, 76, 77};
  static const int fmV[] = {1, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46, 49};
  const int *th = fm ? fmT : amT, *va = fm ? fmV : amV;
  const int n = fm ? (int)(sizeof(fmT) / sizeof(fmT[0])) : (int)(sizeof(amT) / sizeof(amT[0]));
  for(int i = 0 ; i < n ; i++)
  {
    if(rssi <= th[i])
    {
      if(!i) return va[i];
      int iv = th[i] - th[i - 1];
      if(!iv) return va[i];
      float pos = (float)(rssi - th[i - 1]) / iv;
      return (int)(va[i - 1] + pos * (va[i] - va[i - 1]) + 0.5f);
    }
  }
  return va[n - 1];
}

static inline const char *ckWxShort(int code)
{
  switch(wxKind(code))
  {
    case WX_CLEAR:  return uiText(code == 1 ? "MOSTLY CLEAR" : "CLEAR SKY");
    case WX_PARTLY: return uiText("PARTLY CLOUDY");
    case WX_CLOUDY: return uiText("OVERCAST");
    case WX_FOG:    return uiText("FOG");
    case WX_RAIN:   return (code >= 80 || code == 65 || code == 67) ? uiText("SHOWERS") : uiText("RAIN");
    case WX_SNOW:   return uiText("SNOW");
    default:        return uiText("STORM");
  }
}

template<class G>
static void ckText(G &g, const lgfx::GFXfont &f, const char *s, int x, int y, uint16_t col, int datum = 0)
{
  g.setFont(&f);
  g.setTextDatum((lgfx::textdatum_t)datum);
  g.setTextColor(col);
  g.drawString(s, x, y);
}

// Station / long name inside a fixed width: drawn as is when it fits (datum 0 = left, 1 = centred), otherwise scrolled
// (marquee) so it never runs over the neighbouring elements. y is the top of the text.
template<class G>
static void ckNameFit(G &g, const lgfx::GFXfont &f, const char *s, int x, int y, int w, uint16_t col, int datum = 0)
{
  if(w < 8 || !s || !*s) return;
  g.setFont(&f);
  const int tw = g.textWidth(s), fh = g.fontHeight();
  if(tw <= w) { ckText(g, f, s, datum == 1 ? x + w / 2 : x, y, col, datum); return; }
  const int gap = 28, off = (int)((lgfx::millis() / 40) % (uint32_t)(tw + gap));
  g.setClipRect(x, y - 1, w, fh + 2);
  ckText(g, f, s, x - off, y, col, 0);
  ckText(g, f, s, x - off + tw + gap, y, col, 0);
  g.clearClipRect();
}

// Left card: animated weather, or a muted placeholder while there is no data
template<class G>
static void ckWeatherCard(G &g, const CkIn &in, const CkColors &c, int x, int y, int w, int h)
{
  g.fillRect(x, y, w, h, c.bg);
  if(!in.haveWx || !in.wx.valid)
  {
    g.drawRect(x, y, w, h, c.line);
    ckText(g, FontB612Reg10, uiText("WEATHER"), x + 5, y + 4, c.label);
    // Idle cloud that keeps drifting so the card does not look frozen
    WxBg bg = wxBackground(WX_CLOUDY, false, h);
    wxIcon(g, WX_CLOUDY, false, x + w / 2, y + 40, 0.8f, in.t, bg);
    ckText(g, FontB612Reg10, in.wxStatus && in.wxStatus[0] ? in.wxStatus : uiText("SEARCHING..."), x + w / 2, y + 72, c.muted, 1);
    ckText(g, FontB612Reg10, "ATS-MINI", x + w / 2, y + 88, c.line, 1);
    return;
  }
  const WxData &d = in.wx;
  const int kind = wxKind(d.code);
  const bool day = d.isDay != 0;
  WxBg bg = wxBackground(kind, day, h);

  // Vertical gradient background, slightly darkened so the cockpit palette still dominates
  for(int i = 0 ; i < h ; i++) g.drawFastHLine(x, y + i, w, ckMix(bg.at(i), c.bg, 90));
  g.drawRect(x, y, w, h, c.line);
  // Corner ticks (CDU style)
  g.drawFastHLine(x, y, 6, c.label); g.drawFastVLine(x, y, 6, c.label);
  g.drawFastHLine(x + w - 6, y + h - 1, 6, c.label); g.drawFastVLine(x + w - 1, y + h - 6, 6, c.label);

  ckText(g, FontB612Reg10, in.place && in.place[0] ? in.place : uiText("WEATHER"), x + 5, y + 4, c.label);

  wxIcon(g, kind, day, x + 33, y + 34, 0.7f, in.t, bg);

  char b[24];
  snprintf(b, sizeof(b), "%d`", d.temp);
  ckText(g, FontB612Bold16, b, x + w - 5, y + 20, 0xFFFF, 2);   // TR datum

  ckText(g, FontB612Bold12, ckWxShort(d.code), x + 5, y + 62, 0xFFFF);
  snprintf(b, sizeof(b), "MAX%d` MIN%d`", d.days[0].tmax, d.days[0].tmin);
  ckText(g, FontB612Reg10, b, x + 5, y + 75, c.text);
  snprintf(b, sizeof(b), uiLang == LANG_PT ? "UMID%d%% V%d" : uiLang == LANG_ES ? "HUM%d%% V%d" : "HUM%d%% W%d", d.hum, d.wind);
  ckText(g, FontB612Reg10, b, x + 5, y + 86, c.muted);
}

// Underline under the selected frequency digit.
// FM: slots (right to left) .01 .10 1 10 100 around the decimal point; AM/SSB: .001 .010 .100 then 1 10 100 ...
template<class G>
static void ckDigitUnderline(G &g, const CkIn &in, const CkColors &c, int rightEdge, int bigW, int smallW, int dotW, int y)
{
  if(in.hl == 100 || (in.hl & 0x7F) == 100) return;
  const int hl = in.hl & 0x7F;
  const bool sel = (in.hl & 0x80) != 0;
  const int slot = hl / 2;
  const bool half = (hl & 1) != 0;       // 5 x step: the underline also covers the next digit to the left
  int x1, x0, w;
  auto slotRight = [&](int s, int *wd) -> int
  {
    if(in.mode == 0)
    {
      if(s < 2) { *wd = bigW; return rightEdge - s * bigW; }
      *wd = bigW; return rightEdge - 2 * bigW - dotW - (s - 2) * bigW;
    }
    if(s < 3) { *wd = smallW; return rightEdge + 2 + (3 - s) * smallW + smallW; }
    *wd = bigW; return rightEdge - (s - 3) * bigW;
  };
  if(slot > (in.mode == 0 ? 4 : 8)) return;
  x1 = slotRight(slot, &w);
  x0 = x1 - w;
  if(half) { int w2; int r2 = slotRight(slot + 1, &w2); x0 = r2 - w2; }
  if(x0 > x1) { int t = x0; x0 = x1; x1 = t; }
  if(sel)
  {
    g.fillRoundRect(x0 + 1, y - 1, x1 - x0 - 2, 3, 1, c.hlSel);
    g.fillTriangle(x0, y, x0 + 3, y - 3, x0 + 3, y + 3, c.hlSel);
    g.fillTriangle(x1 - 1, y, x1 - 4, y - 3, x1 - 4, y + 3, c.hlSel);
  }
  else g.fillRoundRect(x0, y - 1, x1 - x0, 3, 1, c.hl);
}

//
// Whole screen. Left-hand side is a weather card; callers draw the side menu instead when a command is active
// by passing leftCard = false (the caller then paints its own panel).
//
template<class G>
static void ckDraw(G &g, const CkIn &in, const CkColors &c, bool leftCard)
{
  g.fillSprite(c.bg);

  // ---- Header: band, mode, VFO, stereo, clock ----
  ckText(g, FontB612Bold16, in.band, 4, 1, c.label);
  int bw = (int)strlen(in.band) * 10 + 8;
  g.setFont(&FontB612Reg12);
  int mw = (int)strlen(in.modeName) * 7 + 8;
  g.drawRoundRect(bw, 2, mw, 14, 3, c.line);
  ckText(g, FontB612Reg12, in.modeName, bw + 4, 4, c.text);
  char vb[2] = {in.vfo, 0};
  g.drawRoundRect(bw + mw + 4, 2, 14, 14, 3, c.line);
  ckText(g, FontB612Reg12, vb, bw + mw + 7, 4, c.label);
  if(in.stereo)
  {
    g.drawCircle(bw + mw + 28, 9, 5, c.label);
    g.drawCircle(bw + mw + 34, 9, 5, c.label);
  }
  if(in.haveTime)
  {
    char tb[8];
    snprintf(tb, sizeof(tb), "%02d:%02d", in.hh, in.mm);
    ckText(g, FontB612Bold12, tb, 150, 4, c.muted);
  }
  g.drawFastHLine(0, 19, 320, c.line);

  // ---- Left card ----
  if(leftCard) ckWeatherCard(g, in, c, 2, 22, 100, 98);

  // ---- Frequency ----
  const int fx = 110;                        // Left edge of the frequency block
  const int bigW = FontB612Big.glyph[0].xAdvance;                 // Monospace advance
  const int smallW = FontB612Bold16.glyph['0' - 32].xAdvance;
  const int dotW = 12;                       // Decimal point cell, narrower than a digit
  const int fy = 24;
  int rightEdge;
  if(in.mode == 0)
  {
    char ip[8], fp[4];
    snprintf(ip, sizeof(ip), "%u", (unsigned)(in.freq / 1000000));
    snprintf(fp, sizeof(fp), "%02u", (unsigned)((in.freq % 1000000) / 10000));
    const int il = (int)strlen(ip);
    rightEdge = fx + 5 * bigW + dotW;
    const int fracX = rightEdge - 2 * bigW;
    ckText(g, FontB612Big, ip, fracX - dotW - il * bigW, fy, c.freq);
    ckText(g, FontB612Big, ".", fracX - dotW + 1, fy, c.freq);
    ckText(g, FontB612Big, fp, fracX, fy, c.freq);
    ckText(g, FontB612Bold16, "MHz", rightEdge + 4, fy + 18, c.unit);
  }
  else
  {
    char ip[10], fp[6];
    snprintf(ip, sizeof(ip), "%u", (unsigned)(in.freq / 1000));
    snprintf(fp, sizeof(fp), ".%03u", (unsigned)(in.freq % 1000));
    const int il = (int)strlen(ip);
    rightEdge = fx + 5 * bigW;
    ckText(g, FontB612Big, ip, rightEdge - il * bigW, fy, c.freq);
    ckText(g, FontB612Bold16, "kHz", rightEdge + 4, fy + 2, c.unit);
    ckText(g, FontB612Bold16, fp, rightEdge + 4, fy + 22, c.freq);
  }
  ckDigitUnderline(g, in, c, rightEdge, bigW, smallW, dotW, fy + 41);

  // ---- Station name and radio text ----
  const int sy = 68;
  if(in.station && in.station[0]) ckText(g, FontB612Bold16, in.station, fx, sy, c.station);
  if(in.radioText && in.radioText[0])
  {
    char rt[30];
    snprintf(rt, sizeof(rt), "%s", in.radioText);
    ckText(g, FontB612Reg12, rt, fx, sy + 19, c.muted);
  }

  // ---- Tuning parameters (label over value) ----
  {
    const int cy = 100, cw = 52;
    char vol[8];
    if(in.volume < 0) snprintf(vol, sizeof(vol), "%s", uiText("MUTED")); else snprintf(vol, sizeof(vol), "%d", in.volume);
    const char *labels[4] = {"STEP", "BW", in.agcLabel ? in.agcLabel : "AGC", "VOL"};
    const char *vals[4] = {in.step, in.bw, in.agc, vol};
    for(int i = 0 ; i < 4 ; i++)
    {
      ckText(g, FontB612Reg10, labels[i], fx + i * cw, cy, c.label);
      char v[9];
      snprintf(v, sizeof(v), "%s", vals[i]);
      for(char *p = v ; *p ; p++) if(*p >= 'a' && *p <= 'z') *p -= 32;
      ckText(g, FontB612Bold12, v, fx + i * cw, cy + 10, c.text);
    }
  }

  // ---- Signal section ----
  g.drawFastHLine(0, 124, 320, c.line);
  const int segs = 42, pitch = 6, sw = 4;
  const int bx0 = 54;
  const int strength = ckStrength(in.rssi, in.mode == 0);                  // 0..49
  const int lit = strength * segs / 49;
  static float peak = 0;
  if(lit > peak) peak = (float)lit; else peak -= 0.18f;
  if(peak < lit) peak = (float)lit;

  ckText(g, FontB612Bold12, "S", 4, 129, c.label);
  char vb2[8];
  snprintf(vb2, sizeof(vb2), "%d", in.rssi);
  ckText(g, FontB612Bold16, vb2, 50, 127, c.text, 2);                      // TR
  const int sy0 = 128, shh = 11;
  for(int i = 0 ; i < segs ; i++)
  {
    uint16_t col;
    if(i < lit)
    {
      int s9 = segs * 28 / 49;                                            // S9 boundary
      if(i < s9) col = ckMix(c.bar, 0xFFE0, i * 150 / s9);                 // Green -> yellow
      else col = ckMix(0xFD20, c.barPlus, (i - s9) * 256 / (segs - s9));   // Orange -> red
    }
    else col = c.barEmpty;
    g.fillRoundRect(bx0 + i * pitch, sy0, sw, shh, 1, col);
  }
  // Peak hold marker
  {
    int pi = (int)peak;
    if(pi >= segs) pi = segs - 1;
    if(pi > lit && pi >= 0)
    {
      int px = bx0 + pi * pitch + sw / 2;
      g.fillTriangle(px - 3, sy0 - 4, px + 3, sy0 - 4, px, sy0 - 1, c.text);
    }
  }

  // SNR (thin bar)
  ckText(g, FontB612Bold12, "N", 4, 145, c.label);
  snprintf(vb2, sizeof(vb2), "%d", in.snr);
  ckText(g, FontB612Bold12, vb2, 50, 145, c.text, 2);
  const int snrLit = in.snr * segs / 64 > segs ? segs : in.snr * segs / 64;
  for(int i = 0 ; i < segs ; i++)
    g.fillRect(bx0 + i * pitch, 148, sw, 4, i < snrLit ? c.snr : c.barEmpty);

  // Scale: S1 .. S9, +20, +40, +60
  {
    static const struct { const char *t; int seg49; } sc[] = {
      {"1", 3}, {"3", 9}, {"5", 15}, {"7", 22}, {"9", 28}, {"+20", 35}, {"+40", 42}, {"+60", 49}};
    for(const auto &s : sc)
    {
      int px = bx0 + (s.seg49 * segs / 49) * pitch + sw / 2;
      if(px > bx0 + segs * pitch - 4) px = bx0 + segs * pitch - 10;
      g.drawFastVLine(px, 155, 3, c.line);
      ckText(g, FontB612Reg10, s.t, px, 158, c.muted, 1);   // TC datum
    }
  }

  // Band position line along the bottom edge
  g.drawFastHLine(0, 169, 320, c.line);
  if(in.maxFreq > in.minFreq)
  {
    int px = (int)((uint64_t)319 * (in.freq - in.minFreq) / (in.maxFreq - in.minFreq));
    g.fillRect(px - 1, 167, 3, 3, c.pointer);
  }
}
