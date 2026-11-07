#pragma once
//
// Traffic radar looks for the Flights screen, drawn like cockpit traffic displays:
//   1 A32X  : airliner navigation display with a white ring (hollow diamonds, relative altitude)
//   2 ATCAS : grey-bezel TCAS indicator with dotted cyan rings
//   3 BTCAS : bronze-bezel TCAS indicator with threat colours (red square, yellow circle, cyan diamonds)
//   4 B7X   : 737-style compass rose with green traffic squares
// Header-only and templated on the graphics type (LGFX_Sprite on the radio, the same sprite in the PC preview).
// Data comes from the same ADS-B list as the map; nothing here uses the network.
//
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace rd
{
struct Ac
{
  float distNm, bearing;      // From the observer; bearing in radians, clockwise from north
  int altFt;                  // -1 = on the ground
  int vs; bool hasVs;         // ft/min
  int track, speedKt;
  char callsign[9];
  char type[5];
};
struct In
{
  const Ac *ac;
  int n, total, sel, radius;  // Listed aircraft, aircraft inside the radius, selected index, radius in nm
  const char *label;          // Observer label (e.g. SJK)
  const char *status;         // "" when fine
  float t;
};

static inline uint16_t rgb(int r, int g, int b) { return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)); }
static inline uint16_t mix(uint16_t a, uint16_t b, int k)
{
  const int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31, br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  return (uint16_t)((((ar * (255 - k) + br * k) / 255) << 11) | (((ag * (255 - k) + bg * k) / 255) << 5) | ((ab * (255 - k) + bb * k) / 255));
}

// 0 other, 1 proximate (within 10 nm), 2 traffic advisory (5 nm, below 3000 ft), 3 resolution (2 nm, below 1500 ft), -1 on the ground.
// The observer is on the ground, so the relative altitude is the altitude itself.
static int threat(const Ac &a)
{
  if(a.altFt < 0) return -1;
  if(a.distNm <= 2.0f && a.altFt <= 1500) return 3;
  if(a.distNm <= 5.0f && a.altFt <= 3000) return 2;
  if(a.distNm <= 10.0f) return 1;
  return 0;
}
// Relative altitude in hundreds of feet: +NN under 10,000 ft, flight level from there on
static void altLabel(char *o, size_t n, int altFt)
{
  if(altFt >= 10000) snprintf(o, n, "%03d", (altFt + 50) / 100);
  else snprintf(o, n, "+%02d", (altFt + 50) / 100);
}

template<class G, class F>
static void txt(G &g, const F &f, const char *s, int x, int y, uint16_t col, int datum = 0)
{
  g.setFont(&f);
  g.setTextDatum((lgfx::textdatum_t)datum);
  g.setTextColor(col);
  g.drawString(s, x, y);
}

// Anti-aliased ring on a black ground (the radar screens are always black behind the rings)
template<class G>
static void ring(G &g, int cx, int cy, int r, float w, uint16_t col)
{
  g.fillSmoothCircle(cx, cy, r, col);
  g.fillSmoothCircle(cx, cy, (int)lroundf(r - w), 0x0000);
}
template<class G>
static void dot(G &g, int x, int y, uint16_t col) { g.fillSmoothCircle(x, y, 1, col); }

