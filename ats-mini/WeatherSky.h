#pragma once
//
// Realistic sky for the Weather screen (320x170): fractal-noise clouds with light and shade, sun with
// corona, moon with maria, twinkling stars, rain and snow in depth layers, mist, fog and lightning.
// Header-only and templated on the graphics type, like WeatherUI.h (which includes this file after its basics).
// Everything is procedural: no pictures are stored or downloaded.
//
#include <stdlib.h>
#if defined(ESP_PLATFORM)
#include <esp_heap_caps.h>
#endif

#define WXS_W 384          // Cloud map width: wraps, wider than the screen so the pattern never repeats on it
#define WXS_H 112          // Cloud map height: the rest of the screen is clear sky / haze

namespace wxs
{
static uint8_t *alphaMap = nullptr, *litMap = nullptr;
static int builtKind = -1, builtDay = -1;
static bool failed = false;

static inline uint32_t hash(int x, int y, int seed)
{
  uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)seed * 2246822519u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}
static inline float lat(int x, int y, int per, int seed)
{
  x %= per; if(x < 0) x += per;
  return (float)(hash(x, y, seed) & 0xFFFF) * (1.0f / 65535.0f);
}
static float vnoise(float x, float y, int per, int seed)
{
  const int xi = (int)floorf(x), yi = (int)floorf(y);
  float fx = x - xi, fy = y - yi;
  fx = fx * fx * (3.0f - 2.0f * fx); fy = fy * fy * (3.0f - 2.0f * fy);
  const float a = lat(xi, yi, per, seed), b = lat(xi + 1, yi, per, seed), c = lat(xi, yi + 1, per, seed), d = lat(xi + 1, yi + 1, per, seed);
  return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}
// 4 octaves; x counts cells of the first octave across the (wrapping) map width
static float fbm(float x, float y, int octaves)
{
  float s = 0, amp = 0.5f, norm = 0;
  int per = 4;
  for(int o = 0 ; o < octaves ; o++)
  {
    s += amp * vnoise(x, y, per, o * 17 + 3);
    norm += amp; amp *= 0.5f; x *= 2.0f; y *= 2.0f; per *= 2;
  }
  return s / norm;
}
static inline float smooth(float a, float b, float x)
{
  float k = (x - a) / (b - a);
  if(k < 0) k = 0; if(k > 1) k = 1;
  return k * k * (3.0f - 2.0f * k);
}

static void *alloc(size_t n)
{
#if defined(ESP_PLATFORM)
  void *p = heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);     // Big buffers live in PSRAM
  if(p) return p;
#endif
  return malloc(n);
}

struct Tint { int lr, lg, lb, sr, sg, sb; };       // lit / shade colour of the clouds

static Tint tintFor(int kind, bool day)
{
  if(day)
  {
    switch(kind)
    {
      case WX_CLEAR: case WX_PARTLY: return {255, 255, 255, 142, 164, 198};
      case WX_CLOUDY:                return {226, 231, 238, 112, 124, 142};
      case WX_FOG:                   return {226, 230, 234, 150, 158, 168};
      case WX_RAIN:                  return {150, 160, 174, 62, 70, 86};
      case WX_SNOW:                  return {244, 247, 252, 160, 172, 190};
      default:                       return {112, 114, 132, 34, 36, 52};
    }
  }
  switch(kind)
  {
    case WX_CLEAR: case WX_PARTLY: return {104, 118, 168, 20, 26, 52};
    case WX_CLOUDY:                return {84, 92, 112, 22, 26, 38};
    case WX_FOG:                   return {96, 104, 116, 38, 44, 54};
    case WX_RAIN:                  return {70, 80, 100, 16, 20, 34};
    case WX_SNOW:                  return {120, 132, 160, 36, 46, 70};
    default:                       return {62, 60, 84, 12, 10, 26};
  }
}

