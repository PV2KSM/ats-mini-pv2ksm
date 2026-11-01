#pragma once
//
// PV2KSM v0 interface: palettes, drawing helpers and the main-screen layouts (Analog, S-Meter, Vintage, Clock,
// Memories). Header-only and written only against LovyanGFX, so the same code runs on the radio and in the PC preview.
// Everything that looks like a glow, glass or shadow is baked into solid colours (no per-pixel read-back), so a frame
// costs about the same as the old layouts.
//
#include "I18n.h"
#include "CockpitUI.h"

// ---------------------------------------------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------------------------------------------
static constexpr uint16_t nuRgb(int r, int g, int b) { return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)); }

#define NU_ACCENTS 7
#define NU_BGS     5
static const char *const nuAccentName[NU_ACCENTS] = {"Amber", "Green", "Cyan", "Blue", "Magenta", "Red", "White"};
static const char *const nuBgName[NU_BGS] = {"Graphite", "Black", "Navy", "Wine", "Forest"};
static const uint16_t nuAccentRgb[NU_ACCENTS] =
{
  nuRgb(255, 176, 48), nuRgb(110, 230, 140), nuRgb(110, 230, 255), nuRgb(110, 170, 255),
  nuRgb(255, 110, 230), nuRgb(255, 100, 80), nuRgb(255, 255, 255)
};
#define NU_TEXTS 12
static const char *const nuTextName[NU_TEXTS] = {"Accent", "White", "Cream", "Amber", "Orange", "Yellow", "Green", "Mint", "Cyan", "Sky", "Pink", "Red"};
static const uint16_t nuTextRgb[NU_TEXTS] =
{
  0, nuRgb(255, 255, 255), nuRgb(250, 236, 200), nuRgb(255, 190, 70), nuRgb(255, 140, 50), nuRgb(255, 235, 80),
  nuRgb(110, 230, 140), nuRgb(170, 255, 215), nuRgb(110, 230, 255), nuRgb(160, 200, 255), nuRgb(255, 140, 225), nuRgb(255, 110, 100)
};
static const uint16_t nuBgTop[NU_BGS] = {nuRgb(18, 22, 32), nuRgb(0, 0, 0), nuRgb(10, 26, 58), nuRgb(52, 14, 26), nuRgb(14, 44, 34)};
static const uint16_t nuBgBot[NU_BGS] = {nuRgb(8, 10, 15), nuRgb(0, 0, 0), nuRgb(4, 10, 28), nuRgb(22, 6, 12), nuRgb(5, 18, 13)};

struct NuPal
{
  uint16_t acc;        // Accent (ticks, chips, bars)
  uint16_t accDim;     // Dim accent (minor ticks)
  uint16_t accGlow;    // Soft glow around the accent
  uint16_t txt;        // Numbers and letters (white or the accent)
  uint16_t txt2;       // Secondary text
  uint16_t muted;      // Labels
  uint16_t bg1, bg2;   // Background gradient
  uint16_t card;       // Card fill
  uint16_t cardLine;   // Card outline
  uint16_t good, bad;  // Green / red signal colours
  uint16_t needle;     // Red tuning pointer
};

static inline NuPal nuPalette(uint8_t accent, uint8_t textIdx, uint8_t bg)
{
  if(textIdx >= NU_TEXTS) textIdx = 1;
  if(accent >= NU_ACCENTS) accent = 0;
  if(bg >= NU_BGS) bg = 0;
  NuPal p;
  p.acc = nuAccentRgb[accent];
  p.accDim = ckMix(nuBgTop[bg], p.acc, 120);
  p.accGlow = ckMix(nuBgTop[bg], p.acc, 70);
  p.txt = textIdx == 0 ? p.acc : nuTextRgb[textIdx];
  p.txt2 = ckMix(nuBgTop[bg], 0xFFFF, 190);
  p.muted = ckMix(nuBgTop[bg], 0xFFFF, 120);
  p.bg1 = nuBgTop[bg]; p.bg2 = nuBgBot[bg];
  p.card = ckMix(nuBgTop[bg], 0xFFFF, 20);
  p.cardLine = ckMix(nuBgTop[bg], 0xFFFF, 52);
  p.good = nuRgb(110, 230, 140); p.bad = nuRgb(255, 90, 70);
  p.needle = nuRgb(255, 80, 60);
  return p;
}

// ---------------------------------------------------------------------------------------------------------------
// Input for the layouts
// ---------------------------------------------------------------------------------------------------------------
struct NuMem { uint32_t freq; char name[10]; };

struct NuIn
{
  CkIn k;
  float sN, nN;               // Smoothed needles 0..1
  const NuMem *mem;           // Memories of the current band (may be null)
  int nMem;
  int day, month, wday;       // Date (month 0 = unknown)
  int ss;                     // Seconds 0..59 (clock layouts)
  const uint8_t *wf;          // Waterfall history: wfRows rows of wfCols samples 0..255, row 0 = newest sweep
  int wfRows, wfCols, wfHave; // wfHave = number of valid rows
  uint32_t wfLo, wfHi;        // Frequency span of the sweep (Hz)
  uint32_t ms;                // millis(), used to scroll names that do not fit
};