// Label placement: every label tries a few spots around its symbol and takes the first that touches no other label or symbol.
struct Box { int x0, y0, x1, y1; };
static inline bool hit(const Box &a, const Box &b) { return a.x0 < b.x1 && b.x0 < a.x1 && a.y0 < b.y1 && b.y0 < a.y1; }
struct Placer
{
  Box taken[64]; int nTaken = 0;
  Box area;
  void add(const Box &b) { if(nTaken < 64) taken[nTaken++] = b; }
  bool free(const Box &b) const
  {
    if(b.x0 < area.x0 || b.y0 < area.y0 || b.x1 > area.x1 || b.y1 > area.y1) return false;
    for(int i = 0 ; i < nTaken ; i++) if(hit(b, taken[i])) return false;
    return true;
  }
  static int overlap(const Box &a, const Box &b)
  {
    const int w = (a.x1 < b.x1 ? a.x1 : b.x1) - (a.x0 > b.x0 ? a.x0 : b.x0), h = (a.y1 < b.y1 ? a.y1 : b.y1) - (a.y0 > b.y0 ? a.y0 : b.y0);
    return (w > 0 && h > 0) ? w * h : 0;
  }
  // (x, y) = symbol centre, w x h = text size; order selects the preferred spots (8 around the symbol, then 8 further out).
  // The first spot that touches nothing wins; in a crowd the one with the least overlap does. Returns the top-left of the text.
  void place(int x, int y, int w, int h, bool arrow, const int8_t *order, int *lx, int *ly)
  {
    const int ar = arrow ? 9 : 0;
    int ox[16] = {-w / 2, -w / 2, 9 + ar, -w - 9, 8, -w - 8, 8, -w - 8, 0, 0, 0, 0, 0, 0, 0, 0};
    int oy[16] = {-h - 7, 8, -h / 2, -h / 2, -h - 6, -h - 6, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0};
    for(int c = 0 ; c < 8 ; c++)          // Second ring: the same directions, 11 px further
    {
      const int dx = (ox[c] + w / 2 > 0) ? 1 : (ox[c] + w / 2 < 0 ? -1 : 0), dy = (oy[c] + h / 2 > 0) ? 1 : -1;
      ox[8 + c] = ox[c] + dx * 11; oy[8 + c] = oy[c] + dy * 11;
    }
    int best = -1, bestCost = 1 << 30;
    for(int k = 0 ; k < 16 ; k++)
    {
      const int c = k < 8 ? order[k] : 8 + order[k - 8];
      const Box b{x + ox[c], y + oy[c], x + ox[c] + w, y + oy[c] + h};
      int cost = 0;
      if(b.x0 < area.x0 || b.y0 < area.y0 || b.x1 > area.x1 || b.y1 > area.y1) cost += 2000;
      for(int i = 0 ; i < nTaken ; i++) cost += overlap(b, taken[i]) * 6;
      cost += k;                                  // Mild preference for the earlier spots
      if(cost < bestCost) { bestCost = cost; best = c; }
      if(cost < 16) break;                        // Free spot (only the preference cost)
    }
    *lx = x + ox[best]; *ly = y + oy[best];
    add(Box{*lx, *ly, *lx + w, *ly + h});
  }
};
static const int8_t ORDER_ABOVE[8] = {0, 1, 4, 5, 2, 3, 6, 7};
static const int8_t ORDER_RIGHT[8] = {2, 4, 6, 0, 1, 3, 5, 7};

template<class G>
static void diamond(G &g, int x, int y, int r, uint16_t col, bool filled)
{
  if(filled) { g.fillTriangle(x, y - r, x + r, y, x, y + r, col); g.fillTriangle(x, y - r, x - r, y, x, y + r, col); }
  g.drawWideLine(x, y - r, x + r, y, 0.8f, col); g.drawWideLine(x + r, y, x, y + r, 0.8f, col);
  g.drawWideLine(x, y + r, x - r, y, 0.8f, col); g.drawWideLine(x - r, y, x, y - r, 0.8f, col);
}
template<class G>
static void vArrow(G &g, int x, int y, bool up, uint16_t col)   // Centre at x,y; 11 px tall
{
  const int d = up ? -1 : 1;
  g.drawFastVLine(x, y - 5, 11, col);
  g.drawLine(x, y + 5 * d, x - 2, y + 2 * d, col);
  g.drawLine(x, y + 5 * d, x + 2, y + 2 * d, col);
}
template<class G>
static void ownship(G &g, int cx, int cy, uint16_t col)   // Small aeroplane seen from above
{
  g.drawWideLine(cx, cy - 8, cx, cy + 8, 1.0f, col);
  g.drawWideLine(cx - 8, cy - 1, cx + 8, cy - 1, 1.0f, col);
  g.drawWideLine(cx - 4, cy + 7, cx + 4, cy + 7, 1.0f, col);
}
template<class G>
static void ringTicks(G &g, int cx, int cy, int r, int n, int len, int longLen, uint16_t col, int longEvery)
{
  for(int i = 0 ; i < n ; i++)
  {
    const float a = i * 2.0f * (float)M_PI / n, s = sinf(a), c = cosf(a);
    const int l = (longEvery && i % longEvery == 0) ? longLen : len;
    g.drawWideLine(cx + (int)lroundf(s * r), cy - (int)lroundf(c * r), cx + (int)lroundf(s * (r - l)), cy - (int)lroundf(c * (r - l)), 0.7f, col);
  }
}
template<class G>
static void dotRing(G &g, int cx, int cy, int r, int stepDeg, uint16_t col)
{
  for(int d = 0 ; d < 360 ; d += stepDeg)
  {
    const float a = d * (float)M_PI / 180.0f;
    g.fillSmoothCircle(cx + (int)lroundf(sinf(a) * r), cy - (int)lroundf(cosf(a) * r), 1, col);
  }
}
static inline void pos(const Ac &a, float k, int cx, int cy, int *x, int *y)
{
  const float r = a.distNm * k;
  *x = cx + (int)lroundf(r * sinf(a.bearing)); *y = cy - (int)lroundf(r * cosf(a.bearing));
}

