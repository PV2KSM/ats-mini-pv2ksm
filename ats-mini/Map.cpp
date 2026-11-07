//
// Radar basemap (Web Mercator raster tiles).
//
// The radar circle is MAP_SIZE pixels wide and represents 2 x radius nautical miles. A zoom
// level is chosen so that one tile pixel covers at most one screen pixel (so we only ever
// downscale, with a 2x2 average), then every output pixel is sampled from the tiles that
// cover the radar box (at most 3x3). Tiles are fetched one at a time over HTTPS (CA bundle,
// HTTP/1.1) and decoded with LovyanGFX's PNG decoder into a temporary 256x256 sprite.
// The final image is kept in PSRAM; it is only rebuilt when the position, radius, tile URL or
// key change.
//
#include "Map.h"
#include "Location.h"
#include "Flights.h"
#include "Utils.h"
#include "Menu.h"
#include <time.h>
#include "Diag.h"
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include <esp_heap_caps.h>
#include <math.h>
#include <string.h>
#include "I18n.h"

#define MAP_TASK_STACK    20480
#define MAP_TILE_MAX      (96 * 1024)     // Largest PNG accepted
#define MAP_RETRY_MS      60000UL
#define MAP_ZOOM_MIN      2
#define MAP_ZOOM_MAX      12

