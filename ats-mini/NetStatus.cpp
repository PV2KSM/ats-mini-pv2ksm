//
// Network status screen (menu item "Network"): Wi-Fi link, time, memory, last reset, and the
// state of every online service (flights, map, weather, METAR, rain radar, prices).
//
#include "NetStatus.h"
#include "Utils.h"
#include "Menu.h"
#include "Diag.h"
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <string.h>

// Implemented by the service modules
void flightsInfo(char *st, size_t size, uint32_t *okMs);
void mapInfo(char *st, size_t size, uint32_t *okMs);
void weatherInfo(char *st, size_t size, uint32_t *okMs);
void metarInfo(char *st, size_t size, uint32_t *okMs);
void radarInfo(char *st, size_t size, uint32_t *okMs);
void financeInfo(char *st, size_t size, uint32_t *okMs);

static int page = 0;
static const int PAGES = 2;

void netStatusOpen() { page = 0; }
void netStatusScroll(int16_t delta)
{
  if(delta > 0) page = (page + 1) % PAGES;
  else if(delta < 0) page = (page + PAGES - 1) % PAGES;
}

bool netStatusTick()
{
  if(currentCmd != CMD_NETWORK) return false;
  static uint32_t last = 0;
  const uint32_t now = millis();
  if((uint32_t)(now - last) >= 1000) { last = now; return true; }
  return false;
}

static inline uint16_t rgb(int r, int g, int b) { return lgfx::color565(r, g, b); }

static void ageText(char *out, size_t size, uint32_t okMs)
{
  if(!okMs) { snprintf(out, size, "nunca"); return; }
  const uint32_t s = (millis() - okMs) / 1000;
  if(s < 90) snprintf(out, size, "%us", (unsigned)s);
  else if(s < 5400) snprintf(out, size, "%umin", (unsigned)(s / 60));
  else snprintf(out, size, "%uh", (unsigned)(s / 3600));
}

void netStatusDraw(LGFX_Sprite &spr)
{
  const int W = 320, H = 170;
  for(int y = 0 ; y < H ; y++) spr.fillRect(0, y, W, 1, rgb(8 + y / 16, 20 + y / 9, 40 + y / 6));

  const uint16_t cW = 0xFFFF, cDim = rgb(160, 190, 215), cOk = rgb(120, 230, 170), cWarn = rgb(255, 200, 80), cBad = rgb(255, 120, 110);
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(cW);
  spr.drawString(page == 0 ? "REDE" : "SERVICOS", 8, 5, FONT_SMALL);
  for(int i = 0 ; i < PAGES ; i++)
  {
    const int dx = 316 - (PAGES - 1 - i) * 10;
    if(i == page) spr.fillCircle(dx, 12, 3, cW); else spr.drawCircle(dx, 12, 3, cDim);
  }

  char buf[96];

  if(page == 0)
  {
    const bool on = WiFi.status() == WL_CONNECTED;
    // Wi-Fi line with signal bars
    const int rssi = on ? WiFi.RSSI() : -100;
    spr.setTextColor(cDim); spr.drawString("Wi-Fi", 8, 28, FONT_DEFAULT);
    String ssid = on ? WiFi.SSID() : String("desconectado");
    snprintf(buf, sizeof(buf), "%.22s", ssid.c_str());
    spr.setTextColor(on ? cW : cBad); spr.drawString(buf, 8, 38, FONT_SMALL);
    int level = !on ? 0 : (rssi > -55 ? 4 : rssi > -65 ? 3 : rssi > -75 ? 2 : 1);
    for(int i = 0 ; i < 4 ; i++)
    {
      const int bh = 5 + i * 4;
      spr.fillRect(262 + i * 10, 52 - bh, 7, bh, i < level ? (level >= 3 ? cOk : cWarn) : rgb(50, 70, 95));
    }
    if(on) { snprintf(buf, sizeof(buf), "%d dBm", rssi); spr.setTextColor(cDim); spr.drawString(buf, 262, 56, FONT_DEFAULT); }

    // Detail grid
    struct Row { const char *k; char v[40]; };
    Row rows[6];
    rows[0].k = "IP";     snprintf(rows[0].v, sizeof(rows[0].v), "%s", on ? getWiFiIPAddress() : "-");
    rows[1].k = "Canal";  snprintf(rows[1].v, sizeof(rows[1].v), "%d", on ? (int)WiFi.channel() : 0);
    rows[2].k = "MAC";    snprintf(rows[2].v, sizeof(rows[2].v), "%s", getMACAddress());
    const char *clk = clockGet();
    rows[3].k = "Hora";   snprintf(rows[3].v, sizeof(rows[3].v), "%s %s %s", clk ? clk : "--:--", utcOffsets[utcOffsetIdx].desc, ntpIsAvailable() ? "NTP ok" : "sem NTP");
    const uint32_t up = millis() / 1000;
    rows[4].k = "Ligado"; snprintf(rows[4].v, sizeof(rows[4].v), "%uh %02um", (unsigned)(up / 3600), (unsigned)((up / 60) % 60));
    rows[5].k = "Memoria"; snprintf(rows[5].v, sizeof(rows[5].v), "heap %uk  psram %uk", (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024), (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
    for(int i = 0 ; i < 6 ; i++)
    {
      const int y = 66 + i * 15;
      spr.setTextColor(cDim); spr.drawString(rows[i].k, 8, y + 3, FONT_DEFAULT);
      spr.setTextColor(cW);   spr.drawString(rows[i].v, 62, y, FONT_SMALL);
    }

    // Last reset reason (first part only)
    char rst[200];
    diagText(rst, sizeof(rst));
    char *semi = strchr(rst, ';');
    if(semi) *semi = 0;
    snprintf(buf, sizeof(buf), "Ultimo reset: %.26s", rst);
    spr.setTextColor(rgb(130, 160, 185));
    spr.drawString(buf, 8, H - 11, FONT_DEFAULT);
  }
  else
  {
    struct Svc { const char *name; void (*info)(char *, size_t, uint32_t *); };
    static const Svc svcs[] =
    {
      { "Voos",    flightsInfo }, { "Mapa",    mapInfo },     { "Clima",   weatherInfo },
      { "METAR",   metarInfo },   { "Radar",   radarInfo },   { "Financas", financeInfo },
    };
    for(int i = 0 ; i < 6 ; i++)
    {
      char st[40] = "";
      uint32_t okMs = 0;
      svcs[i].info(st, sizeof(st), &okMs);
      const int y = 28 + i * 23;
      spr.fillRectAlpha(6, y, W - 12, 21, 80, (uint32_t)0x000000);
      const bool ok = okMs && !st[0];
      spr.fillRect(6, y, 3, 21, ok ? cOk : (okMs ? cWarn : rgb(90, 110, 130)));
      spr.setTextColor(cW); spr.drawString(svcs[i].name, 16, y + 3, FONT_SMALL);
      char age[16];
      ageText(age, sizeof(age), okMs);
      spr.setTextColor(cDim); spr.drawString(age, 96, y + 7, FONT_DEFAULT);
      spr.setTextColor(ok ? cOk : cWarn);
      spr.drawString(ok ? "OK" : (st[0] ? st : "aguardando"), 140, y + 3, FONT_SMALL);
    }
  }
}