// ---------------------------------------------------------------------------------------------
// Common pieces
// ---------------------------------------------------------------------------------------------
// Targets inside the radar disc, selected one last. Fills px/py/vis/lbl and reserves the symbol areas in the placer.
struct Layout
{
  int px[32], py[32]; bool vis[32], arrow[32]; char lbl[32][8];
  int lx[32], ly[32];
};
template<class G, class F>
static void layoutTargets(G &g, const F &font, const In &in, float k, int cx, int cy, int R, bool skipGround, const int8_t *order, Placer &pl, Layout &L)
{
  const int n = in.n < 32 ? in.n : 32;
  for(int i = 0 ; i < n ; i++)
  {
    const Ac &a = in.ac[i];
    L.vis[i] = a.distNm <= in.radius && !(skipGround && a.altFt < 0);
    if(!L.vis[i]) continue;
    pos(a, k, cx, cy, &L.px[i], &L.py[i]);
    L.arrow[i] = a.altFt >= 0 && a.hasVs && abs(a.vs) >= 500;
    if(a.altFt >= 0) altLabel(L.lbl[i], 8, a.altFt); else L.lbl[i][0] = 0;
    pl.add(Box{L.px[i] - 7, L.py[i] - 7, L.px[i] + 8 + (L.arrow[i] ? 8 : 0), L.py[i] + 8});
  }
  (void)R;
  g.setFont(&font);
  // Selected first (it gets the best spot), then nearest first
  int ord[32], m = 0;
  if(in.sel >= 0 && in.sel < n && L.vis[in.sel]) ord[m++] = in.sel;
  for(int i = 0 ; i < n ; i++) if(L.vis[i] && i != in.sel) ord[m++] = i;
  for(int j = 0 ; j < m ; j++)
  {
    const int i = ord[j];
    if(!L.lbl[i][0]) { L.lx[i] = L.ly[i] = -1000; continue; }
    const int w = g.textWidth(L.lbl[i]);
    pl.place(L.px[i], L.py[i], w, 11, L.arrow[i], order, &L.lx[i], &L.ly[i]);
  }
}

template<class G>
static void panelInfo(G &g, const In &in, int x0, int y0, int w, int h, uint16_t c1, uint16_t c2, uint16_t bg, uint16_t edge)
{
  g.fillSmoothRoundRect(x0, y0, w, h, 6, bg);
  g.drawRoundRect(x0, y0, w, h, 6, edge);
  char b[32];
  if(in.sel >= 0 && in.sel < in.n)
  {
    const Ac &s = in.ac[in.sel];
    txt(g, FontB612Bold16, s.callsign, x0 + 8, y0 + 6, c1, 0);
    if(s.type[0]) txt(g, FontB612Reg10, s.type, x0 + w - 8, y0 + 10, c2, 2);
    char al[16]; if(s.altFt < 0) snprintf(al, sizeof(al), "GND"); else snprintf(al, sizeof(al), "%d ft", s.altFt);
    txt(g, FontB612Reg12, al, x0 + 8, y0 + 30, c2, 0);
    snprintf(b, sizeof(b), "%d kt   %03d", s.speedKt, s.track); txt(g, FontB612Reg12, b, x0 + 8, y0 + 46, c2, 0);
    if(s.hasVs) snprintf(b, sizeof(b), "%+d fpm", s.vs); else snprintf(b, sizeof(b), "-");
    txt(g, FontB612Reg12, b, x0 + 8, y0 + 62, c2, 0);
    snprintf(b, sizeof(b), "%.1f nm", s.distNm); txt(g, FontB612Bold12, b, x0 + 8, y0 + 78, c1, 0);
  }
  else txt(g, FontB612Reg12, "--", x0 + w / 2, y0 + h / 2 - 6, c2, 1);
}
template<class G>
static void panelButtons(G &g, const In &in, int x0, int y0, int w, uint16_t face, uint16_t hi, uint16_t edge, uint16_t txtc)
{
  const char *lab[2] = {"RNG", "INFO"};
  char r[12]; snprintf(r, sizeof(r), "%d nm", in.radius);
  const int bw = (w - 8) / 2;
  for(int i = 0 ; i < 2 ; i++)
  {
    const int x = x0 + i * (bw + 8), y = y0, h = 36;
    g.fillSmoothRoundRect(x, y, bw, h, 8, face);
    g.drawFastHLine(x + 8, y + 1, bw - 16, hi);
    g.drawRoundRect(x, y, bw, h, 8, edge);
    txt(g, FontB612Bold12, lab[i], x + bw / 2, y + 7, txtc, 1);
    if(i == 0) txt(g, FontB612Reg10, r, x + bw / 2, y + 22, txtc, 1);
  }
}
template<class G>
static void emptyNote(G &g, const In &in, int cx, int y, uint16_t col)
{
  if(in.n > 0) return;
  txt(g, FontB612Reg10, (in.status && in.status[0]) ? in.status : "NO TRAFFIC", cx, y, col, 1);
}