// Builds the cloud map for this kind (once per kind and day/night)
static bool build(int kind, bool day)
{
  if(failed) return false;
  if(alphaMap && builtKind == kind && builtDay == (day ? 1 : 0)) return true;
  const int N = WXS_W * WXS_H;
  if(!alphaMap) alphaMap = (uint8_t *)alloc(N);
  if(!litMap) litMap = (uint8_t *)alloc(N);
  uint8_t *nmap = (uint8_t *)alloc(N);
  if(!alphaMap || !litMap || !nmap) { free(nmap); free(alphaMap); free(litMap); alphaMap = litMap = nullptr; failed = true; return false; }

  static const float th[] = {0.69f, 0.50f, 0.40f, 0.43f, 0.385f, 0.40f, 0.36f};   // clear partly cloudy fog rain snow storm
  const float thr = th[kind];
  const bool overcast = kind >= WX_CLOUDY;
  for(int y = 0 ; y < WXS_H ; y++)
    for(int x = 0 ; x < WXS_W ; x++)
    {
      float n = fbm(x * (4.0f / WXS_W), y * (1.0f / 34.0f), 4);
      if(overcast) n += 0.12f * (1.0f - (float)y / WXS_H);                // Thicker at the top
      if(n < 0) n = 0; if(n > 1) n = 1;
      nmap[y * WXS_W + x] = (uint8_t)(n * 255.0f);
    }
  for(int y = 0 ; y < WXS_H ; y++)
    for(int x = 0 ; x < WXS_W ; x++)
    {
      const int i = y * WXS_W + x;
      const float n = nmap[i] * (1.0f / 255.0f);
      float a = smooth(thr - 0.075f, thr + 0.095f, n);
      a *= 1.0f - smooth(WXS_H * 0.62f, (float)WXS_H, (float)y) * (overcast ? 0.75f : 1.0f);   // Fade towards the horizon
      alphaMap[i] = (uint8_t)(a * 255.0f);
      // Light from the upper left: bright on the lit rim, darker where the cloud is thick
      const int xl = (x - 4 + WXS_W) % WXS_W, yu = y >= 4 ? y - 4 : 0;
      const float edge = (float)nmap[i] - (float)nmap[yu * WXS_W + xl];
      float l = 168.0f + 3.4f * edge - 120.0f * (n - thr);
      if(l < 0) l = 0; if(l > 255) l = 255;
      litMap[i] = (uint8_t)l;
    }
  free(nmap);
  builtKind = kind; builtDay = day ? 1 : 0;
  return true;
}

struct Rgb { int r, g, b; };
static inline Rgb skyAt(const WxBg &bg, int y)
{
  float k = bg.h > 1 ? (float)y / (float)(bg.h - 1) : 0.0f;
  if(k < 0) k = 0; if(k > 1) k = 1;
  return { wxR(bg.t[0] + (bg.b[0] - bg.t[0]) * k), wxR(bg.t[1] + (bg.b[1] - bg.t[1]) * k), wxR(bg.t[2] + (bg.b[2] - bg.t[2]) * k) };
}
static inline int clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }
static inline uint16_t pack(int r, int g, int b) { return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)); }

// Intensity 1..4 from the WMO code
static int intensity(int code, int kind)
{
  if(kind == WX_RAIN)
  {
    if(code >= 51 && code <= 57) return 1;
    if(code == 61 || code == 80) return 2;
    if(code == 63 || code == 81) return 3;
    return 4;
  }
  if(kind == WX_SNOW) return (code == 71 || code == 85) ? 1 : (code == 73 ? 2 : 3);
  return 4;
}

