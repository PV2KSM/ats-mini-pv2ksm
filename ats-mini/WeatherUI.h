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
#define WX_PAGES 3
#endif

enum { WX_CLEAR, WX_PARTLY, WX_CLOUDY, WX_FOG, WX_RAIN, WX_SNOW, WX_STORM };

#ifndef WX_DATUM
#define WX_DATUM(d) (d)
#endif

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

static inline const char *wxDesc(int code)
{
  switch(code)
  {
    case 0:  return "Ceu limpo";
    case 1:  return "Quase limpo";
    case 2:  return "Parcialmente nublado";
    case 3:  return "Nublado";
    case 45: return "Neblina";
    case 48: return "Neblina gelada";
    case 51: return "Garoa fraca";
    case 53: return "Garoa";
    case 55: return "Garoa forte";
    case 56: case 57: return "Garoa gelada";
    case 61: return "Chuva fraca";
    case 63: return "Chuva";
    case 65: return "Chuva forte";
    case 66: case 67: return "Chuva gelada";
    case 71: return "Neve fraca";
    case 73: return "Neve";
    case 75: return "Neve forte";
    case 77: return "Graos de neve";
    case 80: return "Pancadas fracas";
    case 81: return "Pancadas de chuva";
    case 82: return "Pancadas fortes";
    case 85: case 86: return "Pancadas de neve";
    case 95: return "Trovoadas";
    case 96: case 99: return "Trovoada e granizo";
    default: return "Indefinido";
  }
}