// ---------------------------------------------------------------------------------------------
// 1  A32X: black screen, white ring, hollow diamonds
// ---------------------------------------------------------------------------------------------
template<class G>
static void a32x(G &g, const In &in)
{
  g.fillSprite(0x0000);
  const int cx = 160, cy = 86, R = 77;
  const uint16_t white = 0xFFFF, yg = rgb(190, 235, 40), blue = rgb(90, 190, 255), cyan = rgb(110, 235, 255);
  const float k = (float)R / in.radius;
  ring(g, cx, cy, R + 1, 2.6f, 0xFFFF);
  ringTicks(g, cx, cy, R - 3, 8, 6, 10, white, 2);
  ring(g, cx, cy, R / 2, 1.3f, rgb(215, 215, 225));
  ringTicks(g, cx, cy, R / 2, 4, 4, 4, white, 1);
  ownship(g, cx, cy, white);
  Placer pl; pl.area = Box{cx - R - 28, cy - R - 2, cx + R + 28, cy + R + 2};
  Layout L; layoutTargets(g, FontB612Bold12, in, k, cx, cy, R, true, ORDER_ABOVE, pl, L);
  const int n = in.n < 32 ? in.n : 32;
  for(int pass = 0 ; pass < 2 ; pass++)
    for(int i = 0 ; i < n ; i++)
    {
      if((i == in.sel) != (pass == 1) || !L.vis[i]) continue;
      const Ac &a = in.ac[i];
      const bool sel = i == in.sel;
      const uint16_t c = sel ? cyan : white;
      if(a.altFt < 0) { g.fillRect(L.px[i] - 1, L.py[i] - 1, 3, 3, mix(white, 0, 120)); continue; }
      diamond(g, L.px[i], L.py[i], 5, c, sel);
      txt(g, FontB612Bold12, L.lbl[i], L.lx[i], L.ly[i] - 1, c, 0);
      if(L.arrow[i]) vArrow(g, L.px[i] + 11, L.py[i], a.vs > 0, c);
    }
  char b[24];
  txt(g, FontB612Bold12, "T", 20, 54, yg, 1); txt(g, FontB612Bold12, "F", 20, 70, yg, 1); txt(g, FontB612Bold12, "C", 20, 86, yg, 1);
  snprintf(b, sizeof(b), "%03d", in.total > 999 ? 999 : in.total); txt(g, FontB612Bold16, b, 8, 8, yg, 0);
  snprintf(b, sizeof(b), "%03d", in.n > 999 ? 999 : in.n); txt(g, FontB612Bold16, b, 8, 28, yg, 0);
  if(in.sel >= 0 && in.sel < in.n)
  {
    const Ac &s = in.ac[in.sel];
    txt(g, FontB612Bold12, "GS", 312, 6, yg, 2); snprintf(b, sizeof(b), "%d", s.speedKt); txt(g, FontB612Bold12, b, 312, 21, yg, 2);
    txt(g, FontB612Bold12, "HDG", 312, 112, blue, 2); snprintf(b, sizeof(b), "%03d", s.track); txt(g, FontB612Bold12, b, 312, 127, blue, 2);
    txt(g, FontB612Reg10, s.callsign, 8, 152, white, 0);
  }
  txt(g, FontB612Bold12, "NM", 312, 152, yg, 2); snprintf(b, sizeof(b), "%d", in.radius); txt(g, FontB612Bold12, b, 288, 152, yg, 2);
  emptyNote(g, in, cx, cy + 30, yg);
}

