//
// Nearby aircraft screen (ADS-B data from api.adsb.lol)
//
// Notes on the HTTP/TLS implementation:
//  * The request is HTTP/1.1. adsb.lol answers "426 Upgrade Required" to
//    HTTP/1.0 requests, which is what HTTPClient::useHTTP10(true) produces.
//  * HTTP/1.1 responses may use chunked transfer encoding, so the body is read
//    with HTTPClient::writeToStream(), which removes the chunk framing. The
//    body is captured into a bounded buffer and parsed from memory.
//  * The TLS certificate is verified with the same CA bundle used by the OTA
//    updater. That requires a valid clock (the radio sets it over NTP).
//  * HTTPClient::begin() clears request headers, so headers are added after it.
//
#include "Flights.h"
#include "Utils.h"
#include "Menu.h"
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Location.h"
#include "Diag.h"
#include "Map.h"
#include <esp_heap_caps.h>
#include <math.h>
#include <string.h>

#if FLIGHTS_DEBUG
#define FLOG(...) Serial.printf(__VA_ARGS__)
#else
#define FLOG(...) do {} while(0)
#endif

#define FLIGHTS_BODY_MAX_PSRAM (192 * 1024)
#define FLIGHTS_BODY_MAX_HEAP  (48 * 1024)
#define FLIGHTS_TASK_STACK     16384
#define FLIGHTS_VIEW_ASPECT    (320.0f / 170.0f)          // Screen width / height
#define FLIGHTS_VIEW_DIAG      2.2f                       // hypot(aspect, 1) rounded up