static inline const char *wxDayName(const char *date)
{
  static const char *const names[] = {"Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sab"};
  static const int tt[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if(strlen(date) < 10) return "---";
  int y = atoi(date), m = atoi(date + 5), d = atoi(date + 8);
  if(m < 1 || m > 12) return "---";
  if(m < 3) y -= 1;
  return names[(y + y / 4 - y / 100 + y / 400 + tt[m - 1] + d) % 7];
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
      if(day) wxSun(g, cx, cy, 20 * s, t); else wxMoon(g, cx, cy, 21 * s, t, bg);
      break;
    case WX_PARTLY:
      if(day) wxSun(g, cx - 13 * s, cy - 11 * s, 14 * s, t); else wxMoon(g, cx - 13 * s, cy - 11 * s, 15 * s, t, bg);
      wxCloud(g, cx + 8 * s + dr, cy + 9 * s, w * 0.95f, day ? wxRgb(250, 252, 255) : wxRgb(210, 216, 230),
              day ? wxRgb(150, 170, 200) : wxRgb(80, 90, 120));
      break;
    case WX_CLOUDY:
      wxCloud(g, cx - 17 * s - dr, cy - 9 * s, w * 0.62f, wxRgb(190, 200, 216), wxRgb(110, 124, 150));
      wxCloud(g, cx + 4 * s + dr, cy + 3 * s, w, wxRgb(236, 241, 248), wxRgb(128, 144, 170));
      break;
    case WX_FOG:
      wxCloud(g, cx + dr, cy - 12 * s, w * 0.9f, wxRgb(214, 220, 228), wxRgb(140, 150, 166));
      for(int i = 0 ; i < 3 ; i++)
      {
        float off = sinf(t * 0.8f + i * 1.7f) * 5 * s;
        float y = cy + (12 + i * 9) * s;
        g.drawWideLine(wxR(cx - 22 * s + off), wxR(y), wxR(cx + 22 * s + off), wxR(y), 1.8f * s + 0.4f, wxRgb(220, 226, 232));
      }
      break;
    case WX_RAIN:
      wxCloud(g, cx + dr, cy - 10 * s, w, wxRgb(160, 172, 192), wxRgb(92, 104, 130));
      wxDrops(g, cx, cy - 8 * s, w, t, false, wxRgb(120, 190, 255));
      break;
    case WX_SNOW:
      wxCloud(g, cx + dr, cy - 10 * s, w, wxRgb(218, 228, 240), wxRgb(130, 146, 172));
      wxDrops(g, cx, cy - 8 * s, w, t, true, wxRgb(255, 255, 255));
      break;
    default: // storm
      wxCloud(g, cx + dr, cy - 12 * s, w, wxRgb(104, 108, 136), wxRgb(58, 60, 88));
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
    wxIcon(g, WX_CLOUDY, false, 160, 66, 1.1f, t, bg);
    wxText(g, place, 160, 8, fS, cW, 4 /*TC*/, cSh);
    wxText(g, (status && status[0]) ? status : "Buscando clima...", 160, 128, fS, cW, 4, cSh);
    return;
  }

  const int kind = wxKind(d.code);
  const bool day = d.isDay != 0;
  WxBg bg = wxBackground(kind, day, H);
  for(int y = 0 ; y < H ; y++) g.fillRect(0, y, W, 1, bg.at(y));

  char buf[64];

  // Header: place, status/age, page dots
  wxText(g, place, 8, 4, fS, cW, 0 /*TL*/, cSh);
  for(int i = 0 ; i < WX_PAGES ; i++)
  {
    int dx = 316 - (WX_PAGES - 1 - i) * 10;
    if(i == page) g.fillCircle(dx, 12, 3, cW); else g.drawCircle(dx, 12, 3, cDim);
  }
  if(status && status[0]) wxText(g, status, 306 - WX_PAGES * 10, 8, fT, cWarn, 2 /*TR*/, cSh);
  else
  {
    if(ageMin <= 0) snprintf(buf, sizeof(buf), "agora"); else snprintf(buf, sizeof(buf), "%d min", ageMin);
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
    snprintf(buf, sizeof(buf), "Sens %d  Max %d  Min %d", d.feels, d.days[0].tmax, d.days[0].tmin);
    wxText(g, buf, 118, 102, fS, cDim, 0, cSh);

    const int cy0 = 124, ch = 40, cw = 70;
    for(int i = 0 ; i < 4 ; i++)
    {
      int x = 8 + i * (cw + 6);
      wxChip(g, x, cy0, cw, ch);
      const char *label, *unit;
      switch(i)
      {
        case 0: label = "UMIDADE"; unit = "%";    snprintf(buf, sizeof(buf), "%d", d.hum); break;
        case 1: label = "VENTO";   unit = "km/h"; snprintf(buf, sizeof(buf), "%d", d.wind); break;
        case 2: label = "CHUVA";   unit = "%";    snprintf(buf, sizeof(buf), "%d", d.nDays ? d.days[0].pop : 0); break;
        default: label = "PRESSAO"; unit = "hPa"; snprintf(buf, sizeof(buf), "%d", d.pressure); break;
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
    wxText(g, "PROXIMOS DIAS", 8, 22, fT, cDim, 0, cSh);
    const int n = d.nDays < 5 ? d.nDays : 5;
    for(int i = 0 ; i < n ; i++)
    {
      const WxDay &dy = d.days[i];
      int x = 6 + i * 62, w = 58, y = 34, h = 130;
      wxChip(g, x, y, w, h);
      int cxm = x + w / 2;
      wxText(g, i == 0 ? "Hoje" : wxDayName(dy.date), cxm, y + 6, fS, cW, 1 /*TC*/, cSh);
      WxBg dbg = wxBackground(wxKind(dy.code), true, H);
      wxIcon(g, wxKind(dy.code), true, (float)cxm, (float)(y + 46), 0.52f, t, dbg);
      snprintf(buf, sizeof(buf), "%d", dy.tmax);
      wxText(g, buf, cxm, y + 70, fL, cW, 1, cSh);
      snprintf(buf, sizeof(buf), "%d", dy.tmin);
      wxText(g, buf, cxm, y + 98, fS, cDim, 1, cSh);
      // Rain chance bar
      g.fillRect(x + 8, y + 122, w - 16, 3, wxRgb(40, 60, 90));
      g.fillRect(x + 8, y + 122, (w - 16) * (dy.pop > 100 ? 100 : dy.pop) / 100, 3, wxRgb(96, 176, 255));
      snprintf(buf, sizeof(buf), "%d%%", dy.pop);
      wxText(g, buf, cxm, y + 114, fT, cDim, 1);
    }
  }
  else
  {
    wxText(g, "PROXIMAS HORAS", 8, 22, fT, cDim, 0, cSh);
    if(d.nDays)
    {
      snprintf(buf, sizeof(buf), "Nasce %s  Poe %s  UV %d", d.days[0].sunrise, d.days[0].sunset, d.days[0].uv);
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
      wxText(g, "barras: chance de chuva", 312, 22, fT, cDim, 2, cSh);
    }
  }
}