// ---------------------------------------------------------------------------------------------
// 2  ATCAS: grey bezel, dotted cyan rings
// ---------------------------------------------------------------------------------------------
template<class G>
static void atcas(G &g, const In &in)
{
  const uint16_t b1 = rgb(124, 138, 162), b2 = rgb(86, 98, 120);
  for(int y = 0 ; y < 170 ; y++) g.drawFastHLine(0, y, 320, mix(b1, b2, y * 255 / 170));
  const uint16_t dark = rgb(34, 40, 52);
  g.fillTriangle(0, 0, 24, 0, 0, 24, dark); g.fillTriangle(319, 0, 295, 0, 319, 24, dark);
  g.fillTriangle(0, 169, 24, 169, 0, 145, dark); g.fillTriangle(319, 169, 295, 169, 319, 145, dark);
  g.drawLine(24, 0, 0, 24, rgb(170, 184, 206)); g.drawLine(295, 0, 319, 24, rgb(60, 70, 90));
  const int sx = 18, sy = 10, sw = 150, sh = 150, cx = sx + sw / 2, cy = sy + sh / 2, R = 62;
  g.fillSmoothRoundRect(sx - 4, sy - 4, sw + 8, sh + 8, 10, rgb(48, 58, 80));
  g.fillSmoothRoundRect(sx - 1, sy - 1, sw + 2, sh + 2, 8, rgb(20, 24, 34));
  g.fillSmoothRoundRect(sx, sy, sw, sh, 7, 0x0000);
  const uint16_t cy1 = rgb(60, 226, 230), white = 0xFFFF, yel = rgb(255, 230, 40);
  const float k = (float)R / in.radius;
  dotRing(g, cx, cy, R, 3, mix(cy1, 0, 70)); dotRing(g, cx, cy, R / 2, 6, mix(cy1, 0, 140));
  for(int i = 0 ; i < 12 ; i++)
  {
    const float a = i * (float)M_PI / 6.0f;
    const int x = cx + (int)lroundf(sinf(a) * R), y = cy - (int)lroundf(cosf(a) * R);
    const bool major = i % 3 == 0;
    g.fillRect(x - (major ? 1 : 2), y - (major ? 3 : 2), major ? 3 : 4, major ? 7 : 4, cy1);
  }
  ownship(g, cx, cy, cy1);
  Placer pl; pl.area = Box{sx + 2, sy + 2, sx + sw - 2, sy + sh - 2};
  pl.add(Box{sx + sw - 62, sy + 2, sx + sw - 2, sy + 28}); pl.add(Box{sx + sw - 50, sy + sh - 22, sx + sw - 2, sy + sh - 2});   // Range and mode texts
  Layout L; layoutTargets(g, FontB612Bold12, in, k, cx, cy, R, true, ORDER_ABOVE, pl, L);
  const int n = in.n < 32 ? in.n : 32;
  for(int pass = 0 ; pass < 2 ; pass++)
    for(int i = 0 ; i < n ; i++)
    {
      if((i == in.sel) != (pass == 1) || !L.vis[i] || in.ac[i].altFt < 0) continue;
      const Ac &a = in.ac[i];
      const int th = threat(a);
      const bool sel = i == in.sel;
      const uint16_t c = th >= 2 ? yel : white;
      if(th >= 2) g.fillSmoothCircle(L.px[i], L.py[i], 5, yel); else diamond(g, L.px[i], L.py[i], 5, white, sel);
      if(sel) g.drawCircle(L.px[i], L.py[i], 9, cy1);
      txt(g, FontB612Bold12, L.lbl[i], L.lx[i], L.ly[i] - 1, c, 0);
      if(L.arrow[i]) vArrow(g, L.px[i] + 11, L.py[i], a.vs > 0, c);
    }
  char b[12]; snprintf(b, sizeof(b), "%d", in.radius);
  txt(g, FontB612Bold16, b, sx + sw - 30, sy + 5, cy1, 2); txt(g, FontB612Reg10, "nm", sx + sw - 6, sy + 12, cy1, 2);
  txt(g, FontB612Bold12, "NORM", sx + sw - 6, sy + sh - 19, cy1, 2);
  emptyNote(g, in, cx, cy + 22, cy1);
  panelInfo(g, in, 184, 12, 126, 98, cy1, rgb(190, 215, 225), 0x0000, rgb(48, 58, 80));
  panelButtons(g, in, 184, 122, 126, rgb(22, 26, 36), rgb(70, 80, 100), rgb(60, 72, 96), rgb(225, 230, 240));
}

