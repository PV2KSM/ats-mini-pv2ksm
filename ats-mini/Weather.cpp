//
// Weather screen: current conditions, 5-day forecast and next hours (Open-Meteo, no API key).
// Same HTTP/TLS approach as Flights.cpp: HTTP/1.1, CA bundle shared with OTA, body read with
// writeToStream() so chunked responses work, parsed from a bounded buffer.
//
#include "Weather.h"
#include "Location.h"
#include "Diag.h"
#include "NetFetch.h"
#include "Radar.h"
#define WX_DATUM(d) ((lgfx::textdatum_t)(d))
#define WX_PAGES 5
#include "WeatherUI.h"
#include "Utils.h"
#include "Menu.h"
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_heap_caps.h>
#include <string.h>
#include <math.h>
#include "I18n.h"

#define WX_REFRESH_MS     (10UL * 60UL * 1000UL)   // While the screen stays open
#define WX_STALE_MS       (5UL * 60UL * 1000UL)    // Refetch on open if older than this
#define WX_BODY_PSRAM     (96 * 1024)
#define WX_BODY_HEAP      (24 * 1024)
#define WX_TASK_STACK     16384

extern const uint8_t caBundleStart[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t caBundleEnd[] asm("_binary_x509_crt_bundle_end");

static SemaphoreHandle_t mtx = nullptr;
static WxData data;                       // Last good data (valid=false until the first fetch works)
static char status[40] = "";
static uint32_t lastOkMs = 0;
static uint32_t version = 0;
static volatile bool wakeNow = false;
static volatile bool locationDirty = false;

static char *body = nullptr;
static size_t bodyCap = 0;

namespace {
class WxSink : public Stream
{
public:
  size_t len = 0;
  bool overflow = false;
  size_t write(uint8_t c) override { return write(&c, 1); }
  size_t write(const uint8_t *d, size_t n) override
  {
    if(len + n > bodyCap) { overflow = true; return n; }
    memcpy(body + len, d, n);
    len += n;
    return n;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
};
}

static void lock()   { if(mtx) xSemaphoreTake(mtx, portMAX_DELAY); }
static void unlock() { if(mtx) xSemaphoreGive(mtx); }

static void setStatus(const char *s)
{
  lock();
  snprintf(status, sizeof(status), "%s", s ? s : "Erro");
  version++;
  unlock();
}

static bool allocBody()
{
  if(body) return true;
  body = (char *)heap_caps_malloc(WX_BODY_PSRAM, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if(body) { bodyCap = WX_BODY_PSRAM; return true; }
  body = (char *)malloc(WX_BODY_HEAP);
  if(body) { bodyCap = WX_BODY_HEAP; return true; }
  return false;
}

static void copyStr(char *dst, size_t size, const char *src) { strlcpy(dst, src ? src : "", size); }

static bool parseWeather(JsonDocument &doc, WxData &w)
{
  JsonObject c = doc["current"];
  if(c.isNull()) return false;
  memset(&w, 0, sizeof(w));
  w.temp     = wxR(c["temperature_2m"] | 0.0f);
  w.feels    = wxR(c["apparent_temperature"] | 0.0f);
  w.hum      = wxR(c["relative_humidity_2m"] | 0.0f);
  w.code     = c["weather_code"] | 0;
  w.cloud    = wxR(c["cloud_cover"] | 0.0f);
  w.isDay    = c["is_day"] | 1;
  w.pressure = wxR(c["pressure_msl"] | 0.0f);
  w.wind     = wxR(c["wind_speed_10m"] | 0.0f);
  w.windDir  = wxR(c["wind_direction_10m"] | 0.0f);
  w.gust     = wxR(c["wind_gusts_10m"] | 0.0f);
  w.precip   = c["precipitation"] | 0.0f;

  JsonObject dd = doc["daily"];
  JsonArray dt = dd["time"];
  int n = 0;
  for(JsonVariant v : dt)
  {
    if(n >= 6) break;
    WxDay &d = w.days[n];
    copyStr(d.date, sizeof(d.date), v.as<const char *>());
    d.code = dd["weather_code"][n] | 0;
    d.tmax = wxR(dd["temperature_2m_max"][n] | 0.0f);
    d.tmin = wxR(dd["temperature_2m_min"][n] | 0.0f);
    d.pop  = wxR(dd["precipitation_probability_max"][n] | 0.0f);
    d.uv   = wxR(dd["uv_index_max"][n] | 0.0f);
    const char *sr = dd["sunrise"][n] | "";
    const char *ss = dd["sunset"][n] | "";
    copyStr(d.sunrise, sizeof(d.sunrise), strlen(sr) >= 16 ? sr + 11 : "--:--");
    copyStr(d.sunset,  sizeof(d.sunset),  strlen(ss) >= 16 ? ss + 11 : "--:--");
    n++;
  }
  w.nDays = n;
  if(!n) return false;

  // Next hours, starting at the current hour ("YYYY-MM-DDTHH")
  JsonObject hh = doc["hourly"];
  JsonArray ht = hh["time"];
  const char *now = c["time"] | "";
  int start = 0, i = 0;
  for(JsonVariant v : ht)
  {
    const char *s = v.as<const char *>();
    if(s && strlen(now) >= 13 && !strncmp(s, now, 13)) { start = i; break; }
    i++;
  }
  int h = 0;
  for(i = start ; i < (int)ht.size() && h < 12 ; i++, h++)
  {
    const char *s = ht[i] | "";
    w.hours[h].hour = strlen(s) >= 13 ? atoi(s + 11) : 0;
    w.hours[h].temp = wxR(hh["temperature_2m"][i] | 0.0f);
    w.hours[h].pop  = wxR(hh["precipitation_probability"][i] | 0.0f);
  }
  w.nHours = h;
  w.valid = true;
  return true;
}

static void fetchWeatherLocked();
static void fetchWeather()
{
  if(!netLock(200)) { wakeNow = true; return; }
  diagStage(DG_WX_FETCH);
  fetchWeatherLocked();
  netUnlock();
}

static void fetchWeatherLocked()
{
  LocationInfo loc = locationGet();
  if(WiFi.status() != WL_CONNECTED) { setStatus(tr(T_NO_WIFI)); return; }
  if(!clockGetDate(nullptr, nullptr, nullptr, nullptr)) { setStatus(tr(T_NO_TIME)); return; }
  if(!allocBody()) { setStatus(tr(T_NO_MEM)); return; }

  char url[480];
  snprintf(url, sizeof(url),
    "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
    "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,weather_code,"
    "cloud_cover,pressure_msl,wind_speed_10m,wind_direction_10m,wind_gusts_10m"
    "&hourly=temperature_2m,precipitation_probability"
    "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,sunrise,sunset,uv_index_max"
    "&forecast_days=6&timezone=auto", loc.lat, loc.lon);

  NetworkClientSecure client;
  client.setCACertBundle(caBundleStart, caBundleEnd - caBundleStart);
  client.setHandshakeTimeout(10);

  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(15000);
  http.setUserAgent(getUserAgent());
  http.setReuse(false);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if(!http.begin(client, url)) { setStatus("Falha http.begin"); return; }
  http.addHeader("Accept", "application/json");

  int code = http.GET();
  if(code != HTTP_CODE_OK)
  {
    char msg[40], tls[80] = "";
    if(code > 0) snprintf(msg, sizeof(msg), tr(T_HTTP_ERR), code);
    else if(client.lastError(tls, sizeof(tls)) != 0) snprintf(msg, sizeof(msg), tr(T_TLS_ERR), code);
    else snprintf(msg, sizeof(msg), tr(T_NET_ERR), code);
    http.end();
    setStatus(msg);
    return;
  }

  WxSink sink;
  int wrote = http.writeToStream(&sink);
  http.end();
  if(sink.overflow) { setStatus(tr(T_BIG_RESP)); return; }
  if(wrote < 0 || sink.len == 0) { setStatus(tr(T_READ_FAIL)); return; }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body, sink.len);
  if(err) { setStatus(tr(T_JSON_ERR)); return; }

  WxData fresh;
  if(!parseWeather(doc, fresh)) { setStatus(tr(T_BAD_DATA)); return; }

  lock();
  data = fresh;
  lastOkMs = millis();
  status[0] = 0;
  version++;
  unlock();
}

// ---- METAR (aviationweather.gov, raw text) ---------------------------------

static char metarRaw[220] = "";
static char metarIcao[5] = "";
static char metarStatus[32] = "";
static uint32_t metarOkMs = 0;

static void fetchMetar()
{
  const LocationInfo loc = locationGet();
  if(!loc.icao[0])
  {
    lock(); metarRaw[0] = 0; snprintf(metarStatus, sizeof(metarStatus), "%s", tr(T_SET_ICAO)); version++; unlock();
    return;
  }
  char url[96], body[420], err[40];
  snprintf(url, sizeof(url), "https://aviationweather.gov/api/data/metar?ids=%s&format=raw", loc.icao);
  int code = netFetchText(url, body, sizeof(body), err, sizeof(err), "text/plain");
  lock();
  if(code == 200)
  {
    // First line only, without the METAR/SPECI prefix
    char *nl = strpbrk(body, "\r\n");
    if(nl) *nl = 0;
    const char *p = body;
    if(!strncmp(p, "METAR ", 6) || !strncmp(p, "SPECI ", 6)) p += 6;
    if(!*p) snprintf(metarStatus, sizeof(metarStatus), tr(T_NO_METAR), loc.icao);
    else
    {
      strlcpy(metarRaw, p, sizeof(metarRaw));
      strlcpy(metarIcao, loc.icao, sizeof(metarIcao));
      metarOkMs = millis();
      metarStatus[0] = 0;
    }
  }
  else if(code == 204 || code == -105) snprintf(metarStatus, sizeof(metarStatus), tr(T_NO_METAR), loc.icao);
  else snprintf(metarStatus, sizeof(metarStatus), "%s", err);
  version++;
  unlock();
}

// Decoded pieces of a METAR (all optional)
struct MetarInfo
{
  char time[8] = "";      // 1215Z
  char wind[20] = "";     // 270/08kt G15
  char vis[16] = "";      // 9999 -> ">10km"
  char ceil[14] = "";     // BKN020 / sem teto
  char temp[16] = "";     // 23/18
  char qnh[12] = "";      // 1015
  char wx[16] = "";       // RA TS BR FG ...
};

static bool isDigits(const char *s, int n) { for(int i = 0 ; i < n ; i++) if(!isdigit((unsigned char)s[i])) return false; return true; }

static void decodeMetar(const char *raw, MetarInfo *m)
{
  char buf[220];
  strlcpy(buf, raw, sizeof(buf));
  bool haveCeil = false, haveCloud = false;
  int ceilFt = 0;
  char *save = nullptr;
  int idx = 0;
  for(char *t = strtok_r(buf, " ", &save) ; t ; t = strtok_r(nullptr, " ", &save), idx++)
  {
    const int n = strlen(t);
    if(!strcmp(t, "RMK")) break;
    if(idx <= 2 && n == 7 && isDigits(t, 6) && t[6] == 'Z') { snprintf(m->time, sizeof(m->time), "%.2s%.2sZ", t + 2, t + 4); continue; }
    if(n >= 7 && t[n - 2] == 'K' && t[n - 1] == 'T' && (isDigits(t, 5) || !strncmp(t, "VRB", 3)))
    {
      const bool vrb = !strncmp(t, "VRB", 3);
      int spd = atoi(vrb ? t + 3 : t + 3), gust = 0;
      char *g = strchr(t, 'G');
      if(g) gust = atoi(g + 1);
      if(vrb) snprintf(m->wind, sizeof(m->wind), "VRB %dkt", spd);
      else if(spd == 0) snprintf(m->wind, sizeof(m->wind), "%s", tr(T_CALM));
      else if(gust) snprintf(m->wind, sizeof(m->wind), "%.3s %dG%dkt", t, spd, gust);
      else snprintf(m->wind, sizeof(m->wind), "%.3s %dkt", t, spd);
      continue;
    }
    if(!strcmp(t, "CAVOK")) { snprintf(m->vis, sizeof(m->vis), ">10km"); snprintf(m->ceil, sizeof(m->ceil), "CAVOK"); haveCeil = true; continue; }
    if(n == 4 && isDigits(t, 4) && !m->vis[0])
    {
      const int v = atoi(t);
      if(v >= 9999) snprintf(m->vis, sizeof(m->vis), ">10km"); else if(v >= 1000) snprintf(m->vis, sizeof(m->vis), "%d.%dkm", v / 1000, (v % 1000) / 100); else snprintf(m->vis, sizeof(m->vis), "%dm", v);
      continue;
    }
    if(n >= 3 && t[n - 2] == 'S' && t[n - 1] == 'M') { snprintf(m->vis, sizeof(m->vis), "%.*s mi", n - 2, t); continue; }
    if((!strncmp(t, "FEW", 3) || !strncmp(t, "SCT", 3) || !strncmp(t, "BKN", 3) || !strncmp(t, "OVC", 3)) && n >= 6 && isDigits(t + 3, 3))
    {
      haveCloud = true;
      if((t[0] == 'B' || t[0] == 'O') && (!haveCeil || atoi(t + 3) * 100 < ceilFt))
      {
        ceilFt = atoi(t + 3) * 100;
        snprintf(m->ceil, sizeof(m->ceil), "%.3s %dft", t, ceilFt);
        haveCeil = true;
      }
      continue;
    }
    if(!strcmp(t, "NSC") || !strcmp(t, "SKC") || !strcmp(t, "CLR") || !strcmp(t, "NCD")) { if(!haveCeil) snprintf(m->ceil, sizeof(m->ceil), "%s", tr(T_NO_CLOUDS)); haveCeil = true; continue; }
    if(n >= 5 && n <= 7 && strchr(t, '/') && (isdigit((unsigned char)t[0]) || t[0] == 'M'))
    {
      const char *sl = strchr(t, '/');
      auto val = [](const char *s) { return (s[0] == 'M') ? -atoi(s + 1) : atoi(s); };
      snprintf(m->temp, sizeof(m->temp), "%d/%dC", val(t), val(sl + 1));
      continue;
    }
    if(t[0] == 'Q' && n == 5 && isDigits(t + 1, 4)) { snprintf(m->qnh, sizeof(m->qnh), "%s hPa", t + 1); continue; }
    if(t[0] == 'A' && n == 5 && isDigits(t + 1, 4)) { snprintf(m->qnh, sizeof(m->qnh), "%.2s.%.2s inHg", t + 1, t + 3); continue; }
    // Present weather (letters only, e.g. -RA TSRA BR FG)
    if(idx >= 3 && n >= 2 && n <= 7 && !m->wx[0] && (isalpha((unsigned char)t[0]) || t[0] == '-' || t[0] == '+') && strncmp(t, "FEW", 3) && strncmp(t, "SCT", 3) && strncmp(t, "BKN", 3) && strncmp(t, "OVC", 3) && strncmp(t, "NOSIG", 5) && strncmp(t, "TEMPO", 5) && strncmp(t, "BECMG", 5))
    { bool letters = true; for(int i = (t[0] == '-' || t[0] == '+'); i < n; i++) if(!isupper((unsigned char)t[i])) letters = false; if(letters) strlcpy(m->wx, t, sizeof(m->wx)); }
  }
  (void)haveCloud;
}

static void drawMetarPage(LGFX_Sprite &spr, int kind, bool day, const char *place, float t)
{
  char raw[220], icao[5], st[32];
  uint32_t okMs;
  lock();
  strlcpy(raw, metarRaw, sizeof(raw));
  strlcpy(icao, metarIcao, sizeof(icao));
  strlcpy(st, metarStatus, sizeof(st));
  okMs = metarOkMs;
  unlock();
  (void)t; (void)place;

  const int W = 320, H = 170;
  WxBg bg = wxBackground(kind, day, H);
  for(int y = 0 ; y < H ; y++) spr.fillRect(0, y, W, 1, bg.at(y));
  const uint16_t cW = wxRgb(255, 255, 255), cDim = wxRgb(206, 220, 238), cSh = wxRgb(8, 20, 44), cWarn = wxRgb(255, 214, 90);

  char buf[64];
  snprintf(buf, sizeof(buf), "METAR %s", icao[0] ? icao : "");
  wxText(spr, buf, 8, 4, FONT_SMALL, cW, 0, cSh);
  for(int i = 0 ; i < WX_PAGES ; i++)
  {
    int dx = 316 - (WX_PAGES - 1 - i) * 10;
    if(i == 3) spr.fillCircle(dx, 12, 3, cW); else spr.drawCircle(dx, 12, 3, cDim);
  }

  if(!raw[0])
  {
    wxText(spr, st[0] ? st : tr(T_SEARCHING_METAR), 160, 80, FONT_SMALL, st[0] ? cWarn : cW, 4, cSh);
    return;
  }

  MetarInfo m;
  decodeMetar(raw, &m);
  const int ageMin = okMs ? (int)((millis() - okMs) / 60000UL) : 0;
  if(st[0]) wxText(spr, st, 306 - WX_PAGES * 10, 8, FONT_DEFAULT, cWarn, 2, cSh);
  else
  {
    snprintf(buf, sizeof(buf), "%s%s", m.time, m.time[0] ? "  " : ""); { size_t l = strlen(buf); snprintf(buf + l, sizeof(buf) - l, tr(T_FETCHED), ageMin); }
    wxText(spr, buf, 306 - WX_PAGES * 10, 8, FONT_DEFAULT, cDim, 2, cSh);
  }

  struct Cell { const char *label; const char *value; };
  const Cell cells[6] = {
    {tr(T_WIND), m.wind}, {tr(T_VISIB), m.vis}, {tr(T_CEIL), m.ceil},
    {tr(T_TEMPDEW), m.temp}, {"QNH", m.qnh}, {tr(T_WX), m.wx},
  };
  for(int i = 0 ; i < 6 ; i++)
  {
    const int x = 8 + (i % 3) * 104, y = 26 + (i / 3) * 42;
    wxChip(spr, x, y, 100, 38);
    wxText(spr, cells[i].label, x + 6, y + 4, FONT_DEFAULT, cDim, 0);
    wxText(spr, cells[i].value[0] ? cells[i].value : "-", x + 6, y + 18, FONT_SMALL, cW, 0);
  }

  // Raw message, wrapped to the screen (6 px characters)
  wxChip(spr, 6, 112, 308, 52);
  const int perLine = 50;
  int y = 116;
  const char *p = raw;
  for(int line = 0 ; line < 4 && *p ; line++)
  {
    int len = strlen(p);
    if(len > perLine)
    {
      len = perLine;
      while(len > 0 && p[len] != ' ') len--;
      if(len == 0) len = perLine;
    }
    char ln[perLine + 2];
    memcpy(ln, p, len);
    ln[len] = 0;
    wxText(spr, ln, 10, y, FONT_DEFAULT, cW, 0);
    y += 12;
    p += len;
    while(*p == ' ') p++;
  }
}

// The worker fetches while the Weather screen is open
static bool weatherWanted()
{
  return currentCmd == CMD_WEATHER;
}

bool weatherGetSnapshot(WxData *out, char *st, size_t stSize)
{
  lock();
  *out = data;
  if(st) strlcpy(st, status, stSize);
  bool ok = data.valid;
  unlock();
  return ok;
}

static void weatherTask(void *)
{
  uint32_t last = 0;
  for(;;)
  {
    if(weatherWanted())
    {
      // Refresh every WX_REFRESH_MS; after a failure (no data yet) retry every minute
      const uint32_t period = data.valid ? WX_REFRESH_MS : 60000UL;
      bool due = wakeNow || !last || (uint32_t)(millis() - last) >= period;
      if(due)
      {
        wakeNow = false;
        fetchWeather();
        if(currentCmd == CMD_WEATHER) fetchMetar();
        last = millis() | 1;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void weatherBegin()
{
  if(!mtx) mtx = xSemaphoreCreateMutex();
  if(!mtx) return;
  memset(&data, 0, sizeof(data));
  xTaskCreatePinnedToCore(weatherTask, "weather", WX_TASK_STACK, nullptr, 1, nullptr, 1);
}

void weatherLocationChanged()
{
  locationDirty = true;
  lastOkMs = 0;   // Force a refetch next time the screen is opened
  wakeNow = true;
}

static int page = 0;

void weatherOpen()
{
  page = 0;
  lock();
  bool stale = !data.valid || locationDirty || !lastOkMs || (uint32_t)(millis() - lastOkMs) > WX_STALE_MS;
  if(stale) { snprintf(status, sizeof(status), "%s", tr(T_SEARCHING_WX)); version++; }
  const bool metarOld = !metarOkMs || (uint32_t)(millis() - metarOkMs) > WX_REFRESH_MS;
  unlock();
  locationDirty = false;
  if(stale || metarOld) wakeNow = true;
}

bool weatherIsActive() { return currentCmd == CMD_WEATHER; }

void weatherInfo(char *st, size_t size, uint32_t *okMs)
{
  lock(); strlcpy(st, status, size); *okMs = lastOkMs; unlock();
}

void metarInfo(char *st, size_t size, uint32_t *okMs)
{
  lock(); strlcpy(st, metarStatus, size); *okMs = metarOkMs; unlock();
}
int weatherPage() { return page; }

// Encoder click: zoom on the radar page. False = not handled (the screen closes).
bool weatherClick()
{
  if(page != 4) return false;
  radarToggleZoom();
  return true;
}

void weatherScroll(int16_t delta)
{
  if(delta > 0) page = (page + 1) % WX_PAGES;
  else if(delta < 0) page = (page + WX_PAGES - 1) % WX_PAGES;
  if(page == 4) radarWake();
}

// Redraw ~8 times per second for the animated icons while the screen is open
bool weatherTick()
{
  static uint32_t shownMs = 0;
  if(!weatherIsActive()) return false;
  uint32_t now = millis();
  if((uint32_t)(now - shownMs) >= (page == 4 ? 500UL : 120UL)) { shownMs = now; return true; }
  return false;
}

void weatherDraw(LGFX_Sprite &spr)
{
  WxData snap;
  char st[40];
  uint32_t okMs;
  lock();
  snap = data;
  memcpy(st, status, sizeof(st));
  okMs = lastOkMs;
  unlock();

  LocationInfo loc = locationGet();
  char place[40];
  snprintf(place, sizeof(place), "%s  %s", loc.label[0] ? loc.label : "", loc.name);
  int ageMin = okMs ? (int)((millis() - okMs) / 60000UL) : 0;

  if(page == 4)
  {
    radarDraw(spr);
    spr.pushSprite(0, 0);
    return;
  }
  if(page == 3)
  {
    const int kind = snap.valid ? wxKind(snap.code) : WX_CLOUDY;
    drawMetarPage(spr, kind, snap.valid ? snap.isDay != 0 : false, place, (float)(millis() % 100000UL) / 1000.0f);
    spr.pushSprite(0, 0);
    return;
  }
  wxDrawScreen(spr, snap, place, st, ageMin, page, (float)(millis() % 100000UL) / 1000.0f,
               FONT_DEFAULT, FONT_SMALL, FONT_LARGE, FONT_DIGITS);
  spr.pushSprite(0, 0);
}