extern const uint8_t caBundleStart[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t caBundleEnd[] asm("_binary_x509_crt_bundle_end");

static SemaphoreHandle_t mtx = nullptr;
static uint16_t *shown = nullptr;            // Finished image (byte-swapped RGB565, as in a 16-bit sprite)
static bool shownValid = false;
static char status[28] = "";
static volatile bool wantBuild = true;       // Config changed or never built
static volatile uint32_t configGen = 1;      // Bumped by mapInvalidate()
static uint32_t builtGen = 0;                // Generation the image in "shown" was built for
static uint32_t lastTryMs = 0;
static uint32_t lastTryGen = 0;
static bool lastTryFailed = false;

static bool shownLight = false;              // Style of the image in "shown"
static bool builtLight = false;
static bool tplHasStyle = false;             // The tile URL can switch between dark and light

// Light or dark for the given style setting (automatic = daytime 06:00-18:00, local time)
static bool wantLight(uint8_t style)
{
  if(style == MAP_STYLE_LIGHT) return true;
  if(style == MAP_STYLE_DARK) return false;
  if(!clockAvailable()) return false;
  time_t t = time(NULL) + getCurrentUTCOffset() * 15 * 60;
  struct tm lt;
  gmtime_r(&t, &lt);
  return lt.tm_hour >= 6 && lt.tm_hour < 18;
}

static void lock()   { if(mtx) xSemaphoreTake(mtx, portMAX_DELAY); }
static void unlock() { if(mtx) xSemaphoreGive(mtx); }

static void setStatus(const char *s)
{
  lock();
  snprintf(status, sizeof(status), "%s", s ? s : "");
  unlock();
}

static uint32_t mapOkMs = 0;
void mapInfo(char *st, size_t size, uint32_t *okMs)
{
  lock(); strlcpy(st, status, size); *okMs = mapOkMs; unlock();
}

const char *mapStatus()
{
  static char copy[28];
  lock();
  memcpy(copy, status, sizeof(copy));
  unlock();
  return copy;
}

void mapInvalidate()
{
  configGen++;
  wantBuild = true;
}

void mapEnsure()
{
  if(builtGen != configGen || !shownValid) wantBuild = true;
  lastTryFailed = false;
}

bool mapDraw(LGFX_Sprite &spr, int x, int y)
{
  bool ok = false;
  lock();
  if(shown && shownValid)
  {
    spr.pushImage(x, y, MAP_W, MAP_H, (const lgfx::swap565_t *)shown);
    ok = true;
  }
  unlock();
  return ok;
}

// ---- Tile download -------------------------------------------------------

static uint8_t *tileBuf = nullptr;

namespace {
class TileSink : public Stream
{
public:
  size_t len = 0;
  bool overflow = false;
  size_t write(uint8_t c) override { return write(&c, 1); }
  size_t write(const uint8_t *d, size_t n) override
  {
    if(len + n > MAP_TILE_MAX) { overflow = true; return n; }
    memcpy(tileBuf + len, d, n);
    len += n;
    return n;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
};
}

// Expand {z},{x},{y},{key} in the template. A stored key is appended as key=... when the template has no {key}.
static bool buildUrl(char *out, size_t size, const char *tpl, const char *key, int z, int x, int y, bool light)
{
  const char *styleName = light ? "light_all" : "dark_all";
  size_t n = 0;
  bool usedKey = false;
  for(const char *p = tpl ; *p ; )
  {
    char rep[72] = "";
    if(!strncmp(p, "{z}", 3))        { snprintf(rep, sizeof(rep), "%d", z); p += 3; }
    else if(!strncmp(p, "{x}", 3))   { snprintf(rep, sizeof(rep), "%d", x); p += 3; }
    else if(!strncmp(p, "{y}", 3))   { snprintf(rep, sizeof(rep), "%d", y); p += 3; }
    else if(!strncmp(p, "{style}", 7)) { snprintf(rep, sizeof(rep), "%s", styleName); p += 7; }
    else if(!strncmp(p, "dark_all", 8) || !strncmp(p, "light_all", 9))   // A pasted URL with a fixed CARTO style
    { snprintf(rep, sizeof(rep), "%s", styleName); p += (*p == 'd') ? 8 : 9; }
    else if(!strncmp(p, "{key}", 5)) { snprintf(rep, sizeof(rep), "%s", key); p += 5; usedKey = true; }
    else { rep[0] = *p++; rep[1] = 0; }
    size_t l = strlen(rep);
    if(n + l + 1 >= size) return false;
    memcpy(out + n, rep, l);
    n += l;
  }
  out[n] = 0;
  if(!usedKey && key[0] && !strstr(tpl, "openstreetmap.org"))   // Never send the CARTO key to OpenStreetMap
  {
    int w = snprintf(out + n, size - n, "%skey=%s", strchr(out, '?') ? "&" : "?", key);
    if(w < 0 || (size_t)w >= size - n) return false;
  }
  return true;
}

bool mapTileUrl(char *out, size_t size, int z, int x, int y, bool light)
{
  char tpl[160], key[64];
  bool on;
  locationGetMapCfg(tpl, sizeof(tpl), &on);
  locationGetCartoKey(key, sizeof(key));
  return buildUrl(out, size, tpl, key, z, x, y, light);
}

static int lastHttp = 0;

static bool fetchTile(HTTPClient &http, NetworkClientSecure &client, const char *tpl, const char *key, int z, int x, int y, bool light, size_t *len)
{
  char url[320];
  if(!buildUrl(url, sizeof(url), tpl, key, z, x, y, light)) { setStatus(tr(T_MAP_URL)); return false; }
  if(!http.begin(client, url)) { setStatus("Mapa: http.begin"); return false; }
  http.addHeader("Accept", "image/png");
  int code = http.GET();
  lastHttp = code;
  if(code != HTTP_CODE_OK)
  {
    char msg[28];
    if(code < 0) snprintf(msg, sizeof(msg), tr(T_MAP_NET), code);
    else snprintf(msg, sizeof(msg), tr(T_MAP_HTTP), code);
    setStatus(msg);
    http.end();
    return false;
  }
  TileSink sink;
  http.writeToStream(&sink);
  http.end();
  if(sink.overflow || sink.len < 64) { setStatus(tr(T_MAP_TILE_BAD)); return false; }
  *len = sink.len;
  return true;
}

// ---- Resampling ------------------------------------------------------------

static inline uint16_t swap16(uint16_t v) { return (uint16_t)((v >> 8) | (v << 8)); }

static inline void unpack(uint16_t n, int *r, int *g, int *b)
{
  *r = (n >> 11) & 31; *g = (n >> 5) & 63; *b = n & 31;
}


// Light unsharp mask on the finished map (restores the edge contrast lost by resampling).
static void sharpenWork(uint16_t *w)
{
  static const int AMT = 5;                                    // 0.5 in tenths
  // Three source rows (r, g, b) kept unsharpened; in PSRAM so internal RAM stays free for the web pages
  typedef int8_t Row[MAP_W][3];
  Row *src = (Row *)heap_caps_malloc(sizeof(Row) * 3, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if(!src) return;
  auto load = [&](int row, int slot) {
    if(row < 0) row = 0; if(row >= MAP_H) row = MAP_H - 1;
    for(int x = 0 ; x < MAP_W ; x++) { int r, g, b; unpack(swap16(w[row * MAP_W + x]), &r, &g, &b); src[slot][x][0] = r; src[slot][x][1] = g; src[slot][x][2] = b; }
  };
  load(0, 0); load(0, 1); load(1, 2);                            // rows -1 (clamped), 0, 1
  for(int j = 0 ; j < MAP_H ; j++)
  {
    if((j & 15) == 0) vTaskDelay(1);
    for(int x = 0 ; x < MAP_W ; x++)
    {
      const int xl = x > 0 ? x - 1 : 0, xr = x < MAP_W - 1 ? x + 1 : x;
      int o[3];
      for(int c = 0 ; c < 3 ; c++)
      {
        const int centre = src[1][x][c];
        const int blur = (src[0][x][c] + src[2][x][c] + src[1][xl][c] + src[1][xr][c] + 4 * centre) / 8;
        int v = centre + (AMT * (centre - blur)) / 10;
        const int mx = c == 1 ? 63 : 31;
        o[c] = v < 0 ? 0 : (v > mx ? mx : v);
      }
      w[j * MAP_W + x] = swap16((uint16_t)((o[0] << 11) | (o[1] << 5) | o[2]));
    }
    // Next row window
    memcpy(src[0], src[1], sizeof(Row)); memcpy(src[1], src[2], sizeof(Row));
    load(j + 2, 2);
  }
  heap_caps_free(src);
}

static void buildMapLocked();
static void buildMap()
{
  buildMapLocked();
}

static void buildMapLocked()
{
  const LocationInfo loc = locationGet();
  char tpl[160], key[64];
  bool on;
  uint8_t style = MAP_STYLE_LIGHT;
  locationGetMapCfg(tpl, sizeof(tpl), &on, &style);
  const bool light = wantLight(style);
  tplHasStyle = strstr(tpl, "{style}") || strstr(tpl, "dark_all") || strstr(tpl, "light_all");
  locationGetCartoKey(key, sizeof(key));
  const uint32_t gen = configGen;
  diagStage(DG_MAP_START);

  if(!on) { lock(); shownValid = false; status[0] = 0; unlock(); builtGen = gen; return; }
  if(WiFi.status() != WL_CONNECTED) { setStatus(tr(T_MAP_NO_WIFI)); return; }
  if(!clockGetDate(nullptr, nullptr, nullptr, nullptr)) { setStatus(tr(T_MAP_NO_TIME)); return; }

  if(!tileBuf) tileBuf = (uint8_t *)heap_caps_malloc(MAP_TILE_MAX, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if(!tileBuf) tileBuf = (uint8_t *)malloc(32 * 1024);   // Small fallback: larger tiles are rejected
  uint16_t *work = (uint16_t *)heap_caps_calloc(MAP_W * MAP_H, 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if(!tileBuf || !work) { if(work) heap_caps_free(work); setStatus(tr(T_MAP_NO_MEM)); return; }

  // Scale: MAP_R px (centre to top/bottom edge) = the configured range
  const double lat = loc.lat, lonv = loc.lon;
  const double targetMpp = loc.radiusNm * 1852.0 / MAP_R;              // Metres per screen pixel
  const double cosLat = cos(lat * M_PI / 180.0);
  int z = (int)ceil(log2(156543.03392 * cosLat / targetMpp));
  if(z < MAP_ZOOM_MIN) z = MAP_ZOOM_MIN;
  if(z > MAP_ZOOM_MAX) z = MAP_ZOOM_MAX;

  // Per-column / per-row source position and tile index (computed once, in double).
  // Lighter map: if more than 4 tiles would be needed, use a coarser zoom (blurrier, but far less to download)
  static float colSrc[MAP_W], rowSrc[MAP_H];
  static int16_t colTile[MAP_W], rowTile[MAP_H];
  int tx0, tx1, ty0, ty1;
  double scaleS = 1.0;                                  // Tile pixels per screen pixel for the zoom finally used
  for(;;)
  {
    const double tileMpp = 156543.03392 * cosLat / (double)(1 << z);
    const double s = targetMpp / tileMpp;
    scaleS = s;
    const double worldPx = 256.0 * (double)(1 << z);
    const double latR = lat * M_PI / 180.0;
    const double X0 = (lonv + 180.0) / 360.0 * worldPx;
    const double Y0 = (1.0 - log(tan(latR) + 1.0 / cos(latR)) / M_PI) / 2.0 * worldPx;
    for(int i = 0 ; i < MAP_W ; i++)
    {
      const double wx = X0 + ((i + 0.5) - MAP_W / 2.0) * s;
      colTile[i] = (int16_t)floor(wx / 256.0);
      colSrc[i] = (float)(wx - colTile[i] * 256.0);
    }
    for(int j = 0 ; j < MAP_H ; j++)
    {
      const double wy = Y0 + ((j + 0.5) - MAP_H / 2.0) * s;
      rowTile[j] = (int16_t)floor(wy / 256.0);
      rowSrc[j] = (float)(wy - rowTile[j] * 256.0);
    }
    tx0 = colTile[0]; tx1 = colTile[MAP_W - 1]; ty0 = rowTile[0]; ty1 = rowTile[MAP_H - 1];
    if((tx1 - tx0 + 1) * (ty1 - ty0 + 1) > 4 && z > MAP_ZOOM_MIN) { z--; continue; }
    break;
  }
  const int maxT = (1 << z) - 1;
  if(tx1 - tx0 > 3) tx1 = tx0 + 3;
  if(ty1 - ty0 > 2) ty1 = ty0 + 2;

  LGFX_Sprite tile;
  tile.setColorDepth(16);
  tile.setPsram(true);
  if(!tile.createSprite(256, 256)) { heap_caps_free(work); setStatus(tr(T_MAP_NO_MEM)); return; }
  const uint16_t *raw = (const uint16_t *)tile.getBuffer();

  NetworkClientSecure client;
  client.setCACertBundle(caBundleStart, caBundleEnd - caBundleStart);
  client.setHandshakeTimeout(10);
  HTTPClient http;                       // One connection is kept alive for all tiles (same host)
  http.setConnectTimeout(10000);
  http.setTimeout(15000);
  http.setUserAgent("ATS-Mini-PV2KSM/v0t (+https://github.com/PV2KSM/ats-mini-pv2ksm)");
  http.setReuse(true);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  // Only one TLS conversation at a time (flights and weather wait while the tiles download)
  const bool haveNet = netLock(30000);
  int okTiles = 0, wantTiles = 0;
  const int allTiles = (tx1 - tx0 + 1) * (ty1 - ty0 + 1);
  char msg[28];
  for(int ty = ty0 ; ty <= ty1 ; ty++)
  {
    for(int tx = tx0 ; tx <= tx1 ; tx++)
    {
      if(ty < 0 || ty > maxT) continue;
      const int txw = ((tx % (maxT + 1)) + (maxT + 1)) % (maxT + 1);   // Wrap around the antimeridian
      wantTiles++;
      snprintf(msg, sizeof(msg), tr(T_MAP_TILE_N), wantTiles, allTiles);
      setStatus(msg);
      size_t len = 0;
      diagStage(DG_MAP_FETCH, wantTiles);
      bool got = fetchTile(http, client, tpl, key, z, txw, ty, light, &len);
      if(!got && lastHttp < 0) { client.stop(); got = fetchTile(http, client, tpl, key, z, txw, ty, light, &len); }   // One retry on network errors
      if(!got)
      {
        if(lastHttp == 401 || lastHttp == 403) break;                   // Key problem: no point asking for more tiles
        continue;
      }
      diagStage(DG_MAP_DECODE, wantTiles);
      tile.fillSprite(tile.color565(12, 16, 22));
      if(!tile.drawPng(tileBuf, len, 0, 0)) { setStatus(tr(T_MAP_PNG)); continue; }
      okTiles++;
      vTaskDelay(2);
      diagStage(DG_MAP_RESAMPLE, wantTiles);

      // Every output pixel whose source position falls inside this tile
      for(int j = 0 ; j < MAP_H ; j++)
      {
        if((j & 15) == 0) vTaskDelay(1);                 // Keep the idle task (watchdog) and Wi-Fi running
        if(rowTile[j] != ty) continue;
        const float sy = rowSrc[j];
        for(int i = 0 ; i < MAP_W ; i++)
        {
          if(colTile[i] != tx) continue;
          const float sx = colSrc[i];
          float fr = 0, fg = 0, fb = 0;
          int rr, gg, bb;
          if(scaleS < 1.5)
          {
            // Bilinear: exact weights of the four nearest tile pixels (sharper than a plain 2x2 average)
            float x = sx - 0.5f, y = sy - 0.5f;
            if(x < 0) x = 0; if(y < 0) y = 0;
            int ix = (int)x, iy2b = (int)y; float fx = x - ix, fy = y - iy2b;
            const int ix2 = ix + 1 > 255 ? 255 : ix + 1, iyb2 = iy2b + 1 > 255 ? 255 : iy2b + 1;
            const float w4[4] = {(1 - fx) * (1 - fy), fx * (1 - fy), (1 - fx) * fy, fx * fy};
            const uint16_t p[4] = {raw[iy2b * 256 + ix], raw[iy2b * 256 + ix2], raw[iyb2 * 256 + ix], raw[iyb2 * 256 + ix2]};
            for(int k = 0 ; k < 4 ; k++) { unpack(swap16(p[k]), &rr, &gg, &bb); fr += w4[k] * rr; fg += w4[k] * gg; fb += w4[k] * bb; }
          }
          else
          {
            // Larger reduction: exact area average of the source pixels the screen pixel covers
            const float hs = (float)scaleS * 0.5f;
            float x0 = sx - hs, x1 = sx + hs, y0 = sy - hs, y1 = sy + hs;
            if(x0 < 0) x0 = 0; if(y0 < 0) y0 = 0; if(x1 > 256) x1 = 256; if(y1 > 256) y1 = 256;
            float wt = 0;
            for(int yy = (int)y0 ; yy < (int)ceilf(y1) && yy < 256 ; yy++)
            {
              const float wy = fminf(y1, yy + 1.0f) - fmaxf(y0, (float)yy);
              for(int xx = (int)x0 ; xx < (int)ceilf(x1) && xx < 256 ; xx++)
              {
                const float wx = fminf(x1, xx + 1.0f) - fmaxf(x0, (float)xx), w = wx * wy;
                unpack(swap16(raw[yy * 256 + xx]), &rr, &gg, &bb);
                fr += w * rr; fg += w * gg; fb += w * bb; wt += w;
              }
            }
            if(wt > 0) { fr /= wt; fg /= wt; fb /= wt; }
          }
          // Light map dimmed to ~94%, dark map to ~81% (was 69%: too little contrast)
          const float dim = light ? 0.9375f : 0.8125f;
          int r = (int)(fr * dim + 0.5f), g = (int)(fg * dim + 0.5f), b = (int)(fb * dim + 0.5f);
          work[j * MAP_W + i] = swap16((uint16_t)((r << 11) | (g << 5) | b));
        }
        if((j & 31) == 31) vTaskDelay(1);
      }
    }
  }
  tile.deleteSprite();
  http.end();
  client.stop();
  if(haveNet) netUnlock();

  if(okTiles == 0)
  {
    heap_caps_free(work);
    lastTryFailed = true;
    lastTryMs = millis();
    lastTryGen = gen;
    return;                                         // Keep the error text set by fetchTile/drawPng
  }

  sharpenWork(work);
  diagStage(DG_MAP_PUBLISH);
  lock();
  if(!shown) shown = (uint16_t *)heap_caps_malloc(MAP_W * MAP_H * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if(shown)
  {
    memcpy(shown, work, MAP_W * MAP_H * 2);
    shownValid = true;
    shownLight = light;
    mapOkMs = millis();
  }
  snprintf(status, sizeof(status), "%s", okTiles < wantTiles ? tr(T_MAP_PARTIAL) : "");
  unlock();
  heap_caps_free(work);
  builtGen = gen;
  builtLight = light;
  lastTryFailed = okTiles < wantTiles;
  lastTryMs = millis();
  lastTryGen = gen;
}

static void mapTask(void *)
{
  for(;;)
  {
    if(flightsIsActive() && !wantBuild && shownValid && tplHasStyle)
    {
      uint8_t st = MAP_STYLE_LIGHT;
      locationGetMapCfg(nullptr, 0, nullptr, &st);
      if(st == MAP_STYLE_AUTO && wantLight(st) != builtLight) wantBuild = true;   // Day/night changed
    }
    if(flightsIsActive() && wantBuild)
    {
      const bool retryWait = lastTryFailed && lastTryGen == configGen && (uint32_t)(millis() - lastTryMs) < MAP_RETRY_MS;
      if(!retryWait)
      {
        wantBuild = false;
        buildMap();
        if(builtGen != configGen) wantBuild = true;   // Config changed while building, or the build failed
        else if(lastTryFailed) wantBuild = true;      // Retry after MAP_RETRY_MS
      }
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void mapBegin()
{
  if(!mtx) mtx = xSemaphoreCreateMutex();
  if(!mtx) return;
  xTaskCreatePinnedToCore(mapTask, "map", MAP_TASK_STACK, nullptr, 1, nullptr, 1);
}
