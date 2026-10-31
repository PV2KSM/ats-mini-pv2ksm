#pragma once
//
// Weather screen renderer (320x170).
// Header-only and templated on the graphics type so the exact same drawing code
// can run on the radio (LGFX_Sprite) and in a PC preview.
//
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "I18n.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct WxDay  { int code, tmax, tmin, pop, uv; char date[11]; char sunrise[6]; char sunset[6]; };
struct WxHour { int code, temp, pop, hour; };
struct WxData
{
  bool valid;
  int temp, feels, hum, code, cloud, isDay, pressure, windDir, wind, gust;
  float precip;
  int nDays, nHours;
  WxDay days[6];
  WxHour hours[12];
};

#ifndef WX_PAGES
#define WX_PAGES 4
#endif

enum { WX_CLEAR, WX_PARTLY, WX_CLOUDY, WX_FOG, WX_RAIN, WX_SNOW, WX_STORM };

#ifndef WX_DATUM
#define WX_DATUM(d) (d)
#endif

// Colours chosen in Settings (the firmware sets them before drawing)
static uint16_t wxAcc = 0xFDC6;   // Accent: card borders, bars, page dots
static uint16_t wxTxt = 0xFFFF;   // Main text

static inline uint16_t wxMix(uint16_t a, uint16_t b, int k)   // k 0..255 of b
{
  const int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31, br = (b >> 11) & 31, bg_ = (b >> 5) & 63, bb = b & 31;
  return (uint16_t)((((ar * (255 - k) + br * k) / 255) << 11) | (((ag * (255 - k) + bg_ * k) / 255) << 5) | ((ab * (255 - k) + bb * k) / 255));
}

static inline int wxR(float v) { return (int)lroundf(v); }