// ---------------------------------------------------------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------------------------------------------------------
namespace nu
{
  enum { D_TL=0, D_TC=1, D_TR=2, D_ML=4, D_MC=5, D_MR=6, D_BL=16, D_BC=17, D_BR=18 };
  static const char *const wdayName[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  static const char *const monName[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  template<class G> static void bgFill(G &g, const NuPal &p)
  {
    for(int y = 0 ; y < 170 ; y++) g.drawFastHLine(0, y, 320, ckMix(p.bg1, p.bg2, y * 256 / 170));
  }

  // ---- anti-aliased primitives (edges blend with what is already on the sprite) ----------------------------------
  // Arc band between radii r0..r1, angles in degrees as fillArc (0 = 3 o'clock, clockwise)
  template<class G> static void arcAA(G &g, float cx, float cy, float r0, float r1, float a0, float a1, uint16_t c)
  {
    const float rm = (r0 + r1) * 0.5f, hw = (r1 - r0) * 0.5f, k = 3.14159265f / 180.f;
    int n = (int)(fabsf(a1 - a0) * k * rm / 2.0f) + 2; if(n > 140) n = 140;
    float px = cx + rm * cosf(a0 * k), py = cy + rm * sinf(a0 * k);
    for(int i = 1 ; i <= n ; i++)
    {
      const float a = (a0 + (a1 - a0) * i / n) * k, x = cx + rm * cosf(a), y = cy + rm * sinf(a);
      g.drawWedgeLine(px, py, x, y, hw, hw, c);
      px = x; py = y;
    }
  }
  template<class G> static void ringAA(G &g, int cx, int cy, int r, uint16_t c) { arcAA(g, cx + 0.5f, cy + 0.5f, r - 0.5f, r + 0.5f, 0.f, 360.f, c); }
  template<class G> static void rrAA(G &g, int x, int y, int w, int h, int r, uint16_t c)
  {
    const float x0 = x + 0.5f, y0 = y + 0.5f, x1 = x + w - 0.5f, y1 = y + h - 0.5f;
    float R = r - 0.5f; const float mx = (w < h ? w : h) * 0.5f - 0.5f; if(R > mx) R = mx; if(R < 0) R = 0;
    g.drawWedgeLine(x0 + R, y0, x1 - R, y0, 0.5f, 0.5f, c); g.drawWedgeLine(x1, y0 + R, x1, y1 - R, 0.5f, 0.5f, c);
    g.drawWedgeLine(x1 - R, y1, x0 + R, y1, 0.5f, 0.5f, c); g.drawWedgeLine(x0, y1 - R, x0, y0 + R, 0.5f, 0.5f, c);
    if(R > 0.6f)
    {
      arcAA(g, x1 - R, y0 + R, R - 0.5f, R + 0.5f, 270.f, 360.f, c); arcAA(g, x1 - R, y1 - R, R - 0.5f, R + 0.5f, 0.f, 90.f, c);
      arcAA(g, x0 + R, y1 - R, R - 0.5f, R + 0.5f, 90.f, 180.f, c); arcAA(g, x0 + R, y0 + R, R - 0.5f, R + 0.5f, 180.f, 270.f, c);
    }
  }

  // Vertical gradient inside a rounded rectangle (rows near the corners are inset along the arc)
  template<class G> static void gradRound(G &g, int x, int y, int w, int h, int r, uint16_t c1, uint16_t c2, int gloss = 0)
  {
    for(int i = 0 ; i < h ; i++)
    {
      int inset = 0;
      const int dy = i < r ? r - i : (i >= h - r ? i - (h - r - 1) : 0);
      if(dy > 0) inset = r - (int)(sqrtf((float)(r * r - dy * dy)) + 0.5f);
      uint16_t c = ckMix(c1, c2, i * 256 / (h > 1 ? h - 1 : 1));
      if(gloss && i < h * 55 / 100) c = ckMix(c, 0xFFFF, gloss * (h * 55 / 100 - i) * (h * 55 / 100 - i) / ((h * 55 / 100) * (h * 55 / 100)));
      g.drawFastHLine(x + inset, y + i, w - 2 * inset, c);
    }
  }

  template<class G> static void card(G &g, int x, int y, int w, int h, int r, const NuPal &p, bool sel = false)
  {
    g.fillSmoothRoundRect(x + 1, y + 2, w, h, r, ckMix(p.bg2, 0x0000, 150));              // soft drop shadow
    const uint16_t fill = sel ? ckMix(p.card, p.acc, 55) : p.card;
    g.fillSmoothRoundRect(x, y, w, h, r, fill);
    if(w > 2 * r + 4) g.drawFastHLine(x + r, y + 1, w - 2 * r, ckMix(fill, 0xFFFF, sel ? 70 : 34));   // light on the top edge
    rrAA(g, x, y, w, h, r, sel ? p.acc : p.cardLine);
  }

  template<class G> static void text(G &g, const lgfx::GFXfont &f, const char *s, int x, int y, uint16_t col, int datum = 0)
  {
    g.setFont(&f); g.setTextDatum((lgfx::textdatum_t)datum); g.setTextColor(col); g.drawString(s, x, y);
  }


  // Text on a small rounded box in the theme colours, so it stays readable over scales, labels and icons.
  // Same x/y/datum convention as text(). `edge` picks the outline colour (cardLine normally, accent when highlighted).
  template<class G> static void tag(G &g, const NuPal &p, const lgfx::GFXfont &f, const char *s, int x, int y, uint16_t col, int datum = 0, bool hi = false)
  {
    g.setFont(&f);
    const int tw = g.textWidth(s), th = g.fontHeight(), pw = tw + 10, ph = th + 4;
    const int h = datum & 3, v = datum >> 2;                      // 0 left/1 centre/2 right; 0 top/1 middle/4 bottom (as 0,1,4)
    int bx = h == 0 ? x - 5 : (h == 1 ? x - pw / 2 : x - pw + 5);
    int by = v == 0 ? y - 2 : (v == 1 ? y - ph / 2 : y - ph + 2);
    if(bx < 1) bx = 1; if(bx + pw > 319) bx = 319 - pw;
    if(by < 1) by = 1; if(by + ph > 169) by = 169 - ph;
    g.fillRoundRect(bx, by, pw, ph, 5, p.bg2);
    g.drawRoundRect(bx, by, pw, ph, 5, hi ? p.acc : p.cardLine);
    g.setTextDatum(lgfx::textdatum_t::top_left); g.setTextColor(col); g.drawString(s, bx + 5, by + 2);
  }


  // Station / long name inside a fixed region: drawn as is when it fits, otherwise scrolled (marquee) so it never
  // runs over the neighbouring elements. y is the vertical centre; datum D_ML = left aligned, D_MC = centred.
  template<class G> static void nameFit(G &g, const NuPal &, const lgfx::GFXfont &f, const char *s, int x, int y, int w, uint16_t col, uint32_t ms, int datum = D_ML)
  {
    if(w < 8 || !s || !*s) return;
    g.setFont(&f);
    const int tw = g.textWidth(s), fh = g.fontHeight(), ty = y - fh / 2;
    g.setTextDatum(lgfx::textdatum_t::top_left); g.setTextColor(col);
    if(tw <= w) { g.drawString(s, datum == D_MC ? x + (w - tw) / 2 : x, ty); return; }
    const int gap = 28, off = (int)((ms / 40) % (uint32_t)(tw + gap));
    g.setClipRect(x, ty - 1, w, fh + 2);
    g.drawString(s, x - off, ty);
    g.drawString(s, x - off + tw + gap, ty);
    g.clearClipRect();
  }


  // Cursor under the selected frequency digit during direct entry (hl: low 7 bits = position, 0x80 = push+rotate,
  // 100 = off). xs/xe hold the left/right edge of every character of fs; digits are counted from the right, skipping
  // the dot. A "5" position (odd) also covers the next digit to the left, as in the original layouts.
  template<class G> static void digitCursorSpans(G &g, uint8_t hl, const char *fs, const int *xs, const int *xe, int y, uint16_t cNorm, uint16_t cSel)
  {
    if(hl == 100 || (hl & 0x7F) == 100) return;
    const int slot = (hl & 0x7F) / 2; const bool half = (hl & 1) != 0, sel = (hl & 0x80) != 0;
    const int n = (int)strlen(fs);
    int idx[16], nd = 0;
    for(int i = n - 1 ; i >= 0 && nd < 16 ; i--) if(fs[i] != '.') idx[nd++] = i;
    if(slot >= nd) return;
    int a = xs[idx[slot]], b = xe[idx[slot]];
    if(half && slot + 1 < nd && xs[idx[slot + 1]] < a) a = xs[idx[slot + 1]];
    if(sel)
    {
      g.fillRoundRect(a + 1, y - 1, b - a - 2, 3, 1, cSel);
      g.fillTriangle(a, y, a + 3, y - 3, a + 3, y + 3, cSel);
      g.fillTriangle(b - 1, y, b - 4, y - 3, b - 4, y + 3, cSel);
    }
    else g.fillRoundRect(a, y - 1, b - a, 3, 1, cNorm);
  }

  // Same, for a string drawn with a font at xLeft; yc = vertical centre of the text, the line goes just under it.
  template<class G> static void digitCursorFont(G &g, const lgfx::GFXfont &f, float scale, uint8_t hl, const char *fs, int xLeft, int yc, uint16_t cNorm, uint16_t cSel, int dy = 0)
  {
    if(hl == 100 || (hl & 0x7F) == 100) return;
    g.setFont(&f); g.setTextSize(scale);
    const int n = (int)strlen(fs); int xs[16], xe[16]; char b[16];
    if(n > 15) { g.setTextSize(1.0f); return; }
    for(int i = 0 ; i < n ; i++)
    {
      memcpy(b, fs, i); b[i] = 0; xs[i] = xLeft + g.textWidth(b);
      memcpy(b, fs, i + 1); b[i + 1] = 0; xe[i] = xLeft + g.textWidth(b);
    }
    const int y = yc + g.fontHeight() / 2 - 1 + dy;
    g.setTextSize(1.0f);
    digitCursorSpans(g, hl, fs, xs, xe, y, cNorm, cSel);
  }

  // Soft glow: a few outlines that fade into the background
  template<class G> static void glowRound(G &g, int x, int y, int w, int h, int r, uint16_t col, uint16_t bgc)
  {
    rrAA(g, x - 3, y - 3, w + 6, h + 6, r + 3, ckMix(bgc, col, 28));
    rrAA(g, x - 2, y - 2, w + 4, h + 4, r + 2, ckMix(bgc, col, 60));
    rrAA(g, x - 1, y - 1, w + 2, h + 2, r + 1, ckMix(bgc, col, 110));
  }

  template<class G> static void glowCircle(G &g, int cx, int cy, int r, uint16_t col, uint16_t bgc)
  {
    ringAA(g, cx, cy, r + 3, ckMix(bgc, col, 28));
    ringAA(g, cx, cy, r + 2, ckMix(bgc, col, 60));
    ringAA(g, cx, cy, r + 1, ckMix(bgc, col, 110));
  }

  // Battery / Wi-Fi are drawn by the firmware itself; the PC preview draws these stand-ins
  template<class G> static void wifiIcon(G &g, int x, int y, uint16_t col)
  {
    g.fillSmoothCircle(x, y, 1, col);
    for(int r = 4 ; r <= 10 ; r += 3) g.drawArc(x, y, r + 1, r, 225.f, 315.f, col);
  }
  template<class G> static void batteryIcon(G &g, int x, int y, uint16_t col, int lv)
  {
    rrAA(g, x, y, 17, 9, 2, col); g.fillRect(x + 17, y + 3, 2, 3, col); g.fillSmoothRoundRect(x + 2, y + 2, 13 * lv / 100, 5, 1, col);
  }

  // Frequency string: "97.50" in FM, "7205.000" otherwise. Returns the unit.
  static inline const char *freqStr(const CkIn &in, char *buf, size_t n)
  {
    if(in.mode == 0)
    {
      snprintf(buf, n, "%u.%02u", (unsigned)(in.freq / 1000000), (unsigned)((in.freq % 1000000) / 10000));
      return "MHz";
    }
    snprintf(buf, n, "%u.%03u", (unsigned)(in.freq / 1000), (unsigned)(in.freq % 1000));
    return "kHz";
  }

  // Walks the tuning scale: calls fn(x, major, value) for every tick. Same adaptive spacing as the old Analog layout.
  template<class Fn> static int scaleWalk(const CkIn &in, int xa, int xb, Fn fn)
  {
    const bool fm = in.mode == 0;
    const double div = fm ? 1e6 : 1e3;
    double lo = in.minFreq / div, hi = in.maxFreq / div, f = in.freq / div;
    if(hi <= lo) hi = lo + 1;
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
      fn(x, fabs(q - round(q)) < 1e-6, v, major);
    }
    int px = xa + (int)lround((f - lo) * ppu);
    return px < xa ? xa : (px > xb ? xb : px);
  }

  static inline void valStr(char *b, size_t n, double v, double major)
  {
    if(major < 1) snprintf(b, n, "%.1f", v); else snprintf(b, n, "%d", (int)lround(v));
  }

  // Needle as a wedge with a soft shadow; angles in degrees from 12 o'clock, clockwise
  template<class G> static void needle(G &g, float cx, float cy, float len, float a, float wid, uint16_t col, uint16_t shadow)
  {
    const float t = a * 3.14159265f / 180.f, x = cx + len * sinf(t), y = cy - len * cosf(t);
    g.drawWedgeLine(cx + 1.8f, cy + 1.8f, x + 1.8f, y + 1.8f, wid, 0.5f, shadow);
    g.drawWedgeLine(cx, cy, x, y, wid, 0.5f, col);
  }


  // S text from the 0..49 strength: S1..S9 up to 30, then S9+dB
  static inline void nuSText(char *b, size_t n, int s)
  {
    if(s <= 30) snprintf(b, n, "S%d", 1 + s * 8 / 30);
    else snprintf(b, n, "S9+%d", (s - 30) * 60 / 19);
  }

  // ----------------------------------------------------------------------------------------------------------
  // Analog layout: two round dials under a lit tuning scale
  // ----------------------------------------------------------------------------------------------------------
  static const uint16_t cream1 = nuRgb(248, 238, 208), cream2 = nuRgb(206, 186, 140), inkc = nuRgb(48, 38, 26), redc = nuRgb(196, 40, 32);
  static const uint16_t brass = nuRgb(214, 176, 98), wood = nuRgb(60, 46, 28);

  template<class G> static void bezel(G &g, int x, int y, int w, int h, int r)
  {
    g.fillSmoothRoundRect(x - 2, y - 2, w + 4, h + 4, r + 2, wood);
    rrAA(g, x - 1, y - 1, w + 2, h + 2, r + 1, brass);
  }

  template<class G> static void meter(G &g, int x, int y, int w, int h, const char *label, const char *val, float frac,
                                      const char *const *tl, const float *tf, int nt, float redFrom)
  {
    bezel(g, x, y, w, h, 6);
    gradRound(g, x, y, w, h, 6, cream1, cream2, 40);
    const float cx = x + w / 2.f, cy = y + h - 16.f, R = 66.f, a0 = -60.f, a1 = 60.f;
    if(redFrom >= 0) arcAA(g, cx, cy, R - 3.0f, R - 0.5f, 270.f + a0 + (a1 - a0) * redFrom, 270.f + a1, redc);
    arcAA(g, cx, cy, R - 1.0f, R, 270.f + a0, 270.f + a1 - (redFrom >= 0 ? (a1 - a0) * (1 - redFrom) : 0), inkc);
    for(int i = 0 ; i <= 20 ; i++)
    {
      const float a = a0 + (a1 - a0) * i / 20.f, t = a * 3.14159265f / 180.f;
      const uint16_t col = (redFrom >= 0 && i / 20.f >= redFrom) ? redc : inkc;
      g.drawWedgeLine(cx + R * sinf(t), cy - R * cosf(t), cx + (R - 3) * sinf(t), cy - (R - 3) * cosf(t), 0.5f, 0.5f, col);
    }
    for(int i = 0 ; i < nt ; i++)
    {
      const float a = a0 + (a1 - a0) * tf[i], t = a * 3.14159265f / 180.f;
      const uint16_t col = (redFrom >= 0 && tf[i] >= redFrom) ? redc : inkc;
      g.drawWedgeLine(cx + R * sinf(t), cy - R * cosf(t), cx + (R - 6) * sinf(t), cy - (R - 6) * cosf(t), 0.9f, 0.9f, col);
      text(g, FontB612Reg10, tl[i], (int)lroundf(cx + (R - 15) * sinf(t)), (int)lroundf(cy - (R - 15) * cosf(t)), col, D_MC);
    }
    if(frac < 0) frac = 0; if(frac > 1) frac = 1;
    needle(g, cx, cy, R - 2, a0 + (a1 - a0) * frac, 1.6f, nuRgb(30, 24, 18), ckMix(cream2, 0, 90));
    g.fillSmoothCircle((int)cx, (int)cy, 4, nuRgb(40, 32, 22)); g.fillSmoothCircle((int)cx, (int)cy, 1, brass);
    text(g, FontB612Bold12, label, x + 6, y + h - 4, inkc, D_BL);
    text(g, FontB612Bold12, val, x + w - 6, y + h - 4, inkc, D_BR);
  }

  template<class G> static void layoutAnalog(G &g, const NuIn &n, const NuPal &p)
  {
    const CkIn &in = n.k;
    bgFill(g, p);
    // Header: mode chip, frequency, station
    const char *m = in.modeName;
    g.fillSmoothRoundRect(6, 3, 30, 16, 4, p.acc);
    text(g, FontB612Bold12, m, 21, 11, nuRgb(30, 20, 8), D_MC);
    char fs[16]; const char *unit = freqStr(in, fs, sizeof(fs));
    text(g, FontB612Bold16, fs, 44, 11, p.txt, D_ML);
    g.setFont(&FontB612Bold16); const int fw = g.textWidth(fs);
    text(g, FontB612Reg10, unit, 44 + fw + 4, 14, p.muted, D_ML);
    { g.setFont(&FontB612Reg10); const int ux = 44 + fw + 4 + g.textWidth(unit) + 10;
      nameFit(g, p, FontB612Bold12, in.station, ux, 11, 216 - ux, p.txt2, n.ms); }
    digitCursorFont(g, FontB612Bold16, 1.0f, in.hl, fs, 44, 11, p.acc, p.needle);
    // Lit tuning scale
    bezel(g, 6, 24, 308, 46, 5);
    gradRound(g, 6, 24, 308, 46, 5, cream1, cream2, 70);
    const int px = scaleWalk(in, 14, 306, [&](int x, bool maj, double v, double major)
    {
      g.drawFastVLine(x, 30, maj ? 15 : 6, maj ? inkc : nuRgb(110, 92, 64));
      if(maj) { g.drawFastVLine(x + 1, 30, 15, inkc); char b[12]; valStr(b, sizeof(b), v, major); if(x > 22 && x < 298) text(g, FontB612Bold12, b, x, 57, inkc, D_MC); }
    });
    // Pointer with glow + shadow
    g.drawFastVLine(px + 2, 26, 42, ckMix(cream2, 0, 70));
    g.drawFastVLine(px - 2, 26, 42, ckMix(cream2, redc, 70)); g.drawFastVLine(px + 1, 26, 42, ckMix(cream2, redc, 70));
    g.drawFastVLine(px - 1, 25, 44, redc); g.drawFastVLine(px, 25, 44, redc);
    g.fillTriangle(px - 4, 25, px + 4, 25, px, 31, redc);
    // Dials
    static const char *const sl[] = {"1", "3", "5", "7", "9", "+20", "+40"};
    static const float sf[] = {0.00f, 0.18f, 0.36f, 0.50f, 0.62f, 0.78f, 0.92f};
    static const char *const nl[] = {"0", "10", "20", "30", "40"};
    static const float nf[] = {0.f, 0.25f, 0.5f, 0.75f, 1.f};
    const int s = ckStrength(in.rssi, in.mode == 0);
    char sv[10], nv[10];
    nuSText(sv, sizeof(sv), s);
    snprintf(nv, sizeof(nv), "%d dB", in.snr);
    meter(g, 6, 76, 151, 88, "S", sv, n.sN, sl, sf, 7, 0.62f);
    meter(g, 163, 76, 151, 88, "SNR", nv, n.nN, nl, nf, 5, -1.f);
  }

  // ----------------------------------------------------------------------------------------------------------
  // S-Meter layout: big frequency, glass scale and LED bars
  // ----------------------------------------------------------------------------------------------------------
  template<class G> static void ledBar(G &g, int x, int y, int w, int h, int nseg, float frac, const NuPal &p, bool snrBar)
  {
    const float sw = (float)w / nseg;
    for(int i = 0 ; i < nseg ; i++)
    {
      const float f = (float)i / (nseg - 1);
      uint16_t c = snrBar ? p.acc : (f < 0.6f ? p.good : (f < 0.82f ? nuRgb(255, 190, 50) : p.bad));
      const bool on = f <= frac;
      const int x0 = x + (int)(i * sw), x1 = x + (int)((i + 1) * sw) - 2;
      g.fillSmoothRoundRect(x0, y, x1 - x0 + 1, h, 1, on ? c : ckMix(p.bg2, 0xFFFF, 24));
      if(on) g.drawFastHLine(x0 + 1, y + 1, x1 - x0 - 1, ckMix(c, 0xFFFF, 90));
    }
  }

  template<class G> static void layoutSMeter(G &g, const NuIn &n, const NuPal &p)
  {
    const CkIn &in = n.k;
    bgFill(g, p);
    rrAA(g, 6, 4, 32, 16, 4, p.acc);
    text(g, FontB612Bold12, in.modeName, 22, 12, p.txt, D_MC);
    char fs[16]; const char *unit = freqStr(in, fs, sizeof(fs));
    g.setFont(&FontB612Big); g.setTextSize(0.88f); const int fw = g.textWidth(fs);
    const int fx = 160 - fw / 2;
    text(g, FontB612Big, fs, fx + 1, 35, ckMix(p.bg1, p.acc, 90), D_ML);          // soft glow
    text(g, FontB612Big, fs, fx, 34, p.txt, D_ML);
    g.setTextSize(1.0f);
    text(g, FontB612Reg10, unit, fx + fw + 4, 42, p.acc, D_ML);
    nameFit(g, p, FontB612Bold12, in.station, 44, 11, 172, p.txt2, n.ms);          // top row, beside the mode chip (ends before the Wi-Fi / battery banner)
    digitCursorFont(g, FontB612Big, 0.88f, in.hl, fs, fx, 34, p.acc, p.needle, -3);
    // glass scale
    g.fillSmoothRoundRect(8, 52, 304, 40, 6, nuRgb(18, 17, 16));
    rrAA(g, 8, 52, 304, 40, 6, ckMix(p.bg1, p.acc, 90));
    const int px = scaleWalk(in, 16, 304, [&](int x, bool maj, double v, double major)
    {
      g.drawFastVLine(x, 56, maj ? 15 : 5, maj ? p.acc : p.accDim);
      if(maj) { char b[12]; valStr(b, sizeof(b), v, major); if(x > 24 && x < 296) text(g, FontB612Reg10, b, x, 82, p.txt, D_MC); }
    });
    g.drawFastVLine(px - 1, 54, 36, ckMix(p.bg1, p.needle, 90)); g.drawFastVLine(px + 1, 54, 36, ckMix(p.bg1, p.needle, 90));
    g.drawFastVLine(px, 53, 38, p.needle);
    // LED bars
    const int s = ckStrength(in.rssi, in.mode == 0);
    char sv[10], nv[10]; nuSText(sv, sizeof(sv), s); snprintf(nv, sizeof(nv), "%ddB", in.snr);
    text(g, FontB612Bold12, "S", 8, 107, p.txt2, D_ML); text(g, FontB612Bold12, "SNR", 8, 123, p.txt2, D_ML);
    ledBar(g, 40, 102, 228, 9, 26, n.sN, p, false); ledBar(g, 40, 118, 228, 9, 26, n.nN, p, true);
    text(g, FontB612Bold12, sv, 312, 107, p.txt, D_MR); text(g, FontB612Bold12, nv, 312, 123, p.txt, D_MR);
    // chips
    const char *lab[4] = {"VOL", "STEP", "BW", in.agcLabel ? in.agcLabel : "AGC"};
    char vol[8]; snprintf(vol, sizeof(vol), "%d", in.volume);
    const char *val[4] = {vol, in.step ? in.step : "", in.bw ? in.bw : "", in.agc ? in.agc : ""};
    for(int i = 0 ; i < 4 ; i++)
    {
      const int x = 8 + i * 77;
      card(g, x, 134, 70, 34, 5, p);
      text(g, FontB612Reg10, lab[i], x + 6, 137, p.muted, D_TL);
      text(g, FontB612Bold12, val[i], x + 64, 165, p.txt, D_BR);
    }
  }

  // ----------------------------------------------------------------------------------------------------------
  // Vintage layout: wooden receiver with a glass slide-rule, two knobs and a magic eye
  // ----------------------------------------------------------------------------------------------------------
  template<class G> static void layoutVintage(G &g, const NuIn &n, const NuPal &p)
  {
    const CkIn &in = n.k;
    for(int y = 0 ; y < 170 ; y++) g.drawFastHLine(0, y, 320, ckMix(nuRgb(98, 60, 30), nuRgb(50, 29, 13), y * 256 / 170));
    for(int y = 0 ; y < 170 ; y += 5) g.drawFastHLine(0, y, 320, ckMix(nuRgb(98, 60, 30), nuRgb(50, 29, 13), 90));
    g.fillSmoothRoundRect(33, 13, 254, 94, 8, nuRgb(30, 20, 10)); rrAA(g, 33, 13, 254, 94, 8, brass);
    gradRound(g, 36, 16, 248, 88, 6, nuRgb(246, 228, 180), nuRgb(204, 176, 118), 60);
    const int px = scaleWalk(in, 44, 276, [&](int x, bool maj, double v, double major)
    {
      g.drawFastVLine(x, 24, maj ? 18 : 6, maj ? nuRgb(50, 34, 18) : nuRgb(128, 98, 62));
      if(maj) { char b[12]; valStr(b, sizeof(b), v, major); if(x > 52 && x < 268) text(g, FontB612Bold12, b, x, 52, nuRgb(50, 34, 18), D_MC); }
    });
    // memory names under the scale
    if(n.mem) for(int i = 0 ; i < n.nMem && i < 3 ; i++)
    {
      const double f = n.mem[i].freq, lo = in.minFreq, hi = in.maxFreq;
      if(hi > lo && f >= lo && f <= hi) text(g, FontB612Reg10, n.mem[i].name, 44 + (int)((f - lo) / (hi - lo) * 232), 64 + (i & 1) * 12, nuRgb(110, 60, 30), D_MC);
    }
    g.drawFastVLine(px + 2, 18, 80, nuRgb(180, 150, 100)); g.drawFastVLine(px - 1, 18, 80, redc); g.drawFastVLine(px, 18, 80, redc);
    char fs[16]; const char *unit = freqStr(in, fs, sizeof(fs)); char line[40]; snprintf(line, sizeof(line), "%s  %s %s", in.modeName, fs, unit);
    text(g, FontB612Bold12, line, 160, 88, nuRgb(50, 30, 14), D_MC);
    { g.setFont(&FontB612Bold12); const int lw = g.textWidth(line); char pre[16]; snprintf(pre, sizeof(pre), "%s  ", in.modeName);
      digitCursorFont(g, FontB612Bold12, 1.0f, in.hl, fs, 160 - lw / 2 + g.textWidth(pre), 88, redc, nuRgb(30, 18, 8), 1); }
    for(int k = 0 ; k < 2 ; k++)
    {
      const int cx = k ? 303 : 17;
      g.fillSmoothCircle(cx, 62, 13, nuRgb(30, 18, 8)); g.fillSmoothCircle(cx, 60, 10, nuRgb(214, 182, 112)); g.fillSmoothCircle(cx - 3, 57, 3, nuRgb(240, 214, 150));
      g.drawLine(cx, 60, cx, 51, nuRgb(70, 44, 18));
      text(g, FontB612Reg10, k ? "TUNE" : "VOL", cx, 82, nuRgb(230, 200, 130), D_MC);
    }
    // magic eye: green fan that opens with the signal
    g.fillSmoothCircle(160, 131, 16, nuRgb(10, 24, 12)); ringAA(g, 160, 131, 16, brass); ringAA(g, 160, 131, 15, nuRgb(120, 90, 40));
    const float open = 60.f + 240.f * n.sN;
    g.fillArc(160, 131, 4, 13, 270.f - open / 2, 270.f + open / 2, nuRgb(130, 255, 130));
    g.fillSmoothCircle(160, 131, 3, nuRgb(10, 24, 12));
    nameFit(g, p, FontB612Bold12, in.station, 22, 132, 118, nuRgb(240, 218, 160), n.ms);
    char sv[10], nv[10]; nuSText(sv, sizeof(sv), ckStrength(in.rssi, in.mode == 0)); snprintf(nv, sizeof(nv), "%d dB", in.snr);
    text(g, FontB612Bold12, sv, 298, 126, nuRgb(240, 218, 160), D_TR); text(g, FontB612Reg10, nv, 298, 144, nuRgb(190, 160, 100), D_TR);
    if(in.step && in.step[0]) text(g, FontB612Reg10, in.step, 22, 144, nuRgb(190, 160, 100), D_TL);
  }

  // ----------------------------------------------------------------------------------------------------------
  // Clock layout: big time, tuned station and a weather card
  // ----------------------------------------------------------------------------------------------------------
  template<class G> static void layoutClock(G &g, const NuIn &n, const NuPal &p)
  {
    const CkIn &in = n.k;
    bgFill(g, p);
    rrAA(g, 6, 4, 32, 16, 4, p.acc); text(g, FontB612Bold12, in.modeName, 22, 12, p.acc, D_MC);
    char hs[4], ms[4];
    if(in.haveTime) { snprintf(hs, sizeof(hs), "%02d", in.hh); snprintf(ms, sizeof(ms), "%02d", in.mm); } else { strcpy(hs, "--"); strcpy(ms, "--"); }
    g.setFont(&FontB612Big); const int tw2 = g.textWidth("00");
    text(g, FontB612Big, hs, 160 - 10, 62, p.txt, D_MR); text(g, FontB612Big, ms, 160 + 10, 62, p.txt, D_ML);
    (void)tw2;
    g.fillSmoothRoundRect(156, 50, 8, 8, 2, p.acc); g.fillSmoothRoundRect(156, 68, 8, 8, 2, p.acc);
    if(n.month >= 1 && n.month <= 12)
    {
      char d[40]; snprintf(d, sizeof(d), "%s  %d %s", uiText(wdayName[n.wday % 7]), n.day, uiText(monName[n.month - 1]));
      text(g, FontB612Bold12, d, 160, 98, p.txt2, D_MC);
    }
    card(g, 8, 122, 144, 40, 8, p);
    char fs[16]; const char *unit = freqStr(in, fs, sizeof(fs)); char fl[24]; snprintf(fl, sizeof(fl), "%s %s", fs, unit);
    text(g, FontB612Bold12, fl, 16, 130, p.acc, D_TL);
    digitCursorFont(g, FontB612Bold12, 1.0f, in.hl, fs, 16, 137, p.txt, p.needle);
    nameFit(g, p, FontB612Reg10, in.station && in.station[0] ? in.station : in.modeName, 16, 152, 130, p.muted, n.ms);
    card(g, 168, 122, 144, 40, 8, p);
    if(in.haveWx && in.wx.valid)
    {
      const WxData &d = in.wx; const int kind = wxKind(d.code); WxBg bg = wxBackground(kind, d.isDay != 0, 40);
      wxIcon(g, kind, d.isDay != 0, 190, 142, 0.34f, in.t, bg);
      char b[24]; snprintf(b, sizeof(b), "%d`", d.temp); text(g, FontB612Bold16, b, 212, 133, p.txt, D_TL);
      snprintf(b, sizeof(b), "%d%%", d.hum); text(g, FontB612Reg10, b, 212, 148, p.muted, D_TL);
    }
    else text(g, FontB612Reg10, in.wxStatus && in.wxStatus[0] ? in.wxStatus : "--", 240, 142, p.muted, D_MC);
  }

  // ----------------------------------------------------------------------------------------------------------
  // Memories layout: the band as a line with your saved stations as pins
  // ----------------------------------------------------------------------------------------------------------
  template<class G> static void layoutMemories(G &g, const NuIn &n, const NuPal &p)
  {
    const CkIn &in = n.k;
    bgFill(g, p);
    rrAA(g, 6, 4, 32, 16, 4, p.acc); text(g, FontB612Bold12, in.modeName, 22, 12, p.acc, D_MC);
    char fs[16]; const char *unit = freqStr(in, fs, sizeof(fs));
    g.setFont(&FontB612Big); const int fw = g.textWidth(fs);
    text(g, FontB612Big, fs, 160 - fw / 2, 40, p.txt, D_ML);
    digitCursorFont(g, FontB612Big, 1.0f, in.hl, fs, 160 - fw / 2, 40, p.acc, p.needle, -3);
    text(g, FontB612Reg10, unit, 160 + fw / 2 + 4, 48, p.acc, D_ML);
    if(in.station && in.station[0]) nameFit(g, p, FontB612Bold12, in.station, 64, 11, 152, p.txt2, n.ms);
    const int y = 104, x0 = 14, x1 = 306;
    const double lo = in.minFreq, hi = in.maxFreq > in.minFreq ? in.maxFreq : in.minFreq + 1;
    g.fillSmoothRoundRect(x0, y - 1, x1 - x0, 3, 1, ckMix(p.bg1, 0xFFFF, 60));
    const int px = scaleWalk(in, x0, x1, [&](int x, bool maj, double v, double major)
    {
      if(!maj) return;
      g.drawFastVLine(x, y + 2, 5, p.muted); char b[12]; valStr(b, sizeof(b), v, major); if(x > x0 + 8 && x < x1 - 8) text(g, FontB612Reg10, b, x, y + 14, p.muted, D_MC);
    });
    // pins: heights alternate so close memories do not overlap
    int lastX[3] = {-100, -100, -100};
    if(n.mem) for(int i = 0 ; i < n.nMem ; i++)
    {
      const double f = n.mem[i].freq; if(f < lo || f > hi) continue;
      const int x = x0 + (int)((f - lo) / (hi - lo) * (x1 - x0));
      int lv = 0; while(lv < 2 && x - lastX[lv] < 36) lv++;
      lastX[lv] = x;
      const bool cur = fabs(f - in.freq) < 1.0;
      const uint16_t c = cur ? p.acc : ckMix(p.bg1, nuRgb(160, 205, 255), 230);
      const int h = 20 + lv * 14;
      g.drawFastVLine(x, y - h, h, c); g.fillSmoothCircle(x, y - h, 2, c);
      text(g, FontB612Reg10, n.mem[i].name, x, y - h - 8, cur ? p.txt : p.txt2, D_MC);
    }
    g.drawFastVLine(px - 1, y - 5, 14, ckMix(p.bg1, p.needle, 90)); g.drawFastVLine(px + 1, y - 5, 14, ckMix(p.bg1, p.needle, 90));
    g.drawFastVLine(px, y - 6, 16, p.needle);
    char sv[10], nv[10], vol[8]; nuSText(sv, sizeof(sv), ckStrength(in.rssi, in.mode == 0)); snprintf(nv, sizeof(nv), "%d dB", in.snr); snprintf(vol, sizeof(vol), "%d", in.volume);
    const char *lab[4] = {"S", "SNR", "VOL", "STEP"}; const char *val[4] = {sv, nv, vol, in.step ? in.step : ""};
    for(int i = 0 ; i < 4 ; i++) { text(g, FontB612Reg10, lab[i], 8 + i * 76, 152, p.muted, D_ML); text(g, FontB612Bold12, val[i], 8 + i * 76 + 68, 152, p.txt, D_MR); }
  }


  // ----------------------------------------------------------------------------------------------------------
  // Waterfall: frequency on top, spectrum of the last sweep and the history of sweeps as a heat map
  // ----------------------------------------------------------------------------------------------------------
  static inline uint16_t heat(int v)   // 0..255 -> dark blue, blue, cyan, yellow, red
  {
    static const uint16_t stops[5] = {nuRgb(4, 8, 30), nuRgb(20, 60, 200), nuRgb(40, 220, 230), nuRgb(255, 230, 60), nuRgb(255, 50, 30)};
    if(v < 0) v = 0; if(v > 255) v = 255;
    const int seg = v * 4 / 256, f = (v * 4 % 256);
    return ckMix(stops[seg], stops[seg + 1], f);
  }

  template<class G> static void layoutWaterfall(G &g, const NuIn &n, const NuPal &p)
  {
    const CkIn &in = n.k;
    bgFill(g, p);
    rrAA(g, 6, 5, 32, 16, 4, p.acc); text(g, FontB612Bold12, in.modeName, 22, 13, p.acc, D_MC);
    char fs[16]; const char *unit = freqStr(in, fs, sizeof(fs));
    text(g, FontB612Big, fs, 46, 3, p.txt, D_TL);
    g.setFont(&FontB612Big); const int fw = g.textWidth(fs);
    text(g, FontB612Reg10, unit, 46 + fw + 4, 28, p.acc, D_ML);
    if(in.station && in.station[0]) tag(g, p, FontB612Bold12, in.station, 314, 4, p.txt2, D_TR);
    char sv[10]; nuSText(sv, sizeof(sv), ckStrength(in.rssi, in.mode == 0));
    text(g, FontB612Bold12, sv, 314, 22, p.acc, D_TR);

    const int x0 = 10, x1 = 310, w = x1 - x0, ys = 44, hs = 36, yw = 84, rh = 9, rows = 7;
    card(g, x0 - 4, ys - 3, w + 8, hs + 6 + rows * rh + 6 + 14, 6, p);
    const double lo = n.wfLo, hi = n.wfHi > n.wfLo ? n.wfHi : n.wfLo + 1;
    if(!n.wfHave || !n.wf)
    {
      text(g, FontB612Bold12, uiText("No sweep yet"), 160, 92, p.txt2, D_MC);
      text(g, FontB612Reg10, "Menu > Scan", 160, 110, p.muted, D_MC);
    }
    else
    {
      // spectrum of the newest sweep, filled
      for(int x = 0 ; x < w ; x++)
      {
        const int c = x * n.wfCols / w, v = n.wf[c];
        const int h = v * hs / 255;
        g.drawFastVLine(x0 + x, ys + hs - h, h, ckMix(p.bg1, p.acc, 70 + v / 2));
        if(h > 0) g.drawPixel(x0 + x, ys + hs - h, p.acc);
      }
      // waterfall rows (newest at the top)
      for(int r = 0 ; r < rows && r < n.wfHave ; r++)
        for(int x = 0 ; x < w ; x++) g.drawFastVLine(x0 + x, yw + r * rh, rh - 1, heat(n.wf[r * n.wfCols + x * n.wfCols / w]));
    }
    // tuning marker over spectrum and waterfall
    if(in.freq >= lo && in.freq <= hi)
    {
      const int px = x0 + (int)((in.freq - lo) / (hi - lo) * (w - 1));
      g.drawFastVLine(px, ys - 2, hs + 8 + rows * rh, p.needle);
      g.fillTriangle(px - 4, ys - 3, px + 4, ys - 3, px, ys + 3, p.needle);
    }
    // axis labels
    const double div = in.mode == 0 ? 1e6 : 1e3;
    char b[16];
    for(int i = 0 ; i < 3 ; i++)
    {
      snprintf(b, sizeof(b), in.mode == 0 ? "%.1f" : "%.0f", (lo + (hi - lo) * i / 2.0) / div);
      text(g, FontB612Reg10, b, i == 0 ? x0 : (i == 1 ? 160 : x1), 158, p.muted, i == 0 ? D_ML : (i == 1 ? D_MC : D_MR));
    }
  }

  // ----------------------------------------------------------------------------------------------------------
  // Icons (vector, drawn by code: no bitmaps, no flash cost). s = half size in pixels.
  // ----------------------------------------------------------------------------------------------------------
  template<class G> static inline void ln(G &g, float x0, float y0, float x1, float y1, float r, uint16_t c) { g.drawWedgeLine(x0, y0, x1, y1, r, r, c); }

  static inline int iconId(const char *n)
  {
    static const char *const names[] = {"Mode", "Band", "Volume", "VFO A/B", "Step", "Seek", "Scan", "Memory", "Squelch", "Bandwidth", "AGC/ATTN", "AVC", "SoftMute",
      "Flights", "Weather", "Finance", "Network", "Settings", "Brightness", "Date/Time", "UTC Offset", "Sleep Timer", "Sleep Mode", "Off Timer", "Theme", "UI Layout",
      "Zoom Menu", "Scroll Dir.", "RDS", "FM Stereo", "FM Region", "Calibration", "DSP Patches", "Load EiBi", "Update FW", "USB Port", "TCP Port", "Bluetooth", "Wi-Fi",
      "Language", "Accent", "Text color", "Background", "Menu color", "Menu style", "About"};
    if(!strcmp(n, "Visual")) return 24;   // palette icon (same as Theme)
    if(!strcmp(n, "News")) return 46;   // newspaper
    for(unsigned i = 0 ; i < sizeof(names) / sizeof(names[0]) ; i++) if(!strcmp(n, names[i])) return (int)i;
    return 17;   // gear
  }

  // Outline helpers with a thickness that scales with the icon (thin lines vanish when the icon is averaged down)
  template<class G> static void ring(G &g, int x, int y, int r, uint16_t c, int th) { g.fillArc(x, y, r - th > 0 ? r - th : 0, r, 0.f, 360.f, c); }
  template<class G> static void rrect(G &g, int x, int y, int w, int h, int rad, uint16_t c, int th)
  {
    for(int k = 0 ; k < th ; k++) g.drawRoundRect(x + k, y + k, w - 2 * k, h - 2 * k, rad - k > 0 ? rad - k : 0, c);
  }

  template<class G> static void icon(G &g, int id, float cx, float cy, float s, uint16_t c, uint16_t bgc)
  {
    const float r = s * 0.11f > 0.9f ? s * 0.11f : 0.9f;
    const int th = (int)(s * 0.12f + 0.5f) < 1 ? 1 : (int)(s * 0.12f + 0.5f);
    switch(id)
    {
      case 0: // Mode: wave
      {
        float px = cx - s, py = cy;
        for(int i = 1 ; i <= 24 ; i++) { const float x = cx - s + 2 * s * i / 24, y = cy - s * 0.55f * sinf(i / 24.f * 12.566f); ln(g, px, py, x, y, r, c); px = x; py = y; }
        break;
      }
      case 38: // Wi-Fi: signal arcs
        g.fillCircle((int)cx, (int)(cy + s * 0.62f), (int)(r + 1), c);
        for(int k = 1 ; k <= 3 ; k++) g.fillArc((int)cx, (int)(cy + s * 0.62f), (int)(s * 0.5f * k), (int)(s * 0.5f * k + r + 1), 225.f, 315.f, c);
        break;
      case 2: case 12: // Volume / SoftMute
        g.fillTriangle((int)(cx - s * 0.1f), (int)(cy - s * 0.75f), (int)(cx - s * 0.1f), (int)(cy + s * 0.75f), (int)(cx - s * 0.65f), (int)cy, c);
        g.fillRect((int)(cx - s), (int)(cy - s * 0.3f), (int)(s * 0.4f), (int)(s * 0.6f), c);
        if(id == 2) { g.fillArc((int)(cx + s * 0.1f), (int)cy, (int)(s * 0.5f), (int)(s * 0.5f + r + 1), -50.f, 50.f, c); g.fillArc((int)(cx + s * 0.1f), (int)cy, (int)(s * 0.95f), (int)(s * 0.95f + r + 1), -50.f, 50.f, c); }
        else { ln(g, cx + s * 0.35f, cy - s * 0.4f, cx + s * 0.95f, cy + s * 0.4f, r, c); ln(g, cx + s * 0.35f, cy + s * 0.4f, cx + s * 0.95f, cy - s * 0.4f, r, c); }
        break;
      case 4: // Step: arrows
        ln(g, cx - s, cy, cx + s, cy, r, c);
        g.fillTriangle((int)(cx - s), (int)cy, (int)(cx - s * 0.5f), (int)(cy - s * 0.35f), (int)(cx - s * 0.5f), (int)(cy + s * 0.35f), c);
        g.fillTriangle((int)(cx + s), (int)cy, (int)(cx + s * 0.5f), (int)(cy - s * 0.35f), (int)(cx + s * 0.5f), (int)(cy + s * 0.35f), c);
        break;
      
      case 7: // Memory: bookmark
        g.fillTriangle((int)(cx - s * 0.6f), (int)(cy - s * 0.95f), (int)(cx + s * 0.6f), (int)(cy - s * 0.95f), (int)(cx + s * 0.6f), (int)(cy + s * 0.95f), c);
        g.fillTriangle((int)(cx - s * 0.6f), (int)(cy - s * 0.95f), (int)(cx - s * 0.6f), (int)(cy + s * 0.95f), (int)(cx + s * 0.6f), (int)(cy + s * 0.95f), c);
        g.fillTriangle((int)(cx - s * 0.5f), (int)(cy + s * 0.95f), (int)(cx + s * 0.5f), (int)(cy + s * 0.95f), (int)cx, (int)(cy + s * 0.4f), bgc);
        break;
      case 8: // Squelch
        g.fillArc((int)cx, (int)(cy + s * 0.5f), (int)(s * 0.9f), (int)(s * 0.9f + r + 1), 200.f, 340.f, c);
        ln(g, cx, cy + s * 0.5f, cx + s * 0.5f, cy - s * 0.3f, r, c); g.fillCircle((int)cx, (int)(cy + s * 0.5f), (int)(r + 1), c);
        break;
      case 9: // Bandwidth: pulse
        ln(g, cx - s, cy + s * 0.7f, cx - s * 0.5f, cy + s * 0.7f, r, c); ln(g, cx - s * 0.5f, cy + s * 0.7f, cx - s * 0.5f, cy - s * 0.7f, r, c);
        ln(g, cx - s * 0.5f, cy - s * 0.7f, cx + s * 0.5f, cy - s * 0.7f, r, c); ln(g, cx + s * 0.5f, cy - s * 0.7f, cx + s * 0.5f, cy + s * 0.7f, r, c);
        ln(g, cx + s * 0.5f, cy + s * 0.7f, cx + s, cy + s * 0.7f, r, c);
        break;
      case 10: case 11: // AGC / AVC: ramp
        ln(g, cx - s, cy + s * 0.7f, cx - s * 0.3f, cy + s * 0.7f, r, c); ln(g, cx - s * 0.3f, cy + s * 0.7f, cx + s * 0.3f, cy - s * 0.7f, r, c); ln(g, cx + s * 0.3f, cy - s * 0.7f, cx + s, cy - s * 0.7f, r, c);
        break;
      case 14: // Weather: sun behind a cloud
      {
        const float sx = cx - s * 0.42f, sy = cy - s * 0.42f;
        g.fillCircle((int)sx, (int)sy, (int)(s * 0.36f), c);
        for(int i = 0 ; i < 8 ; i++) { const float a = i * 0.7854f; ln(g, sx + s * 0.55f * cosf(a), sy + s * 0.55f * sinf(a), sx + s * 0.8f * cosf(a), sy + s * 0.8f * sinf(a), r * 0.9f, c); }
        for(int pass = 0 ; pass < 2 ; pass++)
        {
          const float gr = pass == 0 ? s * 0.16f : 0; const uint16_t cc = pass == 0 ? bgc : c;
          g.fillCircle((int)(cx - s * 0.05f), (int)(cy + s * 0.2f), (int)(s * 0.46f + gr), cc);
          g.fillCircle((int)(cx + s * 0.5f), (int)(cy + s * 0.36f), (int)(s * 0.36f + gr), cc);
          g.fillRoundRect((int)(cx - s * 0.8f - gr), (int)(cy + s * 0.3f - gr), (int)(s * 1.95f + 2 * gr), (int)(s * 0.52f + 2 * gr), (int)(s * 0.26f + gr), cc);
        }
        break;
      }
      case 13: // Flights: airliner, top view, nose up-right
      {
        const float ca = 0.7071f, sa = 0.7071f, k = s * 0.98f;
        auto P = [&](float x, float y, int *ox, int *oy) { *ox = (int)(cx + (x * ca - y * sa) * k + 0.5f); *oy = (int)(cy + (x * sa + y * ca) * k + 0.5f); };
        auto poly = [&](const float (*q)[2], int n)
        {
          int x0, y0, x1, y1, x2, y2; P(q[0][0], q[0][1], &x0, &y0);
          for(int i = 1 ; i < n - 1 ; i++) { P(q[i][0], q[i][1], &x1, &y1); P(q[i + 1][0], q[i + 1][1], &x2, &y2); g.fillTriangle(x0, y0, x1, y1, x2, y2, c); }
        };
        static const float fus[][2] = {{0,-1},{.07f,-.9f},{.11f,-.72f},{.115f,.55f},{.07f,.85f},{.035f,.97f},{-.035f,.97f},{-.07f,.85f},{-.115f,.55f},{-.11f,-.72f},{-.07f,-.9f}};
        static const float wr[][2] = {{.1f,-.2f},{.98f,.38f},{.98f,.52f},{.1f,.2f}};
        static const float wl[][2] = {{-.1f,-.2f},{-.1f,.2f},{-.98f,.52f},{-.98f,.38f}};
        static const float tr[][2] = {{.1f,.62f},{.44f,.9f},{.44f,.99f},{.08f,.86f}};
        static const float tl[][2] = {{-.1f,.62f},{-.08f,.86f},{-.44f,.99f},{-.44f,.9f}};
        poly(wr, 4); poly(wl, 4); poly(tr, 4); poly(tl, 4); poly(fus, 11);
        // engines under the wings
        int ex, ey, fx, fy;
        P(.4f, .12f, &ex, &ey); P(.4f, .3f, &fx, &fy); ln(g, ex, ey, fx, fy, s * 0.08f, c);
        P(-.4f, .12f, &ex, &ey); P(-.4f, .3f, &fx, &fy); ln(g, ex, ey, fx, fy, s * 0.08f, c);
        // cockpit windscreen
        P(0, -.62f, &ex, &ey); P(0, -.5f, &fx, &fy); ln(g, ex, ey, fx, fy, s * 0.05f, bgc);
        break;
      }
      case 15: // Finance
        ring(g, (int)cx, (int)cy, (int)s, c, th); ring(g, (int)cx, (int)cy, (int)s - 1, c, th);
        g.setFont(&FontB612Bold12); g.setTextDatum((lgfx::textdatum_t)D_MC); g.setTextColor(c); g.drawString("$", (int)cx, (int)cy + 1);
        break;
      case 3: // VFO A/B
        g.setFont(&FontB612Bold16); g.setTextDatum((lgfx::textdatum_t)D_MC); g.setTextColor(c); g.drawString("A/B", (int)cx, (int)cy);
        break;
      case 34: // Update
        g.fillArc((int)cx, (int)cy, (int)(s * 0.75f), (int)(s * 0.75f + r + 1), 330.f, 220.f + 360.f, c);
        g.fillTriangle((int)(cx + s * 0.95f), (int)(cy - s * 0.2f), (int)(cx + s * 0.4f), (int)(cy - s * 0.8f), (int)(cx + s * 0.1f), (int)(cy - s * 0.05f), c);
        break;
      case 18: // Brightness
        g.fillCircle((int)cx, (int)cy, (int)(s * 0.4f), c);
        for(int i = 0 ; i < 8 ; i++) { const float a = i * 0.7854f; ln(g, cx + s * 0.65f * cosf(a), cy + s * 0.65f * sinf(a), cx + s * 0.98f * cosf(a), cy + s * 0.98f * sinf(a), r, c); }
        break;
      case 22: // Sleep mode: moon
        g.fillCircle((int)cx, (int)cy, (int)(s * 0.85f), c); g.fillCircle((int)(cx + s * 0.4f), (int)(cy - s * 0.3f), (int)(s * 0.75f), bgc);
        break;
      case 39: // Language: globe
      {
        ring(g, (int)cx, (int)cy, (int)s, c, th);
        float px = cx, py = cy - s;
        for(int i = 1 ; i <= 24 ; i++) { const float a = i * 6.2832f / 24; const float x = cx + s * 0.45f * sinf(a), y = cy - s * cosf(a); ln(g, px, py, x, y, r * 0.8f, c); px = x; py = y; }
        ln(g, cx - s, cy, cx + s, cy, r * 0.8f, c);
        break;
      }
      case 24: case 43: // Theme / menu colour: paint dots
        g.fillCircle((int)(cx - s * 0.45f), (int)(cy + s * 0.3f), (int)(s * 0.45f), c); g.fillCircle((int)(cx + s * 0.45f), (int)(cy + s * 0.3f), (int)(s * 0.45f), ckMix(bgc, c, 160));
        g.fillCircle((int)cx, (int)(cy - s * 0.4f), (int)(s * 0.45f), ckMix(bgc, c, 100));
        break;
      case 44: // Menu style: grid
        for(int i = 0 ; i < 2 ; i++) for(int j = 0 ; j < 2 ; j++) g.fillRoundRect((int)(cx - s + i * s * 1.1f), (int)(cy - s + j * s * 1.1f), (int)(s * 0.9f), (int)(s * 0.9f), 2, c);
        break;
      case 46: // News: newspaper (headline block, text lines)
        rrect(g, (int)(cx - s), (int)(cy - s * 0.8f), (int)(2 * s), (int)(1.6f * s), 3, c, th); rrect(g, (int)(cx - s) + 1, (int)(cy - s * 0.8f) + 1, (int)(2 * s) - 2, (int)(1.6f * s) - 2, 2, c, th);
        g.fillRect((int)(cx - s * 0.75f), (int)(cy - s * 0.52f), (int)(s * 0.8f), (int)(s * 0.55f), c);
        ln(g, cx + s * 0.25f, cy - s * 0.45f, cx + s * 0.75f, cy - s * 0.45f, r * 0.8f, c); ln(g, cx + s * 0.25f, cy - s * 0.15f, cx + s * 0.75f, cy - s * 0.15f, r * 0.8f, c);
        ln(g, cx - s * 0.75f, cy + s * 0.25f, cx + s * 0.75f, cy + s * 0.25f, r * 0.8f, c); ln(g, cx - s * 0.75f, cy + s * 0.55f, cx + s * 0.4f, cy + s * 0.55f, r * 0.8f, c);
        break;
      case 45: // About: i
        ring(g, (int)cx, (int)cy, (int)s, c, th); ring(g, (int)cx, (int)cy, (int)s - 1, c, th);
        g.fillCircle((int)cx, (int)(cy - s * 0.45f), (int)(r + 0.5f), c); ln(g, cx, cy - s * 0.1f, cx, cy + s * 0.55f, r, c);
        break;

      case 1: // Band: dial scale with pointer
        rrect(g, (int)(cx - s), (int)(cy - s * 0.6f), (int)(2 * s), (int)(1.2f * s), 3, c, th); rrect(g, (int)(cx - s) + 1, (int)(cy - s * 0.6f) + 1, (int)(2 * s) - 2, (int)(1.2f * s) - 2, 2, c, th);
        for(int i = 0 ; i <= 6 ; i++) { const float x = cx - s * 0.8f + i * s * 1.6f / 6; ln(g, x, cy + s * 0.5f, x, cy + s * (i & 1 ? 0.25f : 0.05f), r * 0.8f, c); }
        ln(g, cx + s * 0.2f, cy - s * 0.45f, cx + s * 0.2f, cy + s * 0.45f, r * 1.2f, c);
        break;
      case 5: // Seek: skip forward
        g.fillTriangle((int)(cx - s), (int)(cy - s * 0.6f), (int)(cx - s), (int)(cy + s * 0.6f), (int)(cx - s * 0.1f), (int)cy, c);
        g.fillTriangle((int)(cx - s * 0.1f), (int)(cy - s * 0.6f), (int)(cx - s * 0.1f), (int)(cy + s * 0.6f), (int)(cx + s * 0.8f), (int)cy, c);
        g.fillRect((int)(cx + s * 0.8f), (int)(cy - s * 0.6f), (int)(s * 0.2f) + 1, (int)(s * 1.2f), c);
        break;
      case 6: // Scan: radar
        ring(g, (int)cx, (int)cy, (int)s, c, th); ring(g, (int)cx, (int)cy, (int)s - 1, c, th); ring(g, (int)cx, (int)cy, (int)(s * 0.5f), c, th);
        ln(g, cx, cy, cx + s * 0.7f, cy - s * 0.7f, r * 1.2f, c); g.fillCircle((int)(cx + s * 0.3f), (int)(cy + s * 0.25f), (int)(r + 0.5f), c);
        break;
      case 16: // Network: three linked nodes
        ln(g, cx, cy - s * 0.65f, cx - s * 0.8f, cy + s * 0.55f, r, c); ln(g, cx, cy - s * 0.65f, cx + s * 0.8f, cy + s * 0.55f, r, c); ln(g, cx - s * 0.8f, cy + s * 0.55f, cx + s * 0.8f, cy + s * 0.55f, r, c);
        g.fillCircle((int)cx, (int)(cy - s * 0.65f), (int)(s * 0.28f), c); g.fillCircle((int)(cx - s * 0.8f), (int)(cy + s * 0.55f), (int)(s * 0.28f), c); g.fillCircle((int)(cx + s * 0.8f), (int)(cy + s * 0.55f), (int)(s * 0.28f), c);
        break;
      case 37: // Bluetooth rune
      {
        const float rr = r * 1.3f;
        ln(g, cx - s * 0.5f, cy - s * 0.5f, cx + s * 0.5f, cy + s * 0.5f, rr, c); ln(g, cx + s * 0.5f, cy + s * 0.5f, cx, cy + s, rr, c); ln(g, cx, cy + s, cx, cy - s, rr, c);
        ln(g, cx, cy - s, cx + s * 0.5f, cy - s * 0.5f, rr, c); ln(g, cx + s * 0.5f, cy - s * 0.5f, cx - s * 0.5f, cy + s * 0.5f, rr, c);
        break;
      }
      case 35: // USB trident
        g.fillCircle((int)(cx - s * 0.85f), (int)cy, (int)(s * 0.2f) + 1, c); ln(g, cx - s * 0.85f, cy, cx + s * 0.65f, cy, r * 1.2f, c);
        g.fillTriangle((int)(cx + s), (int)cy, (int)(cx + s * 0.6f), (int)(cy - s * 0.25f), (int)(cx + s * 0.6f), (int)(cy + s * 0.25f), c);
        ln(g, cx - s * 0.2f, cy, cx + s * 0.05f, cy - s * 0.5f, r, c); ln(g, cx + s * 0.05f, cy - s * 0.5f, cx + s * 0.4f, cy - s * 0.5f, r, c); g.fillRect((int)(cx + s * 0.4f), (int)(cy - s * 0.65f), (int)(s * 0.3f) + 1, (int)(s * 0.3f) + 1, c);
        ln(g, cx - s * 0.45f, cy, cx - s * 0.2f, cy + s * 0.5f, r, c); ln(g, cx - s * 0.2f, cy + s * 0.5f, cx + s * 0.3f, cy + s * 0.5f, r, c); g.fillCircle((int)(cx + s * 0.45f), (int)(cy + s * 0.5f), (int)(s * 0.2f) + 1, c);
        break;
      case 36: // TCP: two stacked servers
        for(int k = 0 ; k < 2 ; k++)
        {
          const float y = cy - s * 0.9f + k * s * 1.05f;
          rrect(g, (int)(cx - s), (int)y, (int)(2 * s), (int)(s * 0.75f), 2, c, th); rrect(g, (int)(cx - s) + 1, (int)y + 1, (int)(2 * s) - 2, (int)(s * 0.75f) - 2, 1, c, th);
          g.fillCircle((int)(cx - s * 0.6f), (int)(y + s * 0.375f), (int)(s * 0.14f) + 1, c); ln(g, cx - s * 0.2f, y + s * 0.375f, cx + s * 0.7f, y + s * 0.375f, r * 0.9f, c);
        }
        break;
      case 26: // Zoom: magnifier with plus
        ring(g, (int)(cx - s * 0.2f), (int)(cy - s * 0.2f), (int)(s * 0.65f), c, th); ring(g, (int)(cx - s * 0.2f), (int)(cy - s * 0.2f), (int)(s * 0.65f) - 1, c, th);
        ln(g, cx + s * 0.28f, cy + s * 0.28f, cx + s * 0.95f, cy + s * 0.95f, r * 1.6f, c);
        ln(g, cx - s * 0.5f, cy - s * 0.2f, cx + s * 0.1f, cy - s * 0.2f, r * 0.9f, c); ln(g, cx - s * 0.2f, cy - s * 0.5f, cx - s * 0.2f, cy + s * 0.1f, r * 0.9f, c);
        break;
      case 27: // Scroll direction: up and down arrows
        g.fillTriangle((int)cx, (int)(cy - s), (int)(cx - s * 0.55f), (int)(cy - s * 0.15f), (int)(cx + s * 0.55f), (int)(cy - s * 0.15f), c);
        g.fillTriangle((int)cx, (int)(cy + s), (int)(cx - s * 0.55f), (int)(cy + s * 0.15f), (int)(cx + s * 0.55f), (int)(cy + s * 0.15f), c);
        break;
      case 28: // RDS: label
        rrect(g, (int)(cx - s), (int)(cy - s * 0.6f), (int)(2 * s), (int)(1.2f * s), 3, c, th);
        g.setFont(s > 14 ? (const lgfx::IFont *)&FontB612Bold12 : (const lgfx::IFont *)&FontB612Reg10); g.setTextDatum((lgfx::textdatum_t)D_MC); g.setTextColor(c); g.drawString("RDS", (int)cx, (int)cy);
        break;
      case 29: // FM stereo: two overlapping rings
        for(int k = 0 ; k < 2 ; k++) { const int x = (int)(cx + (k ? 0.45f : -0.45f) * s); ring(g, x, (int)cy, (int)(s * 0.65f), c, th); ring(g, x, (int)cy, (int)(s * 0.65f) - 1, c, th); }
        break;
      case 30: // FM region: map pin
        g.fillCircle((int)cx, (int)(cy - s * 0.3f), (int)(s * 0.65f), c);
        g.fillTriangle((int)(cx - s * 0.58f), (int)(cy - s * 0.05f), (int)(cx + s * 0.58f), (int)(cy - s * 0.05f), (int)cx, (int)(cy + s), c);
        g.fillCircle((int)cx, (int)(cy - s * 0.3f), (int)(s * 0.25f), bgc);
        break;
      case 31: // Calibration: crosshair
        ring(g, (int)cx, (int)cy, (int)(s * 0.6f), c, th); ring(g, (int)cx, (int)cy, (int)(s * 0.6f) - 1, c, th);
        ln(g, cx - s, cy, cx - s * 0.3f, cy, r, c); ln(g, cx + s * 0.3f, cy, cx + s, cy, r, c); ln(g, cx, cy - s, cx, cy - s * 0.3f, r, c); ln(g, cx, cy + s * 0.3f, cx, cy + s, r, c);
        g.fillCircle((int)cx, (int)cy, (int)(r + 0.5f), c);
        break;
      case 32: // DSP patches: chip
        for(int i = -1 ; i <= 1 ; i++) { const float o = i * s * 0.4f; ln(g, cx + o, cy - s * 0.55f, cx + o, cy - s, r, c); ln(g, cx + o, cy + s * 0.55f, cx + o, cy + s, r, c); ln(g, cx - s * 0.55f, cy + o, cx - s, cy + o, r, c); ln(g, cx + s * 0.55f, cy + o, cx + s, cy + o, r, c); }
        g.fillRoundRect((int)(cx - s * 0.55f), (int)(cy - s * 0.55f), (int)(s * 1.1f), (int)(s * 1.1f), 2, c); g.fillRect((int)(cx - s * 0.25f), (int)(cy - s * 0.25f), (int)(s * 0.5f) + 1, (int)(s * 0.5f) + 1, bgc);
        break;
      case 33: // Load EiBi: download into tray
        ln(g, cx, cy - s, cx, cy + s * 0.05f, r * 1.3f, c);
        g.fillTriangle((int)(cx - s * 0.55f), (int)(cy - s * 0.1f), (int)(cx + s * 0.55f), (int)(cy - s * 0.1f), (int)cx, (int)(cy + s * 0.55f), c);
        ln(g, cx - s * 0.9f, cy + s * 0.5f, cx - s * 0.9f, cy + s * 0.9f, r, c); ln(g, cx - s * 0.9f, cy + s * 0.9f, cx + s * 0.9f, cy + s * 0.9f, r, c); ln(g, cx + s * 0.9f, cy + s * 0.9f, cx + s * 0.9f, cy + s * 0.5f, r, c);
        break;
      case 19: // Date/Time: calendar
        rrect(g, (int)(cx - s), (int)(cy - s * 0.8f), (int)(2 * s), (int)(1.8f * s), 3, c, th); rrect(g, (int)(cx - s) + 1, (int)(cy - s * 0.8f) + 1, (int)(2 * s) - 2, (int)(1.8f * s) - 2, 2, c, th);
        g.fillRect((int)(cx - s), (int)(cy - s * 0.8f), (int)(2 * s), (int)(s * 0.5f), c);
        ln(g, cx - s * 0.5f, cy - s, cx - s * 0.5f, cy - s * 0.6f, r * 1.2f, c); ln(g, cx + s * 0.5f, cy - s, cx + s * 0.5f, cy - s * 0.6f, r * 1.2f, c);
        for(int i = 0 ; i < 3 ; i++) for(int j = 0 ; j < 2 ; j++) g.fillRect((int)(cx - s * 0.65f + i * s * 0.5f), (int)(cy - s * 0.05f + j * s * 0.45f), (int)(s * 0.28f) + 1, (int)(s * 0.28f) + 1, c);
        break;
      case 20: // UTC offset: clock
        ring(g, (int)cx, (int)cy, (int)s, c, th); ring(g, (int)cx, (int)cy, (int)s - 1, c, th);
        ln(g, cx, cy, cx, cy - s * 0.65f, r * 1.1f, c); ln(g, cx, cy, cx + s * 0.45f, cy + s * 0.2f, r * 1.1f, c);
        break;
      case 21: // Sleep timer: hourglass
        g.fillTriangle((int)(cx - s * 0.7f), (int)(cy - s), (int)(cx + s * 0.7f), (int)(cy - s), (int)cx, (int)cy, c);
        g.fillTriangle((int)(cx - s * 0.7f), (int)(cy + s), (int)(cx + s * 0.7f), (int)(cy + s), (int)cx, (int)cy, c);
        g.fillTriangle((int)(cx - s * 0.4f), (int)(cy - s * 0.8f), (int)(cx + s * 0.4f), (int)(cy - s * 0.8f), (int)cx, (int)(cy - s * 0.15f), bgc);
        ln(g, cx - s * 0.85f, cy - s, cx + s * 0.85f, cy - s, r * 1.2f, c); ln(g, cx - s * 0.85f, cy + s, cx + s * 0.85f, cy + s, r * 1.2f, c);
        break;
      case 23: // Off timer: power
        g.fillArc((int)cx, (int)cy, (int)(s * 0.8f), (int)(s * 0.8f + r * 1.6f + 1), 310.f, 590.f, c);
        ln(g, cx, cy - s, cx, cy - s * 0.1f, r * 1.4f, c);
        break;
      case 25: // UI layout: panels
        rrect(g, (int)(cx - s), (int)(cy - s * 0.8f), (int)(2 * s), (int)(1.6f * s), 3, c, th); rrect(g, (int)(cx - s) + 1, (int)(cy - s * 0.8f) + 1, (int)(2 * s) - 2, (int)(1.6f * s) - 2, 2, c, th);
        ln(g, cx - s * 0.15f, cy - s * 0.8f, cx - s * 0.15f, cy + s * 0.8f, r, c); ln(g, cx - s * 0.15f, cy, cx + s, cy, r, c);
        break;
      case 40: // Accent: drop
        g.fillCircle((int)cx, (int)(cy + s * 0.3f), (int)(s * 0.62f), c);
        g.fillTriangle((int)(cx - s * 0.58f), (int)(cy + s * 0.1f), (int)(cx + s * 0.58f), (int)(cy + s * 0.1f), (int)cx, (int)(cy - s), c);
        break;
      case 41: // Text colour: letter with colour bar
        g.setFont(s > 14 ? (const lgfx::IFont *)&FontB612Bold16 : (const lgfx::IFont *)&FontB612Bold12); g.setTextDatum((lgfx::textdatum_t)D_MC); g.setTextColor(c); g.drawString("A", (int)cx, (int)(cy - s * 0.15f));
        g.fillRect((int)(cx - s * 0.8f), (int)(cy + s * 0.7f), (int)(s * 1.6f), (int)(s * 0.25f) + 1, c);
        break;
      case 42: // Background: picture
        rrect(g, (int)(cx - s), (int)(cy - s * 0.8f), (int)(2 * s), (int)(1.6f * s), 3, c, th); rrect(g, (int)(cx - s) + 1, (int)(cy - s * 0.8f) + 1, (int)(2 * s) - 2, (int)(1.6f * s) - 2, 2, c, th);
        g.fillTriangle((int)(cx - s * 0.8f), (int)(cy + s * 0.6f), (int)(cx - s * 0.2f), (int)(cy - s * 0.2f), (int)(cx + s * 0.3f), (int)(cy + s * 0.6f), c);
        g.fillTriangle((int)cx, (int)(cy + s * 0.6f), (int)(cx + s * 0.5f), (int)(cy + s * 0.05f), (int)(cx + s * 0.9f), (int)(cy + s * 0.6f), c);
        g.fillCircle((int)(cx + s * 0.5f), (int)(cy - s * 0.4f), (int)(s * 0.2f) + 1, c);
        break;
      default: // 17 and the rest: gear
      {
        for(int i = 0 ; i < 8 ; i++) { const float a = i * 0.7854f; ln(g, cx + s * 0.6f * cosf(a), cy + s * 0.6f * sinf(a), cx + s * 0.98f * cosf(a), cy + s * 0.98f * sinf(a), r * 1.5f, c); }
        g.fillCircle((int)cx, (int)cy, (int)(s * 0.72f), c); g.fillCircle((int)cx, (int)cy, (int)(s * 0.3f), bgc);
        break;
      }
    }
  }

  // Anti-aliased icon: drawn larger into a small sprite and averaged down (cards behind icons are flat colours).
  // The finished icon is kept in a small cache (PSRAM), so scrolling back and forth or redrawing the same page costs only a blit.
  // The CPU runs at 80 MHz, so the work per frame matters: 2x supersampling for small icons, 3x only for the big centre icon.
  template<class G> static void iconAA(G &g, int id, float cx, float cy, float s, uint16_t c, uint16_t bgc)
  {
    constexpr int KMAX = 3, MAXD = 60, NC = 12;
    struct Ent { int16_t id, half; uint16_t c, bgc; uint16_t *px; };
    static Ent cache[NC];
    static int nextEnt = 0;
    static lgfx::LGFX_Sprite *t = nullptr;
    static bool tried = false;
    if(!tried)
    {
      tried = true;
      t = new lgfx::LGFX_Sprite();
      t->setColorDepth(16);
      t->setPsram(true);
      if(!t->createSprite(MAXD * KMAX, MAXD * KMAX)) { delete t; t = nullptr; }
    }
    const int half = (int)(s * 1.15f) + 2, D = half * 2;
    if(!t || D > MAXD || id == 3 || id == 15 || id == 28 || id == 41) { icon(g, id, cx, cy, s, c, bgc); return; }   // text icons stay as they are
    const int ox = (int)lroundf(cx) - half, oy = (int)lroundf(cy) - half;
    Ent *e = nullptr;
    for(int i = 0 ; i < NC ; i++) if(cache[i].px && cache[i].id == id && cache[i].half == half && cache[i].c == c && cache[i].bgc == bgc) { e = &cache[i]; break; }
    if(!e)
    {
      const int K = (s >= 20.f) ? 3 : 2;
      e = &cache[nextEnt]; nextEnt = (nextEnt + 1) % NC;
      if(!e->px) e->px = (uint16_t *)lgfx::heap_alloc_psram(MAXD * MAXD * sizeof(uint16_t));
      if(!e->px) { icon(g, id, cx, cy, s, c, bgc); return; }
      e->id = id; e->half = half; e->c = c; e->bgc = bgc;
      t->fillRect(0, 0, D * K, D * K, bgc);
      icon(*t, id, D * K / 2.0f, D * K / 2.0f, s * K, c, bgc);
      for(int y = 0 ; y < D ; y++)
        for(int x = 0 ; x < D ; x++)
        {
          int r = 0, gg = 0, b = 0;
          for(int j = 0 ; j < K ; j++) for(int i = 0 ; i < K ; i++)
          {
            const uint32_t p = t->readPixel(x * K + i, y * K + j);
            r += (p >> 11) & 31; gg += (p >> 5) & 63; b += p & 31;
          }
          r /= K * K; gg /= K * K; b /= K * K;
          e->px[y * D + x] = (uint16_t)((r << 11) | (gg << 5) | b);
        }
    }
    for(int y = 0 ; y < D ; y++)
      for(int x = 0 ; x < D ; x++)
      {
        const uint16_t o = e->px[y * D + x];
        if(o != bgc) g.drawPixel(ox + x, oy + y, o);
      }
  }

  // Menu accent: the screen's accent or its own
  static inline NuPal menuPal(uint8_t menuColor, uint8_t accent, uint8_t textWhite, uint8_t bg)
  {
    return nuPalette(menuColor ? menuColor - 1 : accent, textWhite, bg);
  }

  // Menu labels per language (ASCII only: the screen fonts have no accents, as in the other translated screens).
  // lang: 0 Portuguese, 1 English, 2 Spanish (same order as uiLang). shortForm = the tile label of the grid.
  struct MenuTr { const char *en, *pt, *es, *ptS, *esS; };
  static inline const char *menuLabel(const char *en, int lang, bool shortForm)
  {
    static const MenuTr T[] = {
      {"Menu", "Menu", "Menu", 0, 0}, {"Visual", "Visual", "Visual", 0, 0}, {"Settings", "Configuracoes", "Ajustes", "Config.", 0},
      {"Mode", "Modo", "Modo", 0, 0}, {"Band", "Banda", "Banda", 0, 0}, {"Volume", "Volume", "Volumen", 0, 0}, {"VFO A/B", "VFO A/B", "VFO A/B", 0, 0},
      {"Step", "Passo", "Paso", 0, 0}, {"Seek", "Busca", "Busqueda", 0, 0}, {"Scan", "Varredura", "Escaneo", 0, 0}, {"Memory", "Memoria", "Memoria", 0, 0},
      {"Squelch", "Squelch", "Squelch", 0, 0}, {"Bandwidth", "Largura de banda", "Ancho de banda", "Largura", "Ancho"}, {"AGC/ATTN", "AGC/ATT", "AGC/ATT", 0, 0},
      {"AVC", "AVC", "AVC", 0, 0}, {"SoftMute", "SoftMute", "SoftMute", 0, 0}, {"Flights", "Voos", "Vuelos", 0, 0}, {"News", "Noticias", "Noticias", 0, 0}, {"Weather", "Clima", "Clima", 0, 0},
      {"Finance", "Financas", "Finanzas", 0, 0}, {"Network", "Rede", "Red", 0, 0},
      {"Brightness", "Brilho", "Brillo", 0, 0}, {"Date/Time", "Data/Hora", "Fecha/Hora", 0, 0}, {"UTC Offset", "Fuso horario", "Huso horario", "Fuso", "Huso"},
      {"Sleep Timer", "Timer de sono", "Temp. de sueno", "Sono", "Sueno"}, {"Sleep Mode", "Modo de sono", "Modo de sueno", "Modo sono", "Modo sueno"},
      {"Off Timer", "Timer desligar", "Temp. apagado", "Desligar", "Apagado"}, {"Zoom Menu", "Menu zoom", "Menu zoom", "Zoom", "Zoom"},
      {"Scroll Dir.", "Sentido do giro", "Sentido del giro", "Giro", "Giro"}, {"RDS", "RDS", "RDS", 0, 0}, {"FM Stereo", "FM Estereo", "FM Estereo", "Estereo", "Estereo"},
      {"FM Region", "Regiao FM", "Region FM", "Regiao", "Region"}, {"Calibration", "Calibracao", "Calibracion", "Calibr.", "Calibr."},
      {"DSP Patches", "Patches DSP", "Parches DSP", "DSP", "DSP"}, {"Load EiBi", "Carregar EiBi", "Cargar EiBi", "EiBi", "EiBi"},
      {"Update FW", "Atualizar FW", "Actualizar FW", "Atualiz.", "Actual."}, {"USB Port", "Porta USB", "Puerto USB", "USB", "USB"}, {"TCP Port", "Porta TCP", "Puerto TCP", "TCP", "TCP"},
      {"Bluetooth", "Bluetooth", "Bluetooth", 0, 0}, {"Wi-Fi", "Wi-Fi", "Wi-Fi", 0, 0}, {"Language", "Idioma", "Idioma", 0, 0}, {"About", "Sobre", "Acerca de", 0, 0},
      {"UI Layout", "Layout da tela", "Diseno de pantalla", "Layout", "Diseno"}, {"Theme", "Tema", "Tema", 0, 0}, {"Accent", "Destaque", "Acento", 0, 0},
      {"Text color", "Cor do texto", "Color de texto", "Texto", "Texto"}, {"Background", "Fundo", "Fondo", 0, 0},
      {"Menu color", "Cor do menu", "Color del menu", "Cor menu", "Col. menu"}, {"Menu style", "Estilo do menu", "Estilo del menu", "Estilo", "Estilo"},
    };
    for(unsigned i = 0 ; i < sizeof(T) / sizeof(T[0]) ; i++)
      if(!strcmp(en, T[i].en))
      {
        if(lang == 0) return shortForm && T[i].ptS ? T[i].ptS : T[i].pt;
        if(lang == 2) return shortForm && T[i].esS ? T[i].esS : T[i].es;
        return en;
      }
    return en;
  }

  // Carousel: big round icon in the middle, smaller neighbours, dots
  template<class G> static void menuCarousel(G &g, const NuPal &p, const char *title, const char *const *items, int count, int idx, int lang = 1)
  {
    bgFill(g, p);
    text(g, FontB612Bold12, menuLabel(title, lang, false), 8, 3, p.muted, D_TL);
    char pg[12]; snprintf(pg, sizeof(pg), "%d/%d", idx + 1, count); text(g, FontB612Bold12, pg, 312, 3, p.muted, D_TR);
    const int cy = 74;
    for(int k = -2 ; k <= 2 ; k++)
    {
      const int j = idx + k; if(j < 0 || j >= count || k == 0) continue;
      const int rad = abs(k) == 1 ? 26 : 17;
      const int cx = 160 + (abs(k) == 1 ? k * 100 : (k > 0 ? 100 + 72 : -100 - 72));
      g.fillSmoothCircle(cx, cy, rad, ckMix(p.bg1, 0xFFFF, abs(k) == 1 ? 30 : 18));
      ringAA(g, cx, cy, rad, ckMix(p.bg1, 0xFFFF, abs(k) == 1 ? 70 : 40));
      iconAA(g, iconId(items[j]), cx, cy, rad * 0.55f, ckMix(p.bg1, 0xFFFF, abs(k) == 1 ? 150 : 90), ckMix(p.bg1, 0xFFFF, abs(k) == 1 ? 30 : 18));
    }
    glowCircle(g, 160, cy, 46, p.acc, p.bg1);
    g.fillSmoothCircle(160, cy, 46, ckMix(p.bg1, 0xFFFF, 34));
    ringAA(g, 160, cy, 46, p.acc); ringAA(g, 160, cy, 45, p.acc); ringAA(g, 160, cy, 44, ckMix(p.bg1, p.acc, 150));
    iconAA(g, iconId(items[idx]), 160, cy, 24, ckMix(p.acc, 0xFFFF, 60), ckMix(p.bg1, 0xFFFF, 34));
    text(g, FontB612Bold16, menuLabel(items[idx], lang, false), 160, 140, p.txt, D_MC);
    const int dots = count > 24 ? 24 : count, step = 7;
    for(int k = 0 ; k < dots ; k++)
    {
      const bool on = (count > 24) ? (idx * dots / count == k) : (k == idx);
      g.fillSmoothCircle(160 + (int)((k - (dots - 1) / 2.f) * step), 161, on ? 2 : 1, on ? p.acc : ckMix(p.bg1, 0xFFFF, 90));
    }
  }

  static inline const char *shortName(const char *n)
  {
    static const char *const m[][2] = {{"Brightness", "Bright."}, {"UTC Offset", "UTC"}, {"Sleep Timer", "Sleep T."}, {"Sleep Mode", "Sleep M."}, {"Off Timer", "Off T."},
      {"Scroll Dir.", "Scroll"}, {"Calibration", "Calib."}, {"DSP Patches", "DSP"}, {"Load EiBi", "EiBi"}, {"Text color", "Text col."}, {"Menu color", "Menu col."},
      {"Menu style", "Menu sty."}, {"Background", "Backgr."}, {"AGC/ATTN", "AGC"}, {"Bandwidth", "BW"}, {"FM Region", "Region"}, {"FM Stereo", "Stereo"}};
    for(unsigned i = 0 ; i < sizeof(m) / sizeof(m[0]) ; i++) if(!strcmp(n, m[i][0])) return m[i][1];
    return n;
  }

  // Grid: 4x2 tiles per page
  template<class G> static void menuGrid(G &g, const NuPal &p, const char *title, const char *const *items, int count, int idx, int lang = 1)
  {
    bgFill(g, p);
    const int pages = (count + 7) / 8, page = idx / 8;
    // top bar: title, selected name (small) and page
    const char *ttl = menuLabel(title, lang, false);
    text(g, FontB612Bold12, ttl, 8, 3, p.muted, D_TL);
    g.setFont(&FontB612Bold12);
    const int tx = 8 + g.textWidth(ttl) + 10;
    text(g, FontB612Bold12, menuLabel(items[idx], lang, false), tx, 3, p.acc, D_TL);
    char pg[12]; snprintf(pg, sizeof(pg), "%d/%d", page + 1, pages); text(g, FontB612Bold12, pg, 312, 3, p.muted, D_TR);
    for(int i = 0 ; i < 8 ; i++)
    {
      const int j = page * 8 + i; if(j >= count) break;
      const int x = 8 + (i % 4) * 77, y = 20 + (i / 4) * 76, w = 73, h = 72;
      const bool sel = (j == idx);
      if(sel) glowRound(g, x, y, w, h, 8, p.acc, p.bg1);
      card(g, x, y, w, h, 8, p, sel);
      iconAA(g, iconId(items[j]), x + w / 2, y + 28, 17, sel ? ckMix(p.acc, 0xFFFF, 60) : p.txt2, sel ? ckMix(p.card, p.acc, 55) : p.card);
      char lb[20]; strncpy(lb, lang == 1 ? shortName(items[j]) : menuLabel(items[j], lang, true), 19); lb[19] = 0;
      g.setFont(&FontB612Reg10);
      while(strlen(lb) > 3 && g.textWidth(lb) > w - 4) lb[strlen(lb) - 1] = 0;
      text(g, FontB612Reg10, lb, x + w / 2, y + 60, sel ? p.txt : p.muted, D_MC);
    }
  }
}