// CA bundle embedded by the build (shared with the OTA updater)
extern const uint8_t caBundleStart[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t caBundleEnd[] asm("_binary_x509_crt_bundle_end");

struct Aircraft
{
  char callsign[9];
  char id[7];     // ICAO hex, used to keep the selection across refreshes
  char reg[9];    // Registration, e.g. PR-GTA (may be empty)
  char type[5];   // ICAO aircraft type, e.g. A320 (may be empty)
  int vs;         // Vertical rate in ft/min
  bool hasVs;
  int altFt;      // -1 = on the ground
  int speedKt;
  int track;
  float distNm;
  float bearing;  // Radians, clockwise from north
};

// Shared state, protected by mtx
static Aircraft list[FLIGHTS_MAX];
static int listCount = 0;
static int listTotal = 0;   // Aircraft inside the radius (may exceed FLIGHTS_MAX)
static char status[40] = "Aguardando";
static uint32_t lastOkMs = 0;
static uint32_t dataVersion = 0;
static SemaphoreHandle_t mtx = nullptr;

// Configuration (written before the task reads it; changes are rare)

static volatile bool wakeNow = false;

// Response body buffer (allocated once, lives in PSRAM when available)
static char *body = nullptr;
static size_t bodyCap = 0;

// Collects the HTTP body into the bounded buffer
class BodySink : public Stream
{
public:
  size_t len = 0;
  bool overflow = false;
  size_t write(uint8_t c) override { return write(&c, 1); }
  size_t write(const uint8_t *data, size_t n) override
  {
    if(len + n > bodyCap) { overflow = true; return n; }
    memcpy(body + len, data, n);
    len += n;
    return n;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
};

static void lock()   { if(mtx) xSemaphoreTake(mtx, portMAX_DELAY); }
static void unlock() { if(mtx) xSemaphoreGive(mtx); }

static void setStatus(const char *s)
{
  lock();
  snprintf(status, sizeof(status), "%s", s ? s : "Erro desconhecido");
  dataVersion++;
  unlock();
}

static bool allocBody()
{
  if(body) return true;
  body = (char *)heap_caps_malloc(FLIGHTS_BODY_MAX_PSRAM, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if(body) { bodyCap = FLIGHTS_BODY_MAX_PSRAM; return true; }
  body = (char *)malloc(FLIGHTS_BODY_MAX_HEAP);
  if(body) { bodyCap = FLIGHTS_BODY_MAX_HEAP; return true; }
  return false;
}

static void fetchFlightsLocked();
static void fetchFlights()
{
  if(!netLock(200)) { wakeNow = true; return; }   // Busy (map download): retry shortly
  diagStage(DG_FL_FETCH);
  fetchFlightsLocked();
  netUnlock();
}

static void fetchFlightsLocked()
{
  const LocationInfo loc = locationGet();
  const float lat = loc.lat, lon = loc.lon;
  const int radius = loc.radiusNm;

  if(lat == 0.0f && lon == 0.0f) { setStatus("Defina a posicao"); return; }
  if(WiFi.status() != WL_CONNECTED) { setStatus("Sem Wi-Fi"); return; }
  // TLS certificate validation needs the date; same rule as the OTA updater
  if(!clockGetDate(nullptr, nullptr, nullptr, nullptr)) { setStatus("Sem hora (NTP)"); return; }
  if(!allocBody()) { setStatus("Sem memoria"); return; }

  char url[128];
  // The view is a screen-shaped rectangle: radius = nm from the centre to the top/bottom edge, so the
  // query circle must cover the whole rectangle (half-diagonal).
  int queryNm = (int)ceilf(radius * FLIGHTS_VIEW_DIAG);
  if(queryNm > 250) queryNm = 250;
  snprintf(url, sizeof(url), "https://api.adsb.lol/v2/point/%.4f/%.4f/%d", lat, lon, queryNm);
  FLOG("[FLIGHT] GET %s\n", url);

  NetworkClientSecure client;
  client.setCACertBundle(caBundleStart, caBundleEnd - caBundleStart);
  client.setHandshakeTimeout(10);

  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(15000);
  http.setUserAgent(getUserAgent());
  http.setReuse(false);                                   // Connection: close
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  // HTTP/1.1 is the default; do NOT call useHTTP10(true) here.
  if(!http.begin(client, url)) { setStatus("Falha http.begin"); return; }
  http.addHeader("Accept", "application/json");           // After begin(): begin() clears headers

  int code = http.GET();
  FLOG("[FLIGHT] HTTP status: %d\n", code);
  if(code != HTTP_CODE_OK)
  {
    char msg[40], tlsMsg[80] = "";
    if(code > 0) snprintf(msg, sizeof(msg), "HTTP erro %d", code);
    else if(client.lastError(tlsMsg, sizeof(tlsMsg)) != 0) snprintf(msg, sizeof(msg), "TLS erro (%d)", code);
    else snprintf(msg, sizeof(msg), "Rede erro %d", code);
    FLOG("[FLIGHT] %s %s\n", http.errorToString(code).c_str(), tlsMsg);
    http.end();
    setStatus(msg);
    return;
  }

  BodySink sink;
  int wrote = http.writeToStream(&sink);
  http.end();
  FLOG("[FLIGHT] body: %u bytes (writeToStream=%d, overflow=%d)\n", (unsigned)sink.len, wrote, sink.overflow);
  if(sink.overflow) { setStatus("Resposta grande"); return; }
  if(wrote < 0 || sink.len == 0) { setStatus("Leitura falhou"); return; }

  JsonDocument filter;
  filter["ac"][0]["flight"]   = true;
  filter["ac"][0]["hex"]      = true;
  filter["ac"][0]["lat"]      = true;
  filter["ac"][0]["lon"]      = true;
  filter["ac"][0]["alt_baro"] = true;
  filter["ac"][0]["gs"]       = true;
  filter["ac"][0]["track"]    = true;
  filter["ac"][0]["r"]        = true;
  filter["ac"][0]["t"]        = true;
  filter["ac"][0]["baro_rate"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body, sink.len, DeserializationOption::Filter(filter));
  if(err)
  {
    FLOG("[FLIGHT] JSON: %s\n", err.c_str());
    setStatus("Erro JSON");
    return;
  }

  // Keep the FLIGHTS_MAX nearest aircraft (insertion into a sorted array)
  Aircraft tmp[FLIGHTS_MAX];
  int n = 0, total = 0;
  const float cosLat = cosf(lat * (float)M_PI / 180.0f);
  for(JsonObject a : doc["ac"].as<JsonArray>())
  {
    if(!a["lat"].is<float>() || !a["lon"].is<float>()) continue;
    const float dy = (a["lat"].as<float>() - lat) * 60.0f;
    const float dx = (a["lon"].as<float>() - lon) * 60.0f * cosLat;
    const float dist = sqrtf(dx * dx + dy * dy);
    if(fabsf(dy) > (float)radius || fabsf(dx) > (float)radius * FLIGHTS_VIEW_ASPECT) continue;   // Outside the screen
    total++;

    int pos = n;
    while(pos > 0 && tmp[pos - 1].distNm > dist) pos--;
    if(pos >= FLIGHTS_MAX) continue;
    const int last = (n < FLIGHTS_MAX) ? n : FLIGHTS_MAX - 1;
    for(int i = last ; i > pos ; i--) tmp[i] = tmp[i - 1];
    if(n < FLIGHTS_MAX) n++;

    Aircraft &t = tmp[pos];
    const char *fl = a["flight"] | "";
    int len = 0;
    while(fl[len] && fl[len] != ' ' && len < 8) { t.callsign[len] = fl[len]; len++; }
    t.callsign[len] = 0;
    if(!len)
    {
      const char *hex = a["hex"] | "?";
      for(len = 0 ; hex[len] && len < 6 ; len++) t.callsign[len] = toupper((unsigned char)hex[len]);
      t.callsign[len] = 0;
    }
    const char *hx = a["hex"] | "";
    int k = 0;
    for( ; hx[k] && k < 6 ; k++) t.id[k] = toupper((unsigned char)hx[k]);
    t.id[k] = 0;
    const char *rg = a["r"] | "";
    for(k = 0 ; rg[k] && k < 8 ; k++) t.reg[k] = toupper((unsigned char)rg[k]);
    t.reg[k] = 0;
    const char *ty = a["t"] | "";
    for(k = 0 ; ty[k] && k < 4 ; k++) t.type[k] = toupper((unsigned char)ty[k]);
    t.type[k] = 0;
    t.hasVs    = a["baro_rate"].is<float>();
    t.vs       = t.hasVs ? (int)a["baro_rate"].as<float>() : 0;
    t.altFt    = a["alt_baro"].is<int>() ? a["alt_baro"].as<int>() : -1;
    t.speedKt  = (int)(a["gs"] | 0.0f);
    t.track    = (int)(a["track"] | 0.0f);
    t.distNm   = dist;
    t.bearing  = atan2f(dx, dy);
  }

  lock();
  memcpy(list, tmp, sizeof(Aircraft) * n);
  listCount = n;
  listTotal = total;
  lastOkMs = millis();
  snprintf(status, sizeof(status), "%s", n ? "OK" : "Sem aeronaves");
  dataVersion++;
  unlock();
  FLOG("[FLIGHT] Aeronaves: %d\n", n);
}

static void flightsTask(void *)
{
  uint32_t last = 0;
  for(;;)
  {
    if(flightsIsActive())
    {
      bool due = wakeNow || (uint32_t)(millis() - last) >= FLIGHTS_REFRESH_MS;
      if(due) { wakeNow = false; fetchFlights(); last = millis(); }
    }
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void flightsBegin()
{
  if(!mtx) mtx = xSemaphoreCreateMutex();
  if(!mtx) return;
  xTaskCreatePinnedToCore(flightsTask, "flights", FLIGHTS_TASK_STACK, nullptr, 1, nullptr, 1);
}

// The observer position changed (set from the web page): refresh on the next opportunity
void flightsLocationChanged()
{
  wakeNow = true;
}

// Selection state: touched only from the main thread
#define FLIGHTS_ROWS 7
static char selId[7] = "";
static int selIdx = 0, topIdx = 0;

static bool showInfo = false;   // Detail card open (main thread only)
static void resetSelection() { selId[0] = 0; selIdx = 0; topIdx = 0; showInfo = false; }

static void snapshot(Aircraft *snap, int *n, int *total, char *st, uint32_t *okMs)
{
  lock();
  *n = listCount;
  *total = listTotal;
  memcpy(snap, list, sizeof(Aircraft) * listCount);
  if(st) memcpy(st, status, sizeof(status));
  if(okMs) *okMs = lastOkMs;
  unlock();
}

void flightsInfo(char *st, size_t size, uint32_t *okMs)
{
  lock();
  strlcpy(st, status, size);
  *okMs = lastOkMs;
  unlock();
}

// Re-find the selected aircraft after a refresh and keep it inside the window
static void resolveSelection(const Aircraft *snap, int n)
{
  if(n <= 0) { resetSelection(); return; }
  int found = -1;
  if(selId[0])
    for(int i = 0 ; i < n ; i++)
      if(!strcmp(snap[i].id, selId)) { found = i; break; }
  if(found >= 0) selIdx = found;
  else if(selIdx >= n) selIdx = n - 1;
  strlcpy(selId, snap[selIdx].id, sizeof(selId));
  if(selIdx < topIdx) topIdx = selIdx;
  if(selIdx >= topIdx + FLIGHTS_ROWS) topIdx = selIdx - FLIGHTS_ROWS + 1;
  int maxTop = n > FLIGHTS_ROWS ? n - FLIGHTS_ROWS : 0;
  if(topIdx > maxTop) topIdx = maxTop;
  if(topIdx < 0) topIdx = 0;
}

void flightsToggleInfo()
{
  lock();
  const bool any = listCount > 0;
  unlock();
  showInfo = any && !showInfo;
}

void flightsScroll(int16_t delta)
{
  Aircraft snap[FLIGHTS_MAX];
  int n, total;
  snapshot(snap, &n, &total, nullptr, nullptr);
  if(n <= 0) return;
  resolveSelection(snap, n);
  selIdx = ((selIdx + delta) % n + n) % n;      // Wraps around
  strlcpy(selId, snap[selIdx].id, sizeof(selId));
  resolveSelection(snap, n);
}

void flightsOpen()
{
  resetSelection();
  mapEnsure();
  lock();
  snprintf(status, sizeof(status), "Buscando...");
  dataVersion++;
  unlock();
  wakeNow = true;
}

bool flightsIsActive() { return currentCmd == CMD_FLIGHTS; }

// Called from the main loop: redraw on new data and for the radar sweep animation
bool flightsTick()
{
  static uint32_t shownVersion = 0xFFFFFFFF, shownMs = 0;
  if(!flightsIsActive()) { shownVersion = 0xFFFFFFFF; return false; }
  uint32_t now = millis();
  lock();
  uint32_t v = dataVersion;
  unlock();
  if(v != shownVersion || (uint32_t)(now - shownMs) >= 120)
  {
    shownVersion = v;
    shownMs = now;
    return true;
  }
  return false;
}

#define C565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

// Aircraft silhouette (fuselage, swept wings, tail), rotated to the heading.
// Local coordinates: +y = nose. Heading h is clockwise from north, in radians.
static void drawPlane(LGFX_Sprite &spr, int cx, int cy, float h, uint16_t col, float k)
{
  const float sh = sinf(h), ch = cosf(h);
  auto P = [&](float lx, float ly, int *ox, int *oy)
  {
    lx *= k; ly *= k;
    *ox = cx + (int)lroundf(lx * ch + ly * sh);
    *oy = cy + (int)lroundf(lx * sh - ly * ch);
  };
  auto tri = [&](float ax, float ay, float bx, float by, float cx2, float cy2)
  {
    int x1, y1, x2, y2, x3, y3;
    P(ax, ay, &x1, &y1); P(bx, by, &x2, &y2); P(cx2, cy2, &x3, &y3);
    spr.fillTriangle(x1, y1, x2, y2, x3, y3, col);
  };
  tri(-1, 5, 1, 5, 0, 8);                 // Nose
  tri(-1, 5, 1, 5, 1, -5);                // Fuselage
  tri(-1, 5, 1, -5, -1, -5);
  tri(-1, 2.5f, -1, -1, -8, -4.5f);       // Left wing
  tri(1, 2.5f, 1, -1, 8, -4.5f);          // Right wing
  tri(-1, -3.5f, -1, -5.5f, -4, -7.5f);   // Left tailplane
  tri(1, -3.5f, 1, -5.5f, 4, -7.5f);      // Right tailplane
}

static uint16_t altColor(int altFt)
{
  if(altFt < 0)      return C565(120, 120, 120);  // Ground
  if(altFt < 5000)   return C565(255, 140, 0);
  if(altFt < 15000)  return C565(255, 230, 0);
  if(altFt < 30000)  return C565(0, 220, 90);
  return C565(0, 200, 255);
}

static void altText(char *out, size_t sz, int altFt)
{
  if(altFt < 0) snprintf(out, sz, "GND");
  else if(altFt >= 18000) snprintf(out, sz, "FL%03d", altFt / 100);
  else snprintf(out, sz, "%dft", altFt);
}

// ---- Full-screen radar over the basemap --------------------------------------

static void chip(LGFX_Sprite &spr, int x, int y, int w, int h)
{
  spr.fillRectAlpha(x, y, w, h, 165, (uint32_t)0x000000);
}

static void dirText(char *out, size_t sz, float bearing)
{
  static const char *const dirs[] = {"N", "NE", "L", "SE", "S", "SO", "O", "NO"};
  float deg = bearing * 180.0f / (float)M_PI;
  if(deg < 0) deg += 360.0f;
  snprintf(out, sz, "%s", dirs[((int)(deg + 22.5f) / 45) & 7]);
}

void flightsDraw(LGFX_Sprite &spr)
{
  const int cx = MAP_W / 2, cy = MAP_H / 2;
  const uint16_t cDim = C565(170, 180, 190), cTxt = 0xFFFF, cHdr = C565(0, 200, 255);

  Aircraft snap[FLIGHTS_MAX];
  int n, total;
  char st[40];
  uint32_t okMs;
  snapshot(snap, &n, &total, st, &okMs);
  resolveSelection(snap, n);
  if(n == 0) showInfo = false;
  const LocationInfo loc = locationGet();
  const int radius = loc.radiusNm;

  spr.fillSprite(0x0000);

  // ---- Basemap ----
  const bool haveMap = mapDraw(spr, 0, 0);
  const float kPx = (float)MAP_R / (float)radius;           // Screen pixels per nautical mile
  spr.setTextDatum(TL_DATUM);

  // Scale bar (bottom left, above the summary chip)
  {
    static const int nice[] = {1, 2, 5, 10, 20, 50, 100, 200};
    int nm = nice[0];
    for(int v : nice) if(v * kPx <= 70.0f) nm = v;
    const int len = (int)(nm * kPx);
    const int sy = MAP_H - 22;
    spr.fillRectAlpha(3, sy - 9, len + 34, 12, 120, (uint32_t)0x000000);
    spr.drawFastHLine(6, sy, len, cDim);
    spr.drawFastVLine(6, sy - 3, 7, cDim);
    spr.drawFastVLine(6 + len, sy - 3, 7, cDim);
    char sb[12];
    snprintf(sb, sizeof(sb), "%dnm", nm);
    spr.setTextColor(cDim);
    spr.drawString(sb, 10 + len, sy - 4, FONT_DEFAULT);
  }
  (void)haveMap;
  char buf[64];

  // ---- Nearby airports (cached; recomputed when the position or range changes) ----
  {
    static NearbyAirport apt[16];
    static int aptN = 0;
    static float aLat = 999, aLon = 999;
    static int aRadius = -1;
    if(aLat != loc.lat || aLon != loc.lon || aRadius != radius)
    {
      aLat = loc.lat; aLon = loc.lon; aRadius = radius;
      aptN = locationNearbyAirports(loc.lat, loc.lon, (float)radius, (float)radius * (float)MAP_W / (float)MAP_H, apt, 16);
    }
    const uint16_t cApt = C565(120, 220, 255);
    for(int i = 0 ; i < aptN ; i++)
    {
      if(!strncmp(apt[i].icao, loc.icao, 4)) continue;
      const int ax = cx + (int)(apt[i].eastNm * kPx), ay = cy - (int)(apt[i].northNm * kPx);
      if(ax < 3 || ax > MAP_W - 3 || ay < 3 || ay > MAP_H - 3) continue;
      spr.drawLine(ax - 3, ay, ax, ay - 3, cApt); spr.drawLine(ax, ay - 3, ax + 3, ay, cApt);
      spr.drawLine(ax + 3, ay, ax, ay + 3, cApt); spr.drawLine(ax, ay + 3, ax - 3, ay, cApt);
      if(i < 8)   // Label only the nearest ones to keep the map readable
      {
        int lx = ax + 6;
        if(lx + 24 > MAP_W - 2) lx = ax - 30;
        spr.fillRectAlpha(lx - 1, ay - 5, 26, 10, 120, (uint32_t)0x000000);
        spr.setTextColor(cApt);
        spr.drawString(apt[i].icao, lx, ay - 4, FONT_DEFAULT);
      }
    }
  }

  // Own position and label (e.g. SJK)
  spr.fillCircle(cx, cy, 3, C565(255, 230, 0));
  spr.drawCircle(cx, cy, 5, C565(255, 230, 0));
  if(loc.label[0])
  {
    spr.setTextColor(C565(255, 230, 0));
    spr.drawString(loc.label, cx + 8, cy + 4, FONT_DEFAULT);
  }

  // ---- Aircraft (selected one on top) ----
  int selX = cx;
  for(int pass = 0 ; pass < 2 ; pass++)
  {
    for(int i = 0 ; i < n ; i++)
    {
      if((i == selIdx) != (pass == 1)) continue;
      const float r = snap[i].distNm * kPx;
      const int x = cx + (int)(r * sinf(snap[i].bearing));
      const int y = cy - (int)(r * cosf(snap[i].bearing));
      const uint16_t c = altColor(snap[i].altFt);
      if(snap[i].altFt < 0) spr.fillRect(x - 1, y - 1, 3, 3, c);   // On the ground
      else drawPlane(spr, x, y, snap[i].track * (float)M_PI / 180.0f, c, i == selIdx ? 1.15f : 0.85f);
      if(i == selIdx)
      {
        selX = x;
        spr.drawCircle(x, y, 11, cTxt);
        if(!showInfo)
        {
          const int len = strlen(snap[i].callsign) * 6;
          int lx = x + 14;
          if(lx + len > MAP_W - 4) lx = x - 14 - len;
          chip(spr, lx - 2, y - 6, len + 4, 12);
          spr.setTextColor(cTxt);
          spr.drawString(snap[i].callsign, lx, y - 4, FONT_DEFAULT);
        }
      }
    }
  }

  // ---- Top bar: count, selection, radius / status ----
  if(n > 0) snprintf(buf, sizeof(buf), "VOOS %d/%d  %dnm", selIdx + 1, total > n ? total : n, radius);
  else snprintf(buf, sizeof(buf), "VOOS 0  %dnm", radius);
  int w = strlen(buf) * 6 + 8;
  chip(spr, 3, 3, w, 12);
  spr.setTextColor(cHdr);
  spr.drawString(buf, 7, 5, FONT_DEFAULT);

  const uint32_t age = okMs ? (millis() - okMs) / 1000 : 0;
  snprintf(buf, sizeof(buf), "%s%s%us", st, okMs ? " " : "", okMs ? (unsigned)age : 0u);
  if(!okMs) snprintf(buf, sizeof(buf), "%s", st);
  w = strlen(buf) * 6 + 8;
  chip(spr, MAP_W - 3 - w, 3, w, 12);
  spr.setTextColor(cDim);
  spr.drawString(buf, MAP_W - w + 1, 5, FONT_DEFAULT);

  if(haveMap)
  {
    const char *at = "(c) OSM/CARTO";
    w = strlen(at) * 6 + 8;
    chip(spr, MAP_W - 3 - w, 17, w, 12);
    spr.setTextColor(cDim);
    spr.drawString(at, MAP_W - w + 1, 19, FONT_DEFAULT);
  }

  const char *ms = mapStatus();
  if(ms[0])
  {
    w = strlen(ms) * 6 + 8;
    chip(spr, 3, 17, w, 12);
    spr.setTextColor(cDim);
    spr.drawString(ms, 7, 19, FONT_DEFAULT);
  }

  if(n == 0)
  {
    chip(spr, cx - 48, cy + 24, 96, 14);
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(cDim);
    spr.drawString("Nenhum aviao", cx, cy + 31, FONT_DEFAULT);
    spr.setTextDatum(TL_DATUM);
    return;
  }

  const Aircraft &s = snap[selIdx];
  char altS[10], dirS[4];
  altText(altS, sizeof(altS), s.altFt);
  dirText(dirS, sizeof(dirS), s.bearing);

  if(!showInfo)
  {
    // Bottom bar: selected aircraft summary and altitude legend
    snprintf(buf, sizeof(buf), "%s  %s  %.1fnm %s", s.callsign, altS, s.distNm, dirS);
    w = strlen(buf) * 6 + 8;
    chip(spr, 3, MAP_H - 15, w, 12);
    spr.setTextColor(cTxt);
    spr.drawString(buf, 7, MAP_H - 13, FONT_DEFAULT);

    static const struct { int alt; const char *txt; int x; } leg[] = {
      {1000, "<5k", 0}, {10000, "<15k", 30}, {20000, "<30k", 66}, {35000, "+30k", 102}};
    const int lx0 = MAP_W - 3 - 134;
    chip(spr, lx0, MAP_H - 15, 134, 12);
    spr.setTextColor(cDim);
    for(const auto &l : leg)
    {
      spr.fillRect(lx0 + 5 + l.x, MAP_H - 11, 6, 6, altColor(l.alt));
      spr.drawString(l.txt, lx0 + 13 + l.x, MAP_H - 13, FONT_DEFAULT);
    }
    spr.setTextDatum(TL_DATUM);
    return;
  }

  // ---- Detail card (opened with a click), on the side opposite to the selected aircraft ----
  const int cw = 150, ch = 150;
  const int x0 = selX >= cx ? 4 : MAP_W - 4 - cw;
  const int y0 = 18;
  chip(spr, x0, y0, cw, ch);
  spr.fillRectAlpha(x0, y0, cw, ch, 60, (uint32_t)0x000000);
  spr.drawRect(x0, y0, cw, ch, C565(110, 140, 170));
  const uint16_t ac = altColor(s.altFt);
  spr.fillRect(x0 + 1, y0 + 1, 4, ch - 2, ac);

  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(cTxt);
  spr.drawString(s.callsign, x0 + 12, y0 + 5, FONT_LARGE);
  snprintf(buf, sizeof(buf), "%s%s%s", s.reg, (s.reg[0] && s.type[0]) ? "  " : "", s.type);
  spr.setTextColor(cHdr);
  spr.drawString(buf[0] ? buf : "-", x0 + 12, y0 + 33, FONT_SMALL);

  char vsS[16];
  if(s.hasVs) snprintf(vsS, sizeof(vsS), "%+d fpm", s.vs); else snprintf(vsS, sizeof(vsS), "-");
  char rows[5][24];
  static const char *const names[5] = {"ALT", "VEL", "RUMO", "V/S", "DIST"};
  if(s.altFt < 0) snprintf(rows[0], sizeof(rows[0]), "no solo");
  else if(s.altFt >= 18000) snprintf(rows[0], sizeof(rows[0]), "%s", altS);
  else snprintf(rows[0], sizeof(rows[0]), "%d ft", s.altFt);
  snprintf(rows[1], sizeof(rows[1]), "%d kt", s.speedKt);
  snprintf(rows[2], sizeof(rows[2]), "%03d", s.track);
  snprintf(rows[3], sizeof(rows[3]), "%s", vsS);
  snprintf(rows[4], sizeof(rows[4]), "%.1f nm %s", s.distNm, dirS);
  for(int k = 0 ; k < 5 ; k++)
  {
    const int yy = y0 + 52 + k * 15;
    spr.setTextColor(cDim);
    spr.drawString(names[k], x0 + 12, yy, FONT_SMALL);
    spr.setTextColor(k == 0 ? ac : cTxt);
    spr.drawString(rows[k], x0 + 56, yy, FONT_SMALL);
  }
  spr.setTextColor(cDim);
  snprintf(buf, sizeof(buf), "ICAO %s  clique: fechar", s.id);
  spr.drawString(buf, x0 + 10, y0 + ch - 13, FONT_DEFAULT);
}