static inline uint16_t wxRgb(int r, int g, int b)
{
  if(r < 0) r = 0; if(r > 255) r = 255;
  if(g < 0) g = 0; if(g > 255) g = 255;
  if(b < 0) b = 0; if(b > 255) b = 255;
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// WMO weather interpretation code -> icon kind
static inline int wxKind(int code)
{
  if(code <= 1) return WX_CLEAR;
  if(code == 2) return WX_PARTLY;
  if(code == 3) return WX_CLOUDY;
  if(code == 45 || code == 48) return WX_FOG;
  if((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) return WX_RAIN;
  if((code >= 71 && code <= 77) || code == 85 || code == 86) return WX_SNOW;
  if(code >= 95) return WX_STORM;
  return WX_CLOUDY;
}

static inline const char *wxDesc(int code) { return trWx(code); }

static inline const char *wxDayName(const char *date)
{
  static const int tt[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if(strlen(date) < 10) return "---";
  int y = atoi(date), m = atoi(date + 5), d = atoi(date + 8);
  if(m < 1 || m > 12) return "---";
  if(m < 3) y -= 1;
  return trDay((y + y / 4 - y / 100 + y / 400 + tt[m - 1] + d) % 7);
}

struct WxBg
{
  int t[3], b[3];
  int h;
  uint16_t at(int y) const
  {
    float k = h > 1 ? (float)y / (float)(h - 1) : 0.0f;
    if(k < 0) k = 0; if(k > 1) k = 1;
    return wxRgb(wxR(t[0] + (b[0] - t[0]) * k), wxR(t[1] + (b[1] - t[1]) * k), wxR(t[2] + (b[2] - t[2]) * k));
  }
};

static inline WxBg wxBackground(int kind, bool day, int h)
{
  static const int day_[7][6] = {
    { 24,108,214, 120,196,246 },  // clear
    { 46,120,200, 140,190,230 },  // partly
    { 84, 98,116, 150,162,176 },  // cloudy
    {120,132,140, 190,196,198 },  // fog
    { 46, 62, 86,  96,114,136 },  // rain
    {110,130,156, 206,220,234 },  // snow
    { 34, 34, 56,  86, 78,112 },  // storm
  };
  static const int night_[7][6] = {
    {  6, 10, 36,  26, 40, 92 },
    { 10, 16, 44,  40, 52,100 },
    { 18, 22, 34,  52, 60, 76 },
    { 30, 34, 40,  70, 76, 82 },
    { 14, 20, 34,  40, 52, 72 },
    { 30, 40, 62,  88,104,130 },
    { 12, 10, 24,  44, 38, 66 },
  };
  const int *c = day ? day_[kind] : night_[kind];
  WxBg bg;
  for(int i = 0 ; i < 3 ; i++) { bg.t[i] = c[i]; bg.b[i] = c[3 + i]; }
  bg.h = h;
  return bg;
}

//
// Icons. s = scale (1.0 is about 64 px wide), t = time in seconds for animation.
// Everything is baked into solid colours (no alpha): halos are concentric discs blended with the sky colour.
//
static inline uint16_t wxSkyAt(const WxBg *bg, int y) { return bg ? bg->at(y < 0 ? 0 : (y > bg->h - 1 ? bg->h - 1 : y)) : wxRgb(60, 130, 210); }

// Soft glow around a point: concentric discs from the outside in
template<class G>
static void wxHalo(G &g, float cx, float cy, float r, uint16_t sky, uint16_t col, int strength)
{
  for(int i = 14 ; i >= 1 ; i--)
    g.fillCircle(wxR(cx), wxR(cy), wxR(r * (0.30f + i * 0.050f)), wxMix(sky, col, strength * (15 - i) * (15 - i) / 225));
}

template<class G>
static void wxCloud(G &g, float cx, float cy, float w, uint16_t col, uint16_t shade)
{
  const uint16_t lite = wxMix(col, 0xFFFF, 110), mid = wxMix(col, shade, 70), low = wxMix(col, shade, 130);
  for(int pass = 0 ; pass < 4 ; pass++)
  {
    // 0 drop shadow, 1 body, 2 underside, 3 highlights
    const uint16_t c = pass == 0 ? shade : (pass == 1 ? col : (pass == 2 ? mid : lite));
    const int o = pass == 0 ? wxR(w * 0.05f) : 0;
    if(pass < 2)
    {
      g.fillSmoothCircle(wxR(cx - 0.24f * w) + o, wxR(cy + 0.02f * w) + o, wxR(0.19f * w), c);
      g.fillSmoothCircle(wxR(cx - 0.02f * w) + o, wxR(cy - 0.10f * w) + o, wxR(0.27f * w), c);
      g.fillSmoothCircle(wxR(cx + 0.24f * w) + o, wxR(cy + 0.04f * w) + o, wxR(0.17f * w), c);
      g.fillSmoothRoundRect(wxR(cx - 0.43f * w) + o, wxR(cy + 0.02f * w) + o, wxR(0.86f * w), wxR(0.20f * w), wxR(0.10f * w), c);
    }
    else if(pass == 2)   // darker underside, gradient in two steps
    {
      g.fillSmoothRoundRect(wxR(cx - 0.43f * w), wxR(cy + 0.12f * w), wxR(0.86f * w), wxR(0.10f * w), wxR(0.05f * w), mid);
      g.fillSmoothRoundRect(wxR(cx - 0.40f * w), wxR(cy + 0.17f * w), wxR(0.80f * w), wxR(0.05f * w), wxR(0.025f * w), low);
    }
    else                 // light on the upper left of every puff
    {
      g.fillSmoothCircle(wxR(cx - 0.07f * w), wxR(cy - 0.17f * w), wxR(0.15f * w), lite);
      g.fillSmoothCircle(wxR(cx - 0.28f * w), wxR(cy - 0.03f * w), wxR(0.09f * w), lite);
      g.fillSmoothCircle(wxR(cx - 0.04f * w), wxR(cy - 0.13f * w), wxR(0.19f * w), wxMix(col, lite, 120));
    }
  }
}

template<class G>
static void wxSun(G &g, float cx, float cy, float r, float t, const WxBg *bg = nullptr)
{
  const uint16_t sky = wxSkyAt(bg, wxR(cy));
  wxHalo(g, cx, cy, r * (2.0f + 0.12f * sinf(t * 1.6f)), sky, wxRgb(255, 214, 90), 110);
  const float rot = t * 0.30f;
  for(int i = 0 ; i < 16 ; i++)
  {
    const float a = rot + i * (float)M_PI / 8.0f;
    const bool lg = !(i & 1);
    const float len = (lg ? 2.05f : 1.65f) + 0.10f * sinf(t * 2.4f + i * 0.9f);
    const float c = cosf(a), s = sinf(a);
    g.drawWedgeLine(cx + c * r * 1.30f, cy + s * r * 1.30f, cx + c * r * len, cy + s * r * len, r * (lg ? 0.13f : 0.09f), r * 0.02f + 0.3f, lg ? wxRgb(255, 222, 96) : wxRgb(255, 196, 70));
  }
  g.fillSmoothCircle(wxR(cx), wxR(cy), wxR(r * 1.14f), wxRgb(255, 170, 40));
  g.fillSmoothCircle(wxR(cx), wxR(cy), wxR(r * 1.04f), wxRgb(255, 196, 56));
  g.fillSmoothCircle(wxR(cx), wxR(cy), wxR(r * 0.95f), wxRgb(255, 214, 72));
  g.fillSmoothCircle(wxR(cx - r * 0.12f), wxR(cy - r * 0.14f), wxR(r * 0.66f), wxRgb(255, 230, 110));
  g.fillSmoothCircle(wxR(cx - r * 0.24f), wxR(cy - r * 0.28f), wxR(r * 0.38f), wxRgb(255, 246, 178));
}

template<class G>
static void wxMoon(G &g, float cx, float cy, float r, float t, const WxBg &bg)
{
  wxHalo(g, cx, cy, r * 1.9f, bg.at(wxR(cy)), wxRgb(150, 170, 230), 80);
  g.fillSmoothCircle(wxR(cx), wxR(cy), wxR(r), wxRgb(244, 240, 214));
  g.fillSmoothCircle(wxR(cx - r * 0.08f), wxR(cy - r * 0.08f), wxR(r * 0.88f), wxRgb(250, 248, 228));
  // Carve the crescent with the sky colour of each scanline (no visible disc)
  {
    int ccx = wxR(cx + r * 0.50f), ccy = wxR(cy - r * 0.30f), rc = wxR(r * 0.84f);
    for(int dy = -rc ; dy <= rc ; dy++)
    {
      int hw = wxR(sqrtf((float)(rc * rc - dy * dy)));
      g.fillRect(ccx - hw, ccy + dy, 2 * hw + 1, 1, bg.at(ccy + dy));
    }
  }
  // craters on the lit side
  g.fillSmoothCircle(wxR(cx - r * 0.45f), wxR(cy + r * 0.15f), wxR(r * 0.12f), wxRgb(226, 220, 190));
  g.fillSmoothCircle(wxR(cx - r * 0.20f), wxR(cy + r * 0.50f), wxR(r * 0.08f), wxRgb(226, 220, 190));
  static const float st[][2] = {{-1.5f,-1.0f},{1.45f,-1.25f},{1.7f,0.3f},{-1.7f,0.5f},{0.3f,-1.7f},{-0.8f,1.6f},{1.2f,1.5f}};
  for(int i = 0 ; i < 7 ; i++)
  {
    float b = 0.5f + 0.5f * sinf(t * 2.0f + i * 1.7f);
    int x = wxR(cx + st[i][0] * r), y = wxR(cy + st[i][1] * r);
    const uint16_t c = wxRgb(150 + wxR(b * 100), 150 + wxR(b * 100), 190 + wxR(b * 60));
    g.fillSmoothCircle(x, y, b > 0.6f ? 2 : 1, c);
    if(b > 0.85f) { g.drawFastHLine(x - 3, y, 7, c); g.drawFastVLine(x, y - 3, 7, c); }   // twinkle
  }
}

template<class G>
static void wxDrops(G &g, float cx, float cy, float w, float t, bool snow, uint16_t col)
{
  const int n = snow ? 9 : 10;
  for(int layer = 0 ; layer < 2 ; layer++)
    for(int i = layer ; i < n ; i += 2)
    {
      const float x0 = cx - w * 0.32f + i * (w * 0.64f / (n - 1));
      const float sp = (snow ? 0.50f : 1.45f) * (layer ? 0.8f : 1.15f);
      const float p = fmodf(t * sp + i * 0.37f, 1.0f);
      const float y = cy + w * 0.24f + p * w * 0.58f;
      const float fade = p > 0.85f ? (1.0f - p) / 0.15f : 1.0f;
      if(snow)
      {
        const float x = x0 + sinf(t * 1.8f + i * 1.3f) * w * 0.05f;
        const int s = layer ? 1 : 2;
        const uint16_t c = wxMix(wxRgb(120, 150, 200), col, wxR(255 * fade));
        g.drawFastHLine(wxR(x) - s, wxR(y), 2 * s + 1, c); g.drawFastVLine(wxR(x), wxR(y) - s, 2 * s + 1, c);
        if(!layer) { g.drawPixel(wxR(x) - 1, wxR(y) - 1, c); g.drawPixel(wxR(x) + 1, wxR(y) + 1, c); g.drawPixel(wxR(x) - 1, wxR(y) + 1, c); g.drawPixel(wxR(x) + 1, wxR(y) - 1, c); }
      }
      else
      {
        const float len = w * (layer ? 0.09f : 0.13f) * fade;
        g.drawWedgeLine(x0 + len * 0.45f, y - len, x0, y, 0.2f, layer ? 1.0f : 1.5f, wxMix(wxRgb(60, 110, 190), col, layer ? 150 : 255));
        if(p > 0.88f) g.drawCircle(wxR(x0), wxR(cy + w * 0.84f), wxR((p - 0.88f) * 28.0f), wxMix(wxRgb(60, 100, 160), col, 80));   // ripple
      }
    }
}

template<class G>
static void wxBolt(G &g, float bx, float by, float s, bool flash)
{
  const uint16_t glow = flash ? wxRgb(255, 240, 150) : wxRgb(120, 90, 30), c = flash ? wxRgb(255, 252, 210) : wxRgb(255, 204, 64);
  for(int pass = 0 ; pass < 2 ; pass++)
  {
    const float k = pass == 0 ? 1.35f : 1.0f; const uint16_t cc = pass == 0 ? glow : c;
    g.fillTriangle(wxR(bx + 7 * s * k), wxR(by), wxR(bx - 4 * s * k), wxR(by + 15 * s), wxR(bx + 3 * s * k), wxR(by + 15 * s), cc);
    g.fillTriangle(wxR(bx + 2 * s * k), wxR(by + 12 * s), wxR(bx + 11 * s * k), wxR(by + 12 * s), wxR(bx - 6 * s * k), wxR(by + 31 * s * (pass == 0 ? 1.05f : 1.0f)), cc);
  }
}

template<class G>
static void wxIcon(G &g, int kind, bool day, float cx, float cy, float s, float t, const WxBg &bg)
{
  const float w = 64.0f * s;
  const float dr = sinf(t * 0.5f) * s * 2.5f;
  const float bob = sinf(t * 1.1f) * s * 1.5f;   // slow vertical float
  switch(kind)
  {
    case WX_CLEAR:
      if(day) wxSun(g, cx, cy, 20 * s, t, &bg); else wxMoon(g, cx, cy, 21 * s, t, bg);
      break;
    case WX_PARTLY:
      if(day) wxSun(g, cx - 13 * s, cy - 11 * s, 14 * s, t, &bg); else wxMoon(g, cx - 13 * s, cy - 11 * s, 15 * s, t, bg);
      wxCloud(g, cx + 8 * s + dr, cy + 9 * s + bob, w * 0.95f, day ? wxRgb(250, 252, 255) : wxRgb(210, 216, 230),
              day ? wxRgb(150, 170, 200) : wxRgb(80, 90, 120));
      break;
    case WX_CLOUDY:
      wxCloud(g, cx - 17 * s - dr, cy - 9 * s - bob, w * 0.62f, wxRgb(176, 188, 208), wxRgb(104, 118, 146));
      wxCloud(g, cx + 4 * s + dr, cy + 3 * s + bob, w, wxRgb(236, 241, 248), wxRgb(128, 144, 170));
      break;
    case WX_FOG:
      wxCloud(g, cx + dr, cy - 12 * s, w * 0.9f, wxRgb(214, 220, 228), wxRgb(140, 150, 166));
      for(int i = 0 ; i < 3 ; i++)
      {
        const float off = sinf(t * 0.8f + i * 1.7f) * 6 * s;
        const float y = cy + (12 + i * 9) * s;
        g.drawWedgeLine(cx - 24 * s + off, y, cx + 24 * s + off, y, 2.4f * s + 0.5f, 2.4f * s + 0.5f, wxMix(wxRgb(190, 200, 212), wxRgb(235, 240, 246), 80 + i * 60));
      }
      break;
    case WX_RAIN:
      wxCloud(g, cx + dr, cy - 10 * s + bob, w, wxRgb(150, 164, 188), wxRgb(86, 98, 126));
      wxDrops(g, cx, cy - 8 * s, w, t, false, wxRgb(130, 200, 255));
      break;
    case WX_SNOW:
      wxCloud(g, cx + dr, cy - 10 * s + bob, w, wxRgb(218, 228, 240), wxRgb(130, 146, 172));
      wxDrops(g, cx, cy - 8 * s, w, t, true, wxRgb(255, 255, 255));
      break;
    default: // storm
    {
      const bool flash = fmodf(t, 2.4f) < 0.35f;
      wxCloud(g, cx + dr, cy - 12 * s + bob, w, flash ? wxRgb(170, 172, 200) : wxRgb(96, 100, 128), flash ? wxRgb(100, 104, 140) : wxRgb(54, 56, 84));
      wxBolt(g, cx - 2 * s, cy + 5 * s, s, flash);
      break;
    }
  }
}

template<class G>
static void wxArrow(G &g, int cx, int cy, float deg, float len, uint16_t col)
{
  float a = deg * (float)M_PI / 180.0f;
  float sx = sinf(a), cxv = cosf(a);
  // Points toward 'deg' (0 = up)
  g.fillTriangle(wxR(cx + sx * len), wxR(cy - cxv * len),
                 wxR(cx - sx * len * 0.6f + cxv * len * 0.55f), wxR(cy + cxv * len * 0.6f + sx * len * 0.55f),
                 wxR(cx - sx * len * 0.6f - cxv * len * 0.55f), wxR(cy + cxv * len * 0.6f - sx * len * 0.55f), col);
}

template<class G, class F>
static void wxText(G &g, const char *s, int x, int y, F font, uint16_t col, int datum, uint16_t shadow = 0)
{
  g.setTextDatum(WX_DATUM(datum));
  if(shadow)
  {
    g.setTextColor(shadow);
    g.drawString(s, x + 1, y + 1, font);
  }
  g.setTextColor(col);
  g.drawString(s, x, y, font);
}

// Rounded translucent card with an accent outline (sky and particles stay visible through it)
template<class G>
static void wxChip(G &g, int x, int y, int w, int h)
{
  const int r = 7;
  for(int i = 0 ; i < r ; i++)
  {
    const int dy = r - i, inset = r - (int)(sqrtf((float)(r * r - dy * dy)) + 0.5f);
    g.fillRectAlpha(x + inset, y + i, w - 2 * inset, 1, 78, (uint32_t)0x000000);
    g.fillRectAlpha(x + inset, y + h - 1 - i, w - 2 * inset, 1, 78, (uint32_t)0x000000);
  }
  g.fillRectAlpha(x, y + r, w, h - 2 * r, 78, (uint32_t)0x000000);
  g.drawRoundRect(x, y, w, h, r, wxMix(wxRgb(20, 30, 50), wxAcc, 150));
}

// Particles over the whole sky: stars, drifting clouds, rain, snow, fog, lightning
template<class G>
static void wxAmbient(G &g, int kind, bool day, float t, const WxBg &bg)
{
  if(!day && (kind == WX_CLEAR || kind == WX_PARTLY))
    for(int i = 0 ; i < 26 ; i++)
    {
      const int x = (i * 97 + 31) % 316 + 2, y = (i * 53 + 11) % 120 + 2;
      const float b = 0.5f + 0.5f * sinf(t * 1.7f + i * 2.1f);
      g.fillCircle(x, y, (b > 0.85f && (i & 3) == 0) ? 1 : 0, wxRgb(120 + wxR(b * 120), 120 + wxR(b * 120), 160 + wxR(b * 90)));
    }
  if(kind == WX_PARTLY || kind == WX_CLOUDY || kind == WX_FOG)
    for(int i = 0 ; i < 3 ; i++)
    {
      const float x = fmodf(t * (5.0f + i * 2.5f) + i * 130.0f, 420.0f) - 50.0f;
      const int y = 30 + i * 46;
      wxCloud(g, x, (float)y, 74.0f + i * 14, wxMix(bg.at(y), 0xFFFF, day ? 30 : 18), wxMix(bg.at(y), 0x0000, 30));
    }
  if(kind == WX_RAIN || kind == WX_STORM)
    for(int i = 0 ; i < 34 ; i++)
    {
      const float sp = 150.0f + (i % 5) * 35.0f;
      const float y = fmodf(t * sp + i * 41.0f, 190.0f) - 14.0f;
      const float x = fmodf(i * 47.0f - y * 0.22f + 640.0f, 330.0f) - 5.0f;
      g.drawWideLine(wxR(x), wxR(y), wxR(x - 2.5f), wxR(y + 10.0f), 1.0f, wxMix(bg.at(wxR(y) < 0 ? 0 : (wxR(y) > 169 ? 169 : wxR(y))), wxRgb(170, 205, 255), 90));
    }
  if(kind == WX_SNOW)
    for(int i = 0 ; i < 34 ; i++)
    {
      const float y = fmodf(t * (22.0f + (i % 4) * 9.0f) + i * 29.0f, 184.0f) - 8.0f;
      const float x = fmodf(i * 53.0f, 320.0f) + sinf(t * 1.3f + i) * 6.0f;
      g.fillCircle(wxR(x), wxR(y), (i & 3) ? 1 : 2, wxRgb(235, 242, 255));
    }
  if(kind == WX_FOG)
    for(int i = 0 ; i < 4 ; i++)
    {
      const float off = sinf(t * 0.35f + i * 1.9f) * 26.0f;
      g.fillRectAlpha(wxR(off) - 20, 20 + i * 38, 360, 16, 34, (uint32_t)0xFFFFFF);
    }
  if(kind == WX_STORM && fmodf(t, 2.4f) < 0.14f) g.fillRectAlpha(0, 0, 320, 170, 64, (uint32_t)0xFFFFFF);
}

//
// Full screen. page: 0 now, 1 next days, 2 next hours. status: "" when fine.
//
template<class G, class F>
static void wxDrawScreen(G &g, const WxData &d, const char *place, const char *status, int ageMin,
                         int page, float t, F fT, F fS, F fL, F fD)
{
  const int W = 320, H = 170;
  const uint16_t cW = wxTxt, cDim = wxRgb(206, 220, 238), cSh = wxRgb(8, 20, 44);
  const uint16_t cWarn = wxRgb(255, 214, 90);

  if(!d.valid)
  {
    WxBg bg = wxBackground(WX_CLOUDY, false, H);
    for(int y = 0 ; y < H ; y++) g.fillRect(0, y, W, 1, bg.at(y));
    wxAmbient(g, WX_CLOUDY, false, t, bg);
    wxIcon(g, WX_CLOUDY, false, 160, 66, 1.1f, t, bg);
    wxText(g, place, 160, 8, fS, cW, 1 /*TC*/, cSh);
    wxText(g, (status && status[0]) ? status : tr(T_SEARCHING_WX), 160, 128, fS, cW, 1 /*TC*/, cSh);
    return;
  }

  const int kind = wxKind(d.code);
  const bool day = d.isDay != 0;
  WxBg bg = wxBackground(kind, day, H);
  for(int y = 0 ; y < H ; y++) g.fillRect(0, y, W, 1, bg.at(y));
  wxAmbient(g, kind, day, t, bg);

  char buf[64];

  // Header: place, status/age, page dots
  wxText(g, place, 8, 4, fS, cW, 0 /*TL*/, cSh);
  for(int i = 0 ; i < WX_PAGES ; i++)
  {
    int dx = 316 - (WX_PAGES - 1 - i) * 10;
    if(i == page) { g.fillCircle(dx, 12, 4, wxAcc); } else g.drawCircle(dx, 12, 3, cDim);
  }
  if(status && status[0]) wxText(g, status, 306 - WX_PAGES * 10, 8, fT, cWarn, 2 /*TR*/, cSh);
  else
  {
    if(ageMin <= 0) snprintf(buf, sizeof(buf), "%s", tr(T_NOW)); else snprintf(buf, sizeof(buf), "%d min", ageMin);
    wxText(g, buf, 306 - WX_PAGES * 10, 8, fT, cDim, 2, cSh);
  }

  if(page == 0)
  {
    wxIcon(g, kind, day, 60, 72, 1.15f, t, bg);

    snprintf(buf, sizeof(buf), "%d", d.temp);
    wxText(g, buf, 118, 26, fD, cW, 0, cSh);
    int tw = g.textWidth(buf, fD);
    g.drawCircle(118 + tw + 8, 34, 4, cW);
    g.drawCircle(118 + tw + 8, 34, 3, cW);
    wxText(g, "C", 118 + tw + 16, 28, fL, cW, 0, cSh);

    wxText(g, wxDesc(d.code), 118, 82, fS, cW, 0, cSh);
    snprintf(buf, sizeof(buf), tr(T_FEELS), d.feels, d.days[0].tmax, d.days[0].tmin);
    wxText(g, buf, 118, 102, fS, cDim, 0, cSh);

    const int cy0 = 124, ch = 40, cw = 70;
    for(int i = 0 ; i < 4 ; i++)
    {
      int x = 8 + i * (cw + 6);
      wxChip(g, x, cy0, cw, ch);
      const char *label, *unit;
      switch(i)
      {
        case 0: label = tr(T_HUMIDITY); unit = "%";    snprintf(buf, sizeof(buf), "%d", d.hum); break;
        case 1: label = tr(T_WIND);   unit = "km/h"; snprintf(buf, sizeof(buf), "%d", d.wind); break;
        case 2: label = tr(T_RAIN);   unit = "%";    snprintf(buf, sizeof(buf), "%d", d.nDays ? d.days[0].pop : 0); break;
        default: label = tr(T_PRESSURE); unit = "hPa"; snprintf(buf, sizeof(buf), "%d", d.pressure); break;
      }
      wxText(g, label, x + 6, cy0 + 5, fT, cDim, 0);
      wxText(g, buf, x + 6, cy0 + 19, fS, cW, 0);
      int tw = g.textWidth(buf, fS);
      wxText(g, unit, x + 9 + tw, cy0 + 25, fT, cDim, 0);
      if(i == 1) wxArrow(g, x + cw - 11, cy0 + 11, (float)d.windDir + 180.0f, 5.5f, cW);
    }
  }
  else if(page == 1)
  {
    wxText(g, tr(T_NEXT_DAYS), 8, 22, fT, cDim, 0, cSh);
    const int n = d.nDays < 5 ? d.nDays : 5;
    for(int i = 0 ; i < n ; i++)
    {
      const WxDay &dy = d.days[i];
      int x = 6 + i * 62, w = 58, y = 34, h = 130;
      wxChip(g, x, y, w, h);
      int cxm = x + w / 2;
      wxText(g, i == 0 ? tr(T_TODAY) : wxDayName(dy.date), cxm, y + 6, fS, cW, 1 /*TC*/, cSh);
      WxBg dbg = wxBackground(wxKind(dy.code), true, H);
      wxIcon(g, wxKind(dy.code), true, (float)cxm, (float)(y + 46), 0.52f, t, dbg);
      snprintf(buf, sizeof(buf), "%d", dy.tmax);
      wxText(g, buf, cxm, y + 70, fL, cW, 1, cSh);
      snprintf(buf, sizeof(buf), "%d", dy.tmin);
      wxText(g, buf, cxm, y + 98, fS, cDim, 1, cSh);
      // Rain chance bar
      g.fillRect(x + 8, y + 122, w - 16, 3, wxRgb(40, 60, 90));
      g.fillRect(x + 8, y + 122, (w - 16) * (dy.pop > 100 ? 100 : dy.pop) / 100, 3, wxMix(wxRgb(96, 176, 255), wxAcc, 90));
      snprintf(buf, sizeof(buf), "%d%%", dy.pop);
      wxText(g, buf, cxm, y + 114, fT, cDim, 1);
    }
  }
  else
  {
    wxText(g, tr(T_NEXT_HOURS), 8, 22, fT, cDim, 0, cSh);
    if(d.nDays)
    {
      snprintf(buf, sizeof(buf), tr(T_SUNRISE), d.days[0].sunrise, d.days[0].sunset, d.days[0].uv);
      wxText(g, buf, 8, 34, fS, cW, 0, cSh);
    }
    const int n = d.nHours;
    if(n >= 2)
    {
      const int x0 = 20, x1 = 300, yTop = 76, yBot = 120;
      int mn = d.hours[0].temp, mx = d.hours[0].temp;
      for(int i = 1 ; i < n ; i++) { if(d.hours[i].temp < mn) mn = d.hours[i].temp; if(d.hours[i].temp > mx) mx = d.hours[i].temp; }
      auto px = [&](int i) { return x0 + i * (x1 - x0) / (n - 1); };
      auto py = [&](int v) { return mx == mn ? (yTop + yBot) / 2 : yBot - (v - mn) * (yBot - yTop) / (mx - mn); };
      // Filled area
      for(int i = 0 ; i < n - 1 ; i++)
      {
        int xa = px(i), xb = px(i + 1), ya = py(d.hours[i].temp), yb = py(d.hours[i + 1].temp);
        for(int x = xa ; x < xb ; x++)
        {
          int y = ya + (yb - ya) * (x - xa) / (xb - xa);
          g.fillRectAlpha(x, y, 1, yBot + 6 - y, 58, (uint32_t)0xFFFFFF);
        }
      }
      for(int i = 0 ; i < n - 1 ; i++)
        g.drawWideLine(px(i), py(d.hours[i].temp), px(i + 1), py(d.hours[i + 1].temp), 1.3f, cW);
      for(int i = 0 ; i < n ; i++)
      {
        g.fillCircle(px(i), py(d.hours[i].temp), 3, cW);
        g.fillCircle(px(i), py(d.hours[i].temp), 1, bg.at(py(d.hours[i].temp)));
        snprintf(buf, sizeof(buf), "%d", d.hours[i].temp);
        wxText(g, buf, px(i), py(d.hours[i].temp) - 14, fT, cW, 1, cSh);
      }
      // Hour labels and rain bars
      for(int i = 0 ; i < n ; i++)
      {
        if(i % 3 == 0)
        {
          snprintf(buf, sizeof(buf), "%dh", d.hours[i].hour);
          wxText(g, buf, px(i), yBot + 9, fT, cDim, 1, cSh);
        }
        int bh = d.hours[i].pop * 22 / 100;
        g.fillRect(px(i) - 6, 164 - 22, 12, 22, wxRgb(24, 40, 70));
        if(bh > 0) g.fillRect(px(i) - 6, 164 - bh, 12, bh, wxRgb(96, 176, 255));
      }
      wxText(g, tr(T_BARS_RAIN), 312, 22, fT, cDim, 2, cSh);
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------
// Original (r30) weather drawing, used when Visual = Original
// ---------------------------------------------------------------------------------------------------------------
namespace wxo
{
//
// Icons. s = scale (1.0 is about 64 px wide), t = time in seconds for animation.
//
template<class G>
static void wxCloud(G &g, float cx, float cy, float w, uint16_t col, uint16_t shade)
{
  for(int pass = 0 ; pass < 2 ; pass++)
  {
    uint16_t c = pass ? col : shade;
    int o = pass ? 0 : wxR(w * 0.045f);
    g.fillCircle(wxR(cx - 0.24f * w) + o, wxR(cy + 0.02f * w) + o, wxR(0.19f * w), c);
    g.fillCircle(wxR(cx - 0.02f * w) + o, wxR(cy - 0.10f * w) + o, wxR(0.27f * w), c);
    g.fillCircle(wxR(cx + 0.24f * w) + o, wxR(cy + 0.04f * w) + o, wxR(0.17f * w), c);
    g.fillRoundRect(wxR(cx - 0.43f * w) + o, wxR(cy + 0.02f * w) + o, wxR(0.86f * w), wxR(0.20f * w), wxR(0.10f * w), c);
  }
}

template<class G>
static void wxSun(G &g, float cx, float cy, float r, float t)
{
  float rot = t * 0.35f;
  for(int i = 0 ; i < 12 ; i++)
  {
    float a = rot + i * (float)M_PI / 6.0f;
    float len = 1.55f + ((i & 1) ? 0.0f : 0.20f) + 0.07f * sinf(t * 3.0f + i);
    float c = cosf(a), s = sinf(a);
    g.drawWideLine(wxR(cx + c * r * 1.28f), wxR(cy + s * r * 1.28f), wxR(cx + c * r * len), wxR(cy + s * r * len),
                   r * 0.07f + 0.6f, wxRgb(255, 214, 72));
  }
  g.fillCircle(wxR(cx), wxR(cy), wxR(r * 1.10f), wxRgb(255, 178, 50));
  g.fillCircle(wxR(cx), wxR(cy), wxR(r), wxRgb(255, 208, 64));
  g.fillCircle(wxR(cx - r * 0.22f), wxR(cy - r * 0.22f), wxR(r * 0.55f), wxRgb(255, 232, 120));
}

template<class G>
static void wxMoon(G &g, float cx, float cy, float r, float t, const WxBg &bg)
{
  g.fillCircle(wxR(cx), wxR(cy), wxR(r), wxRgb(244, 240, 214));
  // Carve the crescent with the sky colour of each scanline (no visible disc)
  {
    int ccx = wxR(cx + r * 0.50f), ccy = wxR(cy - r * 0.30f), rc = wxR(r * 0.84f);
    for(int dy = -rc ; dy <= rc ; dy++)
    {
      int hw = wxR(sqrtf((float)(rc * rc - dy * dy)));
      g.fillRect(ccx - hw, ccy + dy, 2 * hw + 1, 1, bg.at(ccy + dy));
    }
  }
  static const float st[][2] = {{-1.5f,-1.0f},{1.45f,-1.25f},{1.7f,0.3f},{-1.7f,0.5f},{0.3f,-1.7f}};
  for(int i = 0 ; i < 5 ; i++)
  {
    float b = 0.5f + 0.5f * sinf(t * 2.0f + i * 1.7f);
    int x = wxR(cx + st[i][0] * r), y = wxR(cy + st[i][1] * r);
    g.fillCircle(x, y, b > 0.6f ? 2 : 1, wxRgb(150 + wxR(b * 100), 150 + wxR(b * 100), 190 + wxR(b * 60)));
  }
}

template<class G>
static void wxDrops(G &g, float cx, float cy, float w, float t, bool snow, uint16_t col)
{
  const int n = snow ? 7 : 6;
  for(int i = 0 ; i < n ; i++)
  {
    float x = cx - w * 0.30f + i * (w * 0.60f / (n - 1));
    float p = fmodf(t * (snow ? 0.55f : 1.5f) + i * 0.37f, 1.0f);
    float y = cy + w * 0.26f + p * w * 0.52f;
    if(snow)
    {
      x += sinf(t * 2.0f + i * 1.3f) * w * 0.04f;
      g.fillCircle(wxR(x), wxR(y), (i & 1) ? 2 : 1, col);
    }
    else
    {
      float len = w * 0.10f * (p > 0.8f ? (1.0f - p) * 5.0f : 1.0f);
      g.drawWideLine(wxR(x), wxR(y), wxR(x - len * 0.35f), wxR(y + len), 1.1f, col);
    }
  }
}

template<class G>
static void wxIcon(G &g, int kind, bool day, float cx, float cy, float s, float t, const WxBg &bg)
{
  const float w = 64.0f * s;
  const float dr = sinf(t * 0.5f) * s * 2.5f;
  switch(kind)
  {
    case WX_CLEAR:
      if(day) wxo::wxSun(g, cx, cy, 20 * s, t); else wxo::wxMoon(g, cx, cy, 21 * s, t, bg);
      break;
    case WX_PARTLY:
      if(day) wxo::wxSun(g, cx - 13 * s, cy - 11 * s, 14 * s, t); else wxo::wxMoon(g, cx - 13 * s, cy - 11 * s, 15 * s, t, bg);
      wxo::wxCloud(g, cx + 8 * s + dr, cy + 9 * s, w * 0.95f, day ? wxRgb(250, 252, 255) : wxRgb(210, 216, 230),
              day ? wxRgb(150, 170, 200) : wxRgb(80, 90, 120));
      break;
    case WX_CLOUDY:
      wxo::wxCloud(g, cx - 17 * s - dr, cy - 9 * s, w * 0.62f, wxRgb(190, 200, 216), wxRgb(110, 124, 150));
      wxo::wxCloud(g, cx + 4 * s + dr, cy + 3 * s, w, wxRgb(236, 241, 248), wxRgb(128, 144, 170));
      break;
    case WX_FOG:
      wxo::wxCloud(g, cx + dr, cy - 12 * s, w * 0.9f, wxRgb(214, 220, 228), wxRgb(140, 150, 166));
      for(int i = 0 ; i < 3 ; i++)
      {
        float off = sinf(t * 0.8f + i * 1.7f) * 5 * s;
        float y = cy + (12 + i * 9) * s;
        g.drawWideLine(wxR(cx - 22 * s + off), wxR(y), wxR(cx + 22 * s + off), wxR(y), 1.8f * s + 0.4f, wxRgb(220, 226, 232));
      }
      break;
    case WX_RAIN:
      wxo::wxCloud(g, cx + dr, cy - 10 * s, w, wxRgb(160, 172, 192), wxRgb(92, 104, 130));
      wxo::wxDrops(g, cx, cy - 8 * s, w, t, false, wxRgb(120, 190, 255));
      break;
    case WX_SNOW:
      wxo::wxCloud(g, cx + dr, cy - 10 * s, w, wxRgb(218, 228, 240), wxRgb(130, 146, 172));
      wxo::wxDrops(g, cx, cy - 8 * s, w, t, true, wxRgb(255, 255, 255));
      break;
    default: // storm
      wxo::wxCloud(g, cx + dr, cy - 12 * s, w, wxRgb(104, 108, 136), wxRgb(58, 60, 88));
      {
        bool flash = fmodf(t, 2.4f) < 0.35f;
        uint16_t c = flash ? wxRgb(255, 246, 150) : wxRgb(255, 204, 64);
        float bx = cx - 2 * s, by = cy + 5 * s;
        g.fillTriangle(wxR(bx + 7 * s), wxR(by), wxR(bx - 4 * s), wxR(by + 15 * s), wxR(bx + 3 * s), wxR(by + 15 * s), c);
        g.fillTriangle(wxR(bx + 2 * s), wxR(by + 12 * s), wxR(bx + 11 * s), wxR(by + 12 * s), wxR(bx - 6 * s), wxR(by + 31 * s), c);
      }
      break;
  }
}

template<class G>
static void wxArrow(G &g, int cx, int cy, float deg, float len, uint16_t col)
{
  float a = deg * (float)M_PI / 180.0f;
  float sx = sinf(a), cxv = cosf(a);
  // Points toward 'deg' (0 = up)
  g.fillTriangle(wxR(cx + sx * len), wxR(cy - cxv * len),
                 wxR(cx - sx * len * 0.6f + cxv * len * 0.55f), wxR(cy + cxv * len * 0.6f + sx * len * 0.55f),
                 wxR(cx - sx * len * 0.6f - cxv * len * 0.55f), wxR(cy + cxv * len * 0.6f - sx * len * 0.55f), col);
}

template<class G, class F>
static void wxText(G &g, const char *s, int x, int y, F font, uint16_t col, int datum, uint16_t shadow = 0)
{
  g.setTextDatum(WX_DATUM(datum));
  if(shadow)
  {
    g.setTextColor(shadow);
    g.drawString(s, x + 1, y + 1, font);
  }
  g.setTextColor(col);
  g.drawString(s, x, y, font);
}

template<class G>
static void wxChip(G &g, int x, int y, int w, int h)
{
  g.fillRectAlpha(x, y, w, h, 74, (uint32_t)0x000000);
  g.fillRectAlpha(x, y, w, 1, 60, (uint32_t)0xFFFFFF);
}

//
// Full screen. page: 0 now, 1 next days, 2 next hours. status: "" when fine.
//
template<class G, class F>
static void wxDrawScreen(G &g, const WxData &d, const char *place, const char *status, int ageMin,
                         int page, float t, F fT, F fS, F fL, F fD)
{
  const int W = 320, H = 170;
  const uint16_t cW = wxRgb(255, 255, 255), cDim = wxRgb(206, 220, 238), cSh = wxRgb(8, 20, 44);
  const uint16_t cWarn = wxRgb(255, 214, 90);

  if(!d.valid)
  {
    WxBg bg = wxBackground(WX_CLOUDY, false, H);
    for(int y = 0 ; y < H ; y++) g.fillRect(0, y, W, 1, bg.at(y));
    wxo::wxIcon(g, WX_CLOUDY, false, 160, 66, 1.1f, t, bg);
    wxo::wxText(g, place, 160, 8, fS, cW, 4 /*TC*/, cSh);
    wxo::wxText(g, (status && status[0]) ? status : tr(T_SEARCHING_WX), 160, 128, fS, cW, 4, cSh);
    return;
  }

  const int kind = wxKind(d.code);
  const bool day = d.isDay != 0;
  WxBg bg = wxBackground(kind, day, H);
  for(int y = 0 ; y < H ; y++) g.fillRect(0, y, W, 1, bg.at(y));

  char buf[64];

  // Header: place, status/age, page dots
  wxo::wxText(g, place, 8, 4, fS, cW, 0 /*TL*/, cSh);
  for(int i = 0 ; i < WX_PAGES ; i++)
  {
    int dx = 316 - (WX_PAGES - 1 - i) * 10;
    if(i == page) g.fillCircle(dx, 12, 3, cW); else g.drawCircle(dx, 12, 3, cDim);
  }
  if(status && status[0]) wxo::wxText(g, status, 306 - WX_PAGES * 10, 8, fT, cWarn, 2 /*TR*/, cSh);
  else
  {
    if(ageMin <= 0) snprintf(buf, sizeof(buf), "%s", tr(T_NOW)); else snprintf(buf, sizeof(buf), "%d min", ageMin);
    wxo::wxText(g, buf, 306 - WX_PAGES * 10, 8, fT, cDim, 2, cSh);
  }

  if(page == 0)
  {
    wxo::wxIcon(g, kind, day, 60, 72, 1.15f, t, bg);

    snprintf(buf, sizeof(buf), "%d", d.temp);
    wxo::wxText(g, buf, 118, 26, fD, cW, 0, cSh);
    int tw = g.textWidth(buf, fD);
    g.drawCircle(118 + tw + 8, 34, 4, cW);
    g.drawCircle(118 + tw + 8, 34, 3, cW);
    wxo::wxText(g, "C", 118 + tw + 16, 28, fL, cW, 0, cSh);

    wxo::wxText(g, wxDesc(d.code), 118, 82, fS, cW, 0, cSh);
    snprintf(buf, sizeof(buf), tr(T_FEELS), d.feels, d.days[0].tmax, d.days[0].tmin);
    wxo::wxText(g, buf, 118, 102, fS, cDim, 0, cSh);

    const int cy0 = 124, ch = 40, cw = 70;
    for(int i = 0 ; i < 4 ; i++)
    {
      int x = 8 + i * (cw + 6);
      wxo::wxChip(g, x, cy0, cw, ch);
      const char *label, *unit;
      switch(i)
      {
        case 0: label = tr(T_HUMIDITY); unit = "%";    snprintf(buf, sizeof(buf), "%d", d.hum); break;
        case 1: label = tr(T_WIND);   unit = "km/h"; snprintf(buf, sizeof(buf), "%d", d.wind); break;
        case 2: label = tr(T_RAIN);   unit = "%";    snprintf(buf, sizeof(buf), "%d", d.nDays ? d.days[0].pop : 0); break;
        default: label = tr(T_PRESSURE); unit = "hPa"; snprintf(buf, sizeof(buf), "%d", d.pressure); break;
      }
      wxo::wxText(g, label, x + 6, cy0 + 5, fT, cDim, 0);
      wxo::wxText(g, buf, x + 6, cy0 + 19, fS, cW, 0);
      int tw = g.textWidth(buf, fS);
      wxo::wxText(g, unit, x + 9 + tw, cy0 + 25, fT, cDim, 0);
      if(i == 1) wxo::wxArrow(g, x + cw - 11, cy0 + 11, (float)d.windDir + 180.0f, 5.5f, cW);
    }
  }
  else if(page == 1)
  {
    wxo::wxText(g, tr(T_NEXT_DAYS), 8, 22, fT, cDim, 0, cSh);
    const int n = d.nDays < 5 ? d.nDays : 5;
    for(int i = 0 ; i < n ; i++)
    {
      const WxDay &dy = d.days[i];
      int x = 6 + i * 62, w = 58, y = 34, h = 130;
      wxo::wxChip(g, x, y, w, h);
      int cxm = x + w / 2;
      wxo::wxText(g, i == 0 ? tr(T_TODAY) : wxDayName(dy.date), cxm, y + 6, fS, cW, 1 /*TC*/, cSh);
      WxBg dbg = wxBackground(wxKind(dy.code), true, H);
      wxo::wxIcon(g, wxKind(dy.code), true, (float)cxm, (float)(y + 46), 0.52f, t, dbg);
      snprintf(buf, sizeof(buf), "%d", dy.tmax);
      wxo::wxText(g, buf, cxm, y + 70, fL, cW, 1, cSh);
      snprintf(buf, sizeof(buf), "%d", dy.tmin);
      wxo::wxText(g, buf, cxm, y + 98, fS, cDim, 1, cSh);
      // Rain chance bar
      g.fillRect(x + 8, y + 122, w - 16, 3, wxRgb(40, 60, 90));
      g.fillRect(x + 8, y + 122, (w - 16) * (dy.pop > 100 ? 100 : dy.pop) / 100, 3, wxRgb(96, 176, 255));
      snprintf(buf, sizeof(buf), "%d%%", dy.pop);
      wxo::wxText(g, buf, cxm, y + 114, fT, cDim, 1);
    }
  }
  else
  {
    wxo::wxText(g, tr(T_NEXT_HOURS), 8, 22, fT, cDim, 0, cSh);
    if(d.nDays)
    {
      snprintf(buf, sizeof(buf), tr(T_SUNRISE), d.days[0].sunrise, d.days[0].sunset, d.days[0].uv);
      wxo::wxText(g, buf, 8, 34, fS, cW, 0, cSh);
    }
    const int n = d.nHours;
    if(n >= 2)
    {
      const int x0 = 20, x1 = 300, yTop = 76, yBot = 120;
      int mn = d.hours[0].temp, mx = d.hours[0].temp;
      for(int i = 1 ; i < n ; i++) { if(d.hours[i].temp < mn) mn = d.hours[i].temp; if(d.hours[i].temp > mx) mx = d.hours[i].temp; }
      auto px = [&](int i) { return x0 + i * (x1 - x0) / (n - 1); };
      auto py = [&](int v) { return mx == mn ? (yTop + yBot) / 2 : yBot - (v - mn) * (yBot - yTop) / (mx - mn); };
      // Filled area
      for(int i = 0 ; i < n - 1 ; i++)
      {
        int xa = px(i), xb = px(i + 1), ya = py(d.hours[i].temp), yb = py(d.hours[i + 1].temp);
        for(int x = xa ; x < xb ; x++)
        {
          int y = ya + (yb - ya) * (x - xa) / (xb - xa);
          g.fillRectAlpha(x, y, 1, yBot + 6 - y, 58, (uint32_t)0xFFFFFF);
        }
      }
      for(int i = 0 ; i < n - 1 ; i++)
        g.drawWideLine(px(i), py(d.hours[i].temp), px(i + 1), py(d.hours[i + 1].temp), 1.3f, cW);
      for(int i = 0 ; i < n ; i++)
      {
        g.fillCircle(px(i), py(d.hours[i].temp), 3, cW);
        g.fillCircle(px(i), py(d.hours[i].temp), 1, bg.at(py(d.hours[i].temp)));
        snprintf(buf, sizeof(buf), "%d", d.hours[i].temp);
        wxo::wxText(g, buf, px(i), py(d.hours[i].temp) - 14, fT, cW, 1, cSh);
      }
      // Hour labels and rain bars
      for(int i = 0 ; i < n ; i++)
      {
        if(i % 3 == 0)
        {
          snprintf(buf, sizeof(buf), "%dh", d.hours[i].hour);
          wxo::wxText(g, buf, px(i), yBot + 9, fT, cDim, 1, cSh);
        }
        int bh = d.hours[i].pop * 22 / 100;
        g.fillRect(px(i) - 6, 164 - 22, 12, 22, wxRgb(24, 40, 70));
        if(bh > 0) g.fillRect(px(i) - 6, 164 - bh, 12, bh, wxRgb(96, 176, 255));
      }
      wxo::wxText(g, tr(T_BARS_RAIN), 312, 22, fT, cDim, 2, cSh);
    }
  }
}

}