// ---------------------------------------------------------------------------------------------
// 3  BTCAS: bronze bezel, threat colours
// ---------------------------------------------------------------------------------------------
template<class G>
static void btcas(G &g, const In &in)
{
  const uint16_t b1 = rgb(172, 126, 98), b2 = rgb(96, 70, 50);
  for(int y = 0 ; y < 170 ; y++) g.drawFastHLine(0, y, 320, mix(b1, b2, y * 255 / 170));
  const int sx = 22, sy = 10, sw = 148, sh = 150, cx = sx + sw / 2, cy = sy + sh / 2, R = 63;
  g.fillSmoothRoundRect(sx - 4, sy - 4, sw + 8, sh + 8, 22, rgb(196, 160, 132));
  g.fillSmoothRoundRect(sx - 2, sy - 2, sw + 4, sh + 4, 20, rgb(64, 46, 34));
  g.fillSmoothRoundRect(sx, sy, sw, sh, 18, 0x0000);
  static const int scx[4] = {8, 312, 8, 312}, scy[4] = {8, 8, 162, 162};
  for(int i = 0 ; i < 4 ; i++)                                       // Screws at the four corners
  {
    g.fillSmoothCircle(scx[i], scy[i], 6, rgb(70, 48, 30)); g.fillSmoothCircle(scx[i] - 1, scy[i] - 1, 5, rgb(190, 142, 88)); g.fillSmoothCircle(scx[i] - 2, scy[i] - 2, 2, rgb(226, 190, 140));
    g.drawWideLine(scx[i] - 3, scy[i] - 3, scx[i] + 3, scy[i] + 3, 0.7f, rgb(48, 32, 20)); g.drawWideLine(scx[i] - 3, scy[i] + 3, scx[i] + 3, scy[i] - 3, 0.7f, rgb(48, 32, 20));
  }
  const uint16_t cyan = rgb(110, 235, 245), red = rgb(245, 60, 50), yel = rgb(255, 224, 40), grey = rgb(205, 205, 210);
  const float k = (float)R / in.radius;
  ring(g, cx, cy, R, 1.6f, mix(grey, 0, 50));
  for(int i = 0 ; i < 12 ; i++)
  {
    const float a = i * (float)M_PI / 6.0f;
    const bool major = i % 3 == 0;
    const int l = major ? 9 : 5, x0 = cx + (int)lroundf(sinf(a) * R), y0 = cy - (int)lroundf(cosf(a) * R);
    g.drawWideLine(x0, y0, cx + (int)lroundf(sinf(a) * (R - l)), cy - (int)lroundf(cosf(a) * (R - l)), 1.0f, grey);
  }
  dotRing(g, cx, cy, 20, 15, grey);
  ownship(g, cx, cy, 0xFFFF);
  Placer pl; pl.area = Box{sx + 4, sy + 4, sx + sw - 4, sy + sh - 4};
  pl.add(Box{sx + sw - 44, sy + 6, sx + sw - 6, sy + 42});
  Layout L; layoutTargets(g, FontB612Bold12, in, k, cx, cy, R, true, ORDER_ABOVE, pl, L);
  const int n = in.n < 32 ? in.n : 32;
  for(int pass = 0 ; pass < 2 ; pass++)
    for(int i = 0 ; i < n ; i++)
    {
      if((i == in.sel) != (pass == 1) || !L.vis[i] || in.ac[i].altFt < 0) continue;
      const Ac &a = in.ac[i];
      const int th = threat(a), x = L.px[i], y = L.py[i];
      uint16_t c = cyan;
      if(th == 3) { c = red; g.fillRect(x - 4, y - 4, 9, 9, c); }
      else if(th == 2) { c = yel; g.fillSmoothCircle(x, y, 5, c); }
      else if(th == 1) diamond(g, x, y, 5, c, true);
      else { c = mix(cyan, 0, 95); diamond(g, x, y, 5, c, false); }
      if(i == in.sel) g.drawCircle(x, y, 9, 0xFFFF);
      txt(g, FontB612Bold12, L.lbl[i], L.lx[i], L.ly[i] - 1, c, 0);
      if(L.arrow[i]) vArrow(g, x + 11, y, a.vs > 0, c);
    }
  char b[12]; snprintf(b, sizeof(b), "%d", in.radius);
  g.drawRect(sx + sw - 42, sy + 8, 34, 32, grey);
  txt(g, FontB612Bold12, b, sx + sw - 25, sy + 11, grey, 1); txt(g, FontB612Reg10, "nm", sx + sw - 25, sy + 25, grey, 1);
  emptyNote(g, in, cx, cy + 24, cyan);
  panelInfo(g, in, 184, 14, 124, 96, cyan, rgb(205, 210, 208), 0x0000, rgb(64, 46, 34));
  panelButtons(g, in, 184, 122, 124, rgb(46, 34, 26), rgb(120, 92, 70), rgb(150, 112, 84), rgb(235, 225, 215));
}