template<class G>
static bool scene(G &g, const WxBg &bg, int kind, int code, bool day, float t)
{
  if(!build(kind, day)) return false;
  const int W = 320, H = 170;
  const Tint tn = tintFor(kind, day);
  const int off = (int)(t * 4.0f) % WXS_W;
  const bool sunny = day && (kind == WX_CLEAR || kind == WX_PARTLY);
  const bool starry = !day && (kind == WX_CLEAR || kind == WX_PARTLY);
  const float sx = 62.0f, sy = 64.0f;                              // Where the sun / moon sit (behind the temperature block)
  const float R = 92.0f, R2 = R * R;
  const float glowK = sunny ? (kind == WX_CLEAR ? 1.0f : 0.9f) : (day ? (kind == WX_CLOUDY || kind == WX_FOG ? 0.26f : 0.10f) : (kind == WX_CLEAR || kind == WX_PARTLY ? 0.55f : 0.0f));
  const int gr = day ? 255 : 170, gg = day ? 228 : 190, gb = day ? 160 : 255;
  const bool storm = kind == WX_STORM;
  float flash = 0.0f;
  if(storm) { const float ph = fmodf(t, 4.2f); if(ph < 0.22f) flash = (1.0f - ph / 0.22f) * 0.55f; }
  // Bright skies are toned down so the white text stays readable
  const int dk = kind == WX_CLOUDY ? 205 : (kind == WX_FOG ? 190 : (kind == WX_SNOW ? 198 : 256));
  const bool wet = kind == WX_RAIN || kind == WX_STORM || kind == WX_SNOW;

  static lgfx::rgb565_t line[320];
  for(int y = 0 ; y < H ; y++)
  {
    const Rgb s = skyAt(bg, y);
    const uint8_t *arow = y < WXS_H ? alphaMap + y * WXS_W : nullptr;
    const uint8_t *lrow = y < WXS_H ? litMap + y * WXS_W : nullptr;
    // Fog / mist rises from the ground
    int mist = 0;
    if(kind == WX_FOG) mist = wxR(150.0f * smooth(30.0f, 170.0f, (float)y) + 22.0f * sinf(t * 0.4f + y * 0.05f));
    else if(wet) mist = wxR(60.0f * smooth(120.0f, 170.0f, (float)y));
    const float dy = (float)y - sy;
    for(int x = 0 ; x < W ; x++)
    {
      int r = s.r, gc = s.g, b = s.b;
      // Sun / moon glow (Lorentz corona with a soft window)
      const float dx = (float)x - sx;
      const float r2 = dx * dx + dy * dy;
      if(glowK > 0 && r2 < R2)
      {
        const float win = 1.0f - r2 / R2;
        float c = (0.58f / (1.0f + r2 / 256.0f) + 0.26f / (1.0f + r2 / 2304.0f)) * win * win * glowK;
        r += (int)((float)(gr - r) * c); gc += (int)((float)(gg - gc) * c); b += (int)((float)(gb - b) * c);
        if(sunny && r2 < 169.0f)                                      // Disc
        {
          const float d = sqrtf(r2);
          float k = (12.2f - d) / 1.7f; if(k < 0) k = 0; if(k > 1) k = 1;
          const int cr = 255, cg = 251 - (int)(d * 1.6f), cb = 232 - (int)(d * 5.0f);
          r += (int)((float)(cr - r) * k); gc += (int)((float)(cg - gc) * k); b += (int)((float)(cb - b) * k);
        }
        else if(!day && (kind == WX_CLEAR || kind == WX_PARTLY) && r2 < 196.0f)   // Moon: disc with maria and a lit limb
        {
          const float d = sqrtf(r2);
          float k = (12.6f - d) / 1.5f; if(k < 0) k = 0; if(k > 1) k = 1;
          const float maria = vnoise(dx * 0.22f + 40.0f, dy * 0.22f + 40.0f, 4096, 9) * 0.55f + vnoise(dx * 0.55f + 9.0f, dy * 0.55f + 9.0f, 4096, 4) * 0.20f;
          const float term = smooth(-12.0f, 10.0f, dx + dy * 0.25f);   // Light comes from the right
          const float lum = (0.62f + 0.38f * term) * (1.0f - maria * 0.42f) * (1.0f - 0.12f * (d / 12.6f) * (d / 12.6f));
          const int cr = (int)(246.0f * lum), cg = (int)(242.0f * lum), cb = (int)(222.0f * lum);
          r += (int)((float)(cr - r) * k); gc += (int)((float)(cg - gc) * k); b += (int)((float)(cb - b) * k);
        }
      }
      // Clouds
      if(arow)
      {
        const int xi = (x + off) % WXS_W;
        const int a = arow[xi];
        if(a)
        {
          const int l = lrow[xi];
          const int cr = tn.sr + (tn.lr - tn.sr) * l / 255, cg = tn.sg + (tn.lg - tn.sg) * l / 255, cb = tn.sb + (tn.lb - tn.sb) * l / 255;
          r += (cr - r) * a / 255; gc += (cg - gc) * a / 255; b += (cb - b) * a / 255;
        }
      }
      if(mist) { r += (214 - r) * mist / 255; gc += (222 - gc) * mist / 255; b += (232 - b) * mist / 255; }
      if(flash > 0) { const int f = (int)(flash * 255.0f); r += (255 - r) * f / 255; gc += (255 - gc) * f / 255; b += (255 - b) * f / 255; }
      if(dk != 256) { r = r * dk >> 8; gc = gc * dk >> 8; b = b * dk >> 8; }
      line[x].raw = pack(clamp255(r), clamp255(gc), clamp255(b));
    }
    g.pushImage(0, y, W, 1, line);
  }

  // Stars, hidden behind the clouds
  if(starry)
    for(int i = 0 ; i < 80 ; i++)
    {
      const uint32_t h = hash(i, 7, 21);
      const int x = (int)(h % 318) + 1, y = (int)((h >> 9) % 98) + 1;
      if((x - sx) * (x - sx) + (y - sy) * (y - sy) < 36 * 36) continue;       // Washed out by the moon
      const int a = alphaMap[y * WXS_W + (x + off) % WXS_W];
      if(a > 70) continue;
      float b = 0.45f + 0.55f * (0.5f + 0.5f * sinf(t * (1.1f + (h & 7) * 0.23f) + i * 2.3f));
      b *= (float)(255 - a) / 255.0f * (0.45f + 0.55f * (float)((h >> 20) & 15) / 15.0f);
      const int v = (int)(255.0f * b);
      const uint16_t c = wxRgb(v, v, 200 + (v >> 3));
      g.drawPixel(x, y, c);
      if(b > 0.8f && (i & 3) == 0) { g.drawPixel(x + 1, y, c); g.drawPixel(x, y + 1, c); }
    }

  // Rain / snow in three depth layers
  if(kind == WX_RAIN || kind == WX_STORM)
  {
    const int in = intensity(code, kind);
    const float slant = storm ? 0.34f : 0.20f;
    static const int cnt[3][5] = {{0, 16, 30, 46, 62}, {0, 8, 16, 26, 36}, {0, 3, 6, 10, 16}};
    static const float len[3] = {5.0f, 10.0f, 18.0f}, spd[3] = {150.0f, 260.0f, 420.0f};
    static const int mixk[3] = {55, 85, 125};
    for(int L = 0 ; L < 3 ; L++)
      for(int i = 0 ; i < cnt[L][in] ; i++)
      {
        const float sp = spd[L] * (0.9f + 0.2f * (float)(hash(i, L, 5) & 7) / 7.0f);
        const float y = fmodf(t * sp + (float)(hash(i, L, 1) % 400), 190.0f) - 16.0f;
        const float x = fmodf((float)(hash(i, L, 2) % 360) - y * slant + 720.0f, 340.0f) - 10.0f;
        const int yy = wxR(y) < 0 ? 0 : (wxR(y) > 169 ? 169 : wxR(y));
        const Rgb c0 = skyAt(bg, yy);
        const uint16_t col = wxMix(pack(c0.r, c0.g, c0.b), wxRgb(196, 220, 255), mixk[L]);
        g.drawWideLine(wxR(x), wxR(y), wxR(x - len[L] * slant * 1.6f), wxR(y + len[L]), L == 2 ? 1.2f : 0.9f, col);
      }
    if(storm)                                                       // Lightning bolt
    {
      const float ph = fmodf(t, 4.2f);
      if(ph < 0.30f)
      {
        const int seed = (int)(t / 4.2f);
        float bx = 40.0f + (float)(hash(seed, 3, 8) % 240), by = 20.0f;
        for(int k = 0 ; k < 9 ; k++)
        {
          const float nx = bx + (float)((int)(hash(seed, k, 9) % 31) - 15) * 0.9f, ny = by + 15.0f;
          g.drawWideLine(wxR(bx), wxR(by), wxR(nx), wxR(ny), 3.2f, wxRgb(120, 130, 210));
          g.drawWideLine(wxR(bx), wxR(by), wxR(nx), wxR(ny), 1.1f, wxRgb(255, 255, 255));
          bx = nx; by = ny;
        }
      }
    }
  }
  else if(kind == WX_SNOW)
  {
    const int in = intensity(code, kind);
    static const int cnt[3][4] = {{0, 20, 34, 48}, {0, 10, 18, 26}, {0, 4, 8, 12}};
    static const float spd[3] = {16.0f, 28.0f, 44.0f};
    for(int L = 0 ; L < 3 ; L++)
      for(int i = 0 ; i < cnt[L][in] ; i++)
      {
        const float y = fmodf(t * spd[L] * (0.8f + 0.4f * (float)(hash(i, L, 5) & 7) / 7.0f) + (float)(hash(i, L, 1) % 300), 184.0f) - 8.0f;
        const float x = fmodf((float)(hash(i, L, 2) % 330), 320.0f) + sinf(t * (0.9f + L * 0.4f) + i * 1.7f) * (4.0f + L * 3.0f);
        const uint16_t c = wxRgb(224 + L * 10, 234 + L * 7, 250);
        if(L == 0) g.drawPixel(wxR(x), wxR(y), c);
        else g.fillSmoothCircle(wxR(x), wxR(y), L, c);
      }
  }
  return true;
}
}   // namespace wxs
