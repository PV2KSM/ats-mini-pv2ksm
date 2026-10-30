//
// Rain radar page of the Weather screen.
//   Base map: dark CARTO raster tiles (same template and key as the Flights map).
//   Rain:     RainViewer radar tiles (latest past frame, "Universal Blue", PNG with transparency).
// The zoom is an integer level (6 or 7; RainViewer allows at most 7), so one tile pixel is one
// screen pixel and every tile is simply decoded straight into a 320x170 window with
// LovyanGFX's PNG decoder (which clips and blends the transparent rain over the base map).
//
#include "Radar.h"
#include "Location.h"
#include "Map.h"
#include "Weather.h"
#include "NetFetch.h"
#include "Diag.h"
#include "Utils.h"
#include "Menu.h"
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_heap_caps.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define RADAR_W           320
#define RADAR_H           170
#define RADAR_TASK_STACK  20480
#define RADAR_TILE_MAX    (96 * 1024)
#define RADAR_REFRESH_MS  (10UL * 60UL * 1000UL)
#define RADAR_RETRY_MS    60000UL

extern const uint8_t caBundleStart[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t caBundleEnd[] asm("_binary_x509_crt_bundle_end");

static SemaphoreHandle_t mtx = nullptr;
static uint8_t *tileBuf = nullptr;
static uint8_t *shownImg = nullptr;           // What the screen shows (raw 16-bit sprite buffer layout)
static uint8_t *baseImg = nullptr;            // Dimmed base map, without rain
static bool shownValid = false;
static char status[32] = "Radar: aguardando";
static uint32_t frameTime = 0;                // UTC epoch of the radar frame shown
static int shownZoom = 6;
static int zoomSel = 6;
static volatile bool wantBuild = false;
static volatile uint32_t gen = 1;             // Position / map settings generation
static uint32_t shownGen = 0;
static uint32_t baseGen = 0;                  // Generation (and zoom) the base image belongs to
static int baseZoom = 0;
static bool baseValid = false;
static uint32_t lastOkMs = 0, lastTryMs = 0;
static bool lastFailed = false;
static volatile bool wakeFlag = false;

static void lock()   { if(mtx) xSemaphoreTake(mtx, portMAX_DELAY); }
static void unlock() { if(mtx) xSemaphoreGive(mtx); }

static void setStatus(const char *s)
{
  lock();
  snprintf(status, sizeof(status), "%s", s ? s : "");
  unlock();
}

void radarInfo(char *st, size_t size, uint32_t *okMs)
{
  lock(); strlcpy(st, status, size); *okMs = lastOkMs; unlock();
}

void radarInvalidate() { gen++; wantBuild = true; }
void radarWake() { wakeFlag = true; }
void radarToggleZoom() { zoomSel = (zoomSel == 6) ? 7 : 6; wantBuild = true; wakeFlag = true; }

// ---- Download --------------------------------------------------------------

namespace {
class TileSink : public Stream
{
public:
  size_t len = 0;
  bool overflow = false;
  size_t write(uint8_t c) override { return write(&c, 1); }
  size_t write(const uint8_t *d, size_t n) override
  {
    if(len + n > RADAR_TILE_MAX) { overflow = true; return n; }
    memcpy(tileBuf + len, d, n);
    len += n;
    return n;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
};
}

static int lastHttp = 0;

static bool fetchBin(HTTPClient &http, NetworkClientSecure &client, const char *url, size_t *len)
{
  if(!http.begin(client, url)) { lastHttp = -3; return false; }
  http.addHeader("Accept", "image/png");
  const int code = http.GET();
  lastHttp = code;
  if(code != HTTP_CODE_OK) { http.end(); return false; }
  TileSink sink;
  http.writeToStream(&sink);
  http.end();
  if(sink.overflow || sink.len < 64) { lastHttp = -4; return false; }
  *len = sink.len;
  return true;
}

static void worldPos(double lat, double lon, int z, double *x, double *y)
{
  const double worldPx = 256.0 * (double)(1 << z);
  const double latR = lat * M_PI / 180.0;
  *x = (lon + 180.0) / 360.0 * worldPx;
  *y = (1.0 - log(tan(latR) + 1.0 / cos(latR)) / M_PI) / 2.0 * worldPx;
}

static void buildRadar()
{
  const LocationInfo loc = locationGet();
  const uint32_t g = gen;
  const int z = zoomSel;

  if(WiFi.status() != WL_CONNECTED) { setStatus("Radar: sem Wi-Fi"); lastFailed = true; lastTryMs = millis(); return; }
  if(!clockGetDate(nullptr, nullptr, nullptr, nullptr)) { setStatus("Radar: sem hora"); lastFailed = true; lastTryMs = millis(); return; }

  const size_t imgBytes = RADAR_W * RADAR_H * 2;
  if(!tileBuf) tileBuf = (uint8_t *)heap_caps_malloc(RADAR_TILE_MAX, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if(!baseImg) baseImg = (uint8_t *)heap_caps_malloc(imgBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if(!shownImg) shownImg = (uint8_t *)heap_caps_malloc(imgBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  LGFX_Sprite work;
  work.setColorDepth(16);
  work.setPsram(true);
  if(!tileBuf || !baseImg || !shownImg || !work.createSprite(RADAR_W, RADAR_H)) { setStatus("Radar: sem memoria"); lastFailed = true; lastTryMs = millis(); return; }
  uint8_t *wbuf = (uint8_t *)work.getBuffer();

  double px, py;
  worldPos(loc.lat, loc.lon, z, &px, &py);
  const long X0 = (long)floor(px - RADAR_W / 2.0), Y0 = (long)floor(py - RADAR_H / 2.0);
  const int tx0 = (int)floor((double)X0 / 256.0), tx1 = (int)floor((double)(X0 + RADAR_W - 1) / 256.0);
  const int ty0 = (int)floor((double)Y0 / 256.0), ty1 = (int)floor((double)(Y0 + RADAR_H - 1) / 256.0);
  const int maxT = (1 << z) - 1;

  const bool haveNet = netLock(40000);
  if(!haveNet) { setStatus("Radar: rede ocupada"); lastFailed = true; lastTryMs = millis(); return; }
  bool anyBase = false, anyRain = false, failedAny = false;
  char url[200], msg[32];

  // ---- Base map (only when the position, zoom or map settings changed) ----
  if(!baseValid || baseGen != g || baseZoom != z)
  {
    work.fillSprite(work.color565(10, 14, 20));
    NetworkClientSecure client;
    client.setCACertBundle(caBundleStart, caBundleEnd - caBundleStart);
    client.setHandshakeTimeout(10);
    HTTPClient http;
    http.setConnectTimeout(10000);
    http.setTimeout(15000);
    http.setUserAgent(getUserAgent());
    http.setReuse(true);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int n = 0;
    const int total = (tx1 - tx0 + 1) * (ty1 - ty0 + 1);
    for(int ty = ty0 ; ty <= ty1 ; ty++)
    {
      for(int tx = tx0 ; tx <= tx1 ; tx++)
      {
        if(ty < 0 || ty > maxT) continue;
        const int txw = ((tx % (maxT + 1)) + (maxT + 1)) % (maxT + 1);
        n++;
        snprintf(msg, sizeof(msg), "Radar: mapa %d/%d", n, total);
        setStatus(msg);
        if(!mapTileUrl(url, sizeof(url), z, txw, ty, false)) { failedAny = true; continue; }
        size_t len = 0;
        bool got = fetchBin(http, client, url, &len);
        if(!got && lastHttp < 0) { client.stop(); got = fetchBin(http, client, url, &len); }
        if(!got) { failedAny = true; if(lastHttp == 401 || lastHttp == 403) break; continue; }
        if(work.drawPng(tileBuf, len, (int32_t)(tx * 256L - X0), (int32_t)(ty * 256L - Y0))) anyBase = true; else failedAny = true;
        vTaskDelay(2);
      }
    }
    http.end();
    client.stop();
    if(anyBase)
    {
      work.fillRectAlpha(0, 0, RADAR_W, RADAR_H, 90, (uint32_t)0x000000);   // Dim the base so the rain stands out
      memcpy(baseImg, wbuf, imgBytes);
      baseValid = true; baseGen = g; baseZoom = z;
    }
    else baseValid = false;
  }
  else
  {
    memcpy(wbuf, baseImg, imgBytes);
    anyBase = true;
  }
  if(!baseValid) work.fillSprite(work.color565(10, 14, 20));

  // ---- RainViewer: index, then the tiles of the latest frame ----
  uint32_t ftime = 0;
  {
    setStatus("Radar: chuva");
    char err[40];
    const size_t bodyCap = 6144;
    char *body = (char *)heap_caps_malloc(bodyCap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    int code = -1;
    if(body) code = netFetchLocked("https://api.rainviewer.com/public/weather-maps.json", body, bodyCap, err, sizeof(err), "application/json");
    char host[64] = "", path[64] = "";
    if(code == 200)
    {
      JsonDocument doc;
      if(!deserializeJson(doc, body))
      {
        strlcpy(host, doc["host"] | "", sizeof(host));
        JsonArray past = doc["radar"]["past"];
        if(past && past.size())
        {
          JsonObject last = past[past.size() - 1];
          strlcpy(path, last["path"] | "", sizeof(path));
          ftime = last["time"] | 0;
        }
      }
    }
    if(body) heap_caps_free(body);
    if(host[0] && path[0])
    {
      NetworkClientSecure client;
      client.setCACertBundle(caBundleStart, caBundleEnd - caBundleStart);
      client.setHandshakeTimeout(10);
      HTTPClient http;
      http.setConnectTimeout(10000);
      http.setTimeout(15000);
      http.setUserAgent(getUserAgent());
      http.setReuse(true);
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      int n = 0;
      const int total = (tx1 - tx0 + 1) * (ty1 - ty0 + 1);
      for(int ty = ty0 ; ty <= ty1 ; ty++)
      {
        for(int tx = tx0 ; tx <= tx1 ; tx++)
        {
          if(ty < 0 || ty > maxT) continue;
          const int txw = ((tx % (maxT + 1)) + (maxT + 1)) % (maxT + 1);
          n++;
          snprintf(msg, sizeof(msg), "Radar: chuva %d/%d", n, total);
          setStatus(msg);
          snprintf(url, sizeof(url), "%s%s/256/%d/%d/%d/2/1_1.png", host, path, z, txw, ty);
          size_t len = 0;
          bool got = fetchBin(http, client, url, &len);
          if(!got && lastHttp < 0) { client.stop(); got = fetchBin(http, client, url, &len); }
          if(!got) { failedAny = true; continue; }
          if(work.drawPng(tileBuf, len, (int32_t)(tx * 256L - X0), (int32_t)(ty * 256L - Y0))) anyRain = true; else failedAny = true;
          vTaskDelay(2);
        }
      }
      http.end();
      client.stop();
    }
    else failedAny = true;
  }
  netUnlock();

  if(!anyBase && !anyRain) { setStatus("Radar: falha na rede"); lastFailed = true; lastTryMs = millis(); return; }

  lock();
  memcpy(shownImg, wbuf, imgBytes);
  shownValid = true;
  shownZoom = z;
  shownGen = g;
  frameTime = ftime;
  snprintf(status, sizeof(status), "%s", anyRain ? (failedAny ? "parcial" : "") : "sem chuva (rede)");
  unlock();
  lastFailed = !anyRain;
  lastOkMs = millis();
  lastTryMs = millis();
}

static void radarTask(void *)
{
  for(;;)
  {
    if(weatherIsActive() && weatherPage() == 4)
    {
      bool due = wakeFlag;
      if(!due && wantBuild) due = true;
      if(!due && !shownValid) due = true;
      if(!due && lastOkMs && (uint32_t)(millis() - lastOkMs) >= RADAR_REFRESH_MS) due = true;
      if(due && lastFailed && lastTryMs && (uint32_t)(millis() - lastTryMs) < RADAR_RETRY_MS && !wakeFlag) due = false;
      if(due)
      {
        wakeFlag = false;
        wantBuild = false;
        // Reopened shortly after a good build with nothing changed: keep what is there
        const bool fresh = shownValid && shownGen == gen && shownZoom == zoomSel && lastOkMs && (uint32_t)(millis() - lastOkMs) < RADAR_REFRESH_MS && !lastFailed;
        if(!fresh) buildRadar();
      }
    }
    else wakeFlag = false;
    vTaskDelay(pdMS_TO_TICKS(400));
  }
}

void radarBegin()
{
  if(!mtx) mtx = xSemaphoreCreateMutex();
  if(!mtx) return;
  xTaskCreatePinnedToCore(radarTask, "radar", RADAR_TASK_STACK, nullptr, 1, nullptr, 1);
}

// ---- Drawing ---------------------------------------------------------------

static inline uint16_t C565(int r, int g, int b) { return lgfx::color565(r, g, b); }

void radarDraw(LGFX_Sprite &spr)
{
  const LocationInfo loc = locationGet();
  bool have;
  char st[32];
  int zoom;
  uint32_t ft;
  lock();
  have = shownValid;
  zoom = shownZoom;
  ft = frameTime;
  strlcpy(st, status, sizeof(st));
  if(have) spr.pushImage(0, 0, RADAR_W, RADAR_H, (const lgfx::swap565_t *)shownImg);
  unlock();
  if(!have) spr.fillSprite(C565(10, 14, 20));

  const uint16_t cTxt = 0xFFFF, cDim = C565(170, 190, 210), cAcc = C565(0, 200, 255), cGold = C565(255, 214, 90);
  auto chip = [&](int x, int y, int w, int h) { spr.fillRectAlpha(x, y, w, h, 165, (uint32_t)0x000000); };

  spr.setTextDatum(TL_DATUM);

  // Header chip: title, frame time (local) and zoom
  char buf[64];
  if(ft)
  {
    time_t t = (time_t)ft + getCurrentUTCOffset() * 15 * 60;
    struct tm tmv;
    gmtime_r(&t, &tmv);
    snprintf(buf, sizeof(buf), "CHUVA %02d:%02d  z%d", tmv.tm_hour, tmv.tm_min, zoom);
  }
  else snprintf(buf, sizeof(buf), "RADAR DE CHUVA");
  int w = strlen(buf) * 6 + 8;
  chip(3, 3, w, 12);
  spr.setTextColor(cAcc);
  spr.drawString(buf, 7, 5, FONT_DEFAULT);

  if(st[0])
  {
    w = strlen(st) * 6 + 8;
    chip(3, 17, w, 12);
    spr.setTextColor(cGold);
    spr.drawString(st, 7, 19, FONT_DEFAULT);
  }

  {
    const char *at = "(c) OSM/CARTO RainViewer";
    const int aw = strlen(at) * 6 + 8;
    chip(RADAR_W - 3 - aw, 17, aw, 12);
    spr.setTextColor(cDim);
    spr.drawString(at, RADAR_W - aw + 1, 19, FONT_DEFAULT);
  }

  // Page dots (the radar is the last page)
  for(int i = 0 ; i < 5 ; i++)
  {
    const int dx = 316 - (4 - i) * 10;
    if(i == 4) spr.fillCircle(dx, 10, 3, cTxt); else spr.drawCircle(dx, 10, 3, cDim);
  }

  // Own position
  const int cx = RADAR_W / 2, cy = RADAR_H / 2;
  spr.fillCircle(cx, cy, 3, C565(255, 230, 0));
  spr.drawCircle(cx, cy, 5, C565(255, 230, 0));
  if(loc.label[0])
  {
    spr.setTextColor(C565(255, 230, 0));
    spr.drawString(loc.label, cx + 8, cy + 4, FONT_DEFAULT);
  }

  // Scale bar: metres per pixel at this latitude and zoom
  if(have)
  {
    const double mpp = 156543.03392 * cos(loc.lat * M_PI / 180.0) / (double)(1 << zoom);
    static const int nice[] = {10, 20, 50, 100, 200, 500};
    int km = nice[0];
    for(int v : nice) if(v * 1000.0 / mpp <= 80.0) km = v;
    const int len = (int)(km * 1000.0 / mpp);
    const int sy = RADAR_H - 8;
    chip(3, sy - 11, len + 34, 14);
    spr.drawFastHLine(6, sy, len, cDim);
    spr.drawFastVLine(6, sy - 3, 7, cDim);
    spr.drawFastVLine(6 + len, sy - 3, 7, cDim);
    snprintf(buf, sizeof(buf), "%dkm", km);
    spr.setTextColor(cDim);
    spr.drawString(buf, 10 + len, sy - 4, FONT_DEFAULT);
  }
  else
  {
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(cTxt);
    spr.drawString("Carregando radar...", cx, cy + 30, FONT_SMALL);
    spr.setTextDatum(TL_DATUM);
  }

  // Hint (bottom right)
  chip(RADAR_W - 3 - 90, RADAR_H - 17, 90, 14);
  spr.setTextColor(cDim);
  spr.drawString("clique: zoom", RADAR_W - 3 - 86, RADAR_H - 14, FONT_DEFAULT);
}