// ---------------------------------------------------------------------------------------------
// 4  B7X: compass rose (north up), green traffic squares, magenta line to the selected aircraft
// ---------------------------------------------------------------------------------------------
template<class G>
static void b7x(G &g, const In &in)
{
  g.fillSprite(0x0000);
  const int cx = 168, cy = 88, R = 72;
  const uint16_t white = 0xFFFF, green = rgb(70, 230, 90), mag = rgb(255, 80, 255), cyan = rgb(80, 220, 245), org = rgb(255, 150, 40), grn2 = rgb(120, 235, 70);
  const float k = (float)(R - 8) / in.radius;
  ring(g, cx, cy, R, 1.4f, white);
  for(int d = 0 ; d < 360 ; d += 5)
  {
    const float a = d * (float)M_PI / 180.0f;
    const int l = d % 30 == 0 ? 9 : (d % 10 == 0 ? 6 : 3);
    g.drawWideLine(cx + (int)lroundf(sinf(a) * R), cy - (int)lroundf(cosf(a) * R), cx + (int)lroundf(sinf(a) * (R - l)), cy - (int)lroundf(cosf(a) * (R - l)), d % 10 == 0 ? 0.7f : 0.45f, white);
  }
  static const struct { int d; const char *t; } lab[] = {{0, "N"}, {30, "3"}, {60, "6"}, {90, "E"}, {120, "12"}, {150, "15"}, {180, "S"}, {210, "21"}, {240, "24"}, {270, "W"}, {300, "30"}, {330, "33"}};
  for(const auto &l : lab)
  {
    const float a = l.d * (float)M_PI / 180.0f;
    txt(g, FontB612Reg10, l.t, cx + (int)lroundf(sinf(a) * (R - 18)), cy - (int)lroundf(cosf(a) * (R - 18)), white, 4 /*MC*/);
  }
  for(int d = 45 ; d < 360 ; d += 90)                                // Triangles at the diagonals
  {
    const float a = d * (float)M_PI / 180.0f, s = sinf(a), c = cosf(a);
    const int x = cx + (int)lroundf(s * (R + 5)), y = cy - (int)lroundf(c * (R + 5));
    g.drawTriangle(x + (int)lroundf(-c * 4 - s * 3), y + (int)lroundf(-s * 4 + c * 3), x + (int)lroundf(c * 4 - s * 3), y + (int)lroundf(s * 4 + c * 3), x + (int)lroundf(s * 4), y - (int)lroundf(c * 4), white);
  }
  const int Ri = (R - 8) / 2;
  ring(g, cx, cy, Ri, 1.2f, white);
  ringTicks(g, cx, cy, Ri, 8, 3, 3, white, 0);
  char b[16]; snprintf(b, sizeof(b), "%d", in.radius / 2);
  txt(g, FontB612Reg10, b, cx - Ri + 3, cy - Ri + 6, white, 0);
  if(in.sel >= 0 && in.sel < in.n && in.ac[in.sel].distNm <= in.radius)
  {
    int x, y; pos(in.ac[in.sel], k, cx, cy, &x, &y);
    g.drawWideLine(cx, cy, x, y, 0.7f, mag);
  }
  g.fillTriangle(cx, cy - 9, cx - 2, cy + 3, cx + 2, cy + 3, white);
  g.fillTriangle(cx - 9, cy + 1, cx + 9, cy + 1, cx, cy - 3, white);
  g.fillTriangle(cx - 4, cy + 8, cx + 4, cy + 8, cx, cy + 3, white);
  Placer pl; pl.area = Box{86, 2, 258, 168};
  for(const auto &l : lab)
  {
    const float a = l.d * (float)M_PI / 180.0f;
    const int lx = cx + (int)lroundf(sinf(a) * (R - 18)), ly = cy - (int)lroundf(cosf(a) * (R - 18)), lw = strlen(l.t) * 6 + 2;
    pl.add(Box{lx - lw / 2, ly - 6, lx + lw / 2, ly + 6});
  }
  Layout L; layoutTargets(g, FontB612Reg10, in, k, cx, cy, R, true, ORDER_RIGHT, pl, L);
  const int n = in.n < 32 ? in.n : 32;
  for(int pass = 0 ; pass < 2 ; pass++)
    for(int i = 0 ; i < n ; i++)
    {
      if((i == in.sel) != (pass == 1) || !L.vis[i] || in.ac[i].altFt < 0) continue;
      const Ac &a = in.ac[i];
      const int th = threat(a), x = L.px[i], y = L.py[i];
      const uint16_t c = th >= 2 ? org : green;
      g.fillRect(x - 4, y - 4, 9, 9, c);
      if(i == in.sel) g.drawRect(x - 7, y - 7, 15, 15, mag);
      txt(g, FontB612Reg10, L.lbl[i], L.lx[i], L.ly[i], th >= 2 ? org : white, 0);
      if(L.arrow[i]) vArrow(g, x + 11, y, a.vs > 0, c);
    }
  if(in.sel >= 0 && in.sel < in.n)
  {
    const Ac &s = in.ac[in.sel];
    txt(g, FontB612Bold12, s.callsign, 6, 6, mag, 0);
    txt(g, FontB612Reg10, s.type[0] ? s.type : "-", 6, 22, mag, 0);
    txt(g, FontB612Reg10, "GS", 6, 40, white, 0); snprintf(b, sizeof(b), "%d", s.speedKt); txt(g, FontB612Reg10, b, 40, 40, grn2, 0);
    txt(g, FontB612Reg10, "ALT", 6, 54, white, 0); if(s.altFt < 0) snprintf(b, sizeof(b), "GND"); else snprintf(b, sizeof(b), "%d", s.altFt); txt(g, FontB612Reg10, b, 40, 54, grn2, 0);
    txt(g, FontB612Reg10, "VS", 6, 68, white, 0); if(s.hasVs) snprintf(b, sizeof(b), "%+d", s.vs); else snprintf(b, sizeof(b), "-"); txt(g, FontB612Reg10, b, 40, 68, grn2, 0);
    txt(g, FontB612Reg10, "TRK", 6, 82, white, 0); snprintf(b, sizeof(b), "%03d", s.track); txt(g, FontB612Reg10, b, 40, 82, grn2, 0);
    txt(g, FontB612Reg10, "DST", 6, 96, white, 0); snprintf(b, sizeof(b), "%.1f", s.distNm); txt(g, FontB612Reg10, b, 40, 96, grn2, 0);
  }
  int hi = 0, lo = 0;
  for(int i = 0 ; i < in.n ; i++) { if(in.ac[i].altFt >= 18000) hi++; else if(in.ac[i].altFt >= 0) lo++; }
  txt(g, FontB612Bold12, "TRAFFIC", 314, 6, org, 2);
  g.fillRect(262, 22, 54, 30, rgb(20, 40, 150));
  txt(g, FontB612Reg10, "HI", 266, 26, white, 0); snprintf(b, sizeof(b), "%03d", hi); txt(g, FontB612Reg10, b, 312, 26, cyan, 2);
  txt(g, FontB612Reg10, "LO", 266, 38, white, 0); snprintf(b, sizeof(b), "%03d", lo); txt(g, FontB612Reg10, b, 312, 38, cyan, 2);
  txt(g, FontB612Reg10, "TFC", 314, 100, cyan, 2);
  snprintf(b, sizeof(b), "%d NM", in.radius); txt(g, FontB612Reg10, b, 314, 140, cyan, 2);
  emptyNote(g, in, cx, cy + 20, cyan);
}

enum { STYLE_MAP = 0, STYLE_A32X, STYLE_ATCAS, STYLE_BTCAS, STYLE_B7X, STYLE_COUNT };
template<class G>
static void draw(G &g, int style, const In &in)
{
  switch(style)
  {
    case STYLE_A32X:  a32x(g, in); break;
    case STYLE_ATCAS: atcas(g, in); break;
    case STYLE_BTCAS: btcas(g, in); break;
    default:          b7x(g, in); break;
  }
}
}   // namespace rd
