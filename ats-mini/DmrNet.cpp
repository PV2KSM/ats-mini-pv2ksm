// DMR live data. Runs only while the DMR screen is open.
//   BrandMeister: Socket.IO over a WebSocket (wss://api.brandmeister.network/lh/socket.io), event "mqtt", payload = JSON string with
//                 Event (Session-Start / Session-Stop), SourceCall, SourceName, DestinationID, DestinationName, Start, Stop.
//                 The feed carries the whole network: the radio keeps only the preferred talkgroup.
//   Hotspot:      the Pi-Star / WPSD dashboard page (mmdvmhost/lh.php) read over the local network.
#include <ESPmDNS.h>
#include "Dmr.h"
#include "Utils.h"
#include "I18n.h"
#include <Arduino.h>
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include <esp_heap_caps.h>
#include <esp_random.h>
#include <mbedtls/base64.h>
#include <string.h>
#include <stdlib.h>

extern const uint8_t caBundleStart[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t caBundleEnd[] asm("_binary_x509_crt_bundle_end");

#define MSG_MAX 6144

static char *msg = nullptr;      // One WebSocket message
static char *tmp = nullptr;      // Unescaped payload / hotspot page
static NetworkClientSecure *cli = nullptr;
static bool wsUp = false;
static uint32_t upAt = 0;
static char lastTxt[28] = "";
static uint32_t nMsg = 0, nEvt = 0, nDest = 0, nOk = 0, nFr = 0, nBytes = 0;
static uint8_t lastOp = 0;
static char dbgLast[48] = "";
static uint8_t eio = 4;          // Engine.IO version in use
static uint32_t pingMs = 25000, lastPing = 0, retryAt = 0;

// ---- tiny JSON helpers (flat objects) ----
static const char *jKey(const char *j, const char *key)     // Value start after "key" : (spaces allowed), or null
{
  char pat[40]; snprintf(pat, sizeof(pat), "\"%s\"", key);
  const char *p = j;
  while((p = strstr(p, pat)))
  {
    p += strlen(pat); while(*p == ' ') p++;
    if(*p != ':') continue;
    p++; while(*p == ' ') p++;
    return p;
  }
  return nullptr;
}
static bool jStr(const char *j, const char *key, char *out, size_t n)
{
  const char *p = jKey(j, key); if(!p || *p != '"') return false;
  p++; size_t i = 0;
  while(*p && *p != '"' && i + 1 < n) { if(*p == '\\' && p[1]) p++; out[i++] = *p++; }
  out[i] = 0; return true;
}
static bool jNum(const char *j, const char *key, double *v)
{
  const char *p = jKey(j, key); if(!p) return false;
  if(*p == '"') p++;
  if(!(*p == '-' || (*p >= '0' && *p <= '9'))) return false;
  *v = atof(p); return true;
}
static void clipUtf8(char *s, size_t n)         // Never end in the middle of a UTF-8 character
{
  if(strlen(s) < n) return;
  size_t k = n - 1; s[k] = 0; while(k && (s[k - 1] & 0xC0) == 0x80 && (s[k] & 0xC0) == 0x80) k--; if(k && ((unsigned char)s[k - 1] & 0x80) && ((unsigned char)s[k - 1] & 0xC0) != 0x80) s[k - 1] = 0;
}

// ---- WebSocket ----
static bool readN(uint8_t *b, size_t n)
{
  size_t got = 0; const uint32_t t0 = millis();
  while(got < n)
  {
    const int r = cli->read(b + got, n - got);
    if(r > 0) { got += r; continue; }
    if(!cli->connected() && !cli->available()) return false;
    if(millis() - t0 > 5000) return false;
    delay(2);
  }
  return true;
}
static void wsSend(uint8_t op, const char *text)
{
  const size_t n = strlen(text); uint8_t hdr[8]; size_t h = 0;
  hdr[h++] = 0x80 | op;
  if(n < 126) hdr[h++] = 0x80 | n; else { hdr[h++] = 0x80 | 126; hdr[h++] = n >> 8; hdr[h++] = n & 255; }
  uint8_t mask[4]; const uint32_t rnd = esp_random(); memcpy(mask, &rnd, 4);
  memcpy(hdr + h, mask, 4); h += 4;
  uint8_t buf[200]; if(n > sizeof(buf)) return;
  for(size_t i = 0 ; i < n ; i++) buf[i] = text[i] ^ mask[i & 3];
  cli->write(hdr, h); cli->write(buf, n);
}
static void wsClose()
{
  if(cli) { cli->stop(); delete cli; cli = nullptr; }
  wsUp = false;
}
// Opens the socket and does the HTTP upgrade for the given Engine.IO version
static char wsErr[40] = "";
static bool wsOpen(uint8_t ver)
{
  wsClose();
  cli = new NetworkClientSecure();
  cli->setCACertBundle(caBundleStart, caBundleEnd - caBundleStart);
  cli->setHandshakeTimeout(10);
  cli->setTimeout(5);
  if(!cli->connect("api.brandmeister.network", 443)) { char te[40] = ""; const int e = cli->lastError(te, sizeof(te)); snprintf(wsErr, sizeof(wsErr), "BM TLS %d (heap %uk)", e, (unsigned)(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024)); wsClose(); return false; }
  uint8_t kb[16]; for(int i = 0 ; i < 16 ; i += 4) { const uint32_t r = esp_random(); memcpy(kb + i, &r, 4); }
  unsigned char key[32]; size_t kl = 0; mbedtls_base64_encode(key, sizeof(key), &kl, kb, 16); key[kl] = 0;
  char req[420];
  snprintf(req, sizeof(req), "GET /lh/socket.io/?EIO=%u&transport=websocket HTTP/1.1\r\nHost: api.brandmeister.network\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
           "Sec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\nOrigin: https://brandmeister.network\r\nUser-Agent: ATS-Mini\r\n\r\n", ver, (char *)key);
  cli->write((const uint8_t *)req, strlen(req));
  // Read the HTTP answer by hand (up to 8 s) so a failure can say what came back
  char rsp[200]; size_t rn = 0; const uint32_t t0 = millis(); bool hdrEnd = false;
  while(!hdrEnd && millis() - t0 < 8000 && rn + 1 < sizeof(rsp))
  {
    if(cli->available()) { const int ch = cli->read(); if(ch < 0) continue; rsp[rn++] = (char)ch; rsp[rn] = 0; if(rn >= 4 && !memcmp(rsp + rn - 4, "\r\n\r\n", 4)) hdrEnd = true; }
    else if(!cli->connected()) break;
    else delay(5);
  }
  rsp[rn] = 0;
  if(!hdrEnd || !strstr(rsp, " 101"))
  {
    char first[32]; size_t k = 0; for(size_t i = 0 ; i < rn && i < 30 && rsp[i] != '\r' && rsp[i] != '\n' ; i++) first[k++] = rsp[i]; first[k] = 0;
    if(rn == 0) snprintf(wsErr, sizeof(wsErr), "BM: no reply (c%d)", (int)cli->connected());
    else snprintf(wsErr, sizeof(wsErr), "BM: %s", first);
    wsClose(); return false;
  }
  eio = ver; wsUp = true; upAt = millis(); lastTxt[0] = 0; lastPing = millis(); pingMs = 25000;
  return true;
}
// One message into msg; returns its length, 0 = nothing, -1 = connection lost
static int wsRead()
{
  size_t len = 0; bool first = true;
  for(;;)
  {
    if(first && !cli->available()) return cli->connected() ? 0 : -1;
    uint8_t h[2]; if(!readN(h, 2)) return -1;
    const uint8_t op = h[0] & 15; const bool fin = h[0] & 0x80; uint64_t n = h[1] & 127;
    if(n == 126) { uint8_t e[2]; if(!readN(e, 2)) return -1; n = (e[0] << 8) | e[1]; }
    else if(n == 127) { uint8_t e[8]; if(!readN(e, 8)) return -1; n = 0; for(int i = 0 ; i < 8 ; i++) n = (n << 8) | e[i]; }
    nFr++; lastOp = op; nBytes += (uint32_t)n;
    bool store = (op == 1 || op == 2 || op == 0) && len + n < MSG_MAX;     // some servers send text packets as binary frames
    size_t left = (size_t)n;
    if(op == 8) { uint8_t cb[2] = {0, 0}; if(n >= 2) { readN(cb, 2); } snprintf(dbgLast, sizeof(dbgLast), "closed by server: %d", (cb[0] << 8) | cb[1]); return -1; }
    while(left)
    {
      uint8_t blk[256]; const size_t c = left > sizeof(blk) ? sizeof(blk) : left;
      if(!readN(blk, c)) return -1;
      if(store) { memcpy(msg + len, blk, c); len += c; }
      if(op == 9 && len + c < 120) { /* ping payload ignored */ }
      left -= c;
    }
    if(op == 9) { uint8_t pong[2] = {0x8A, 0x80}; uint8_t m[4] = {1, 2, 3, 4}; cli->write(pong, 2); cli->write(m, 4); first = true; continue; }
    if(op == 10) { first = true; continue; }
    first = false;
    if(fin) { msg[len] = 0; return store ? (int)(len ? len : 0) : 0; }
  }
}

// Unescapes a JSON string body (the "payload" value) into dst as UTF-8
static void unescape(const char *p, char *dst, size_t n)
{
  size_t o = 0;
  while(*p && o + 5 < n)
  {
    if(*p == '"') break;
    if(*p == '\\' && p[1])
    {
      p++;
      if(*p == 'u' && p[1] && p[2] && p[3] && p[4])
      {
        char hx[5] = {p[1], p[2], p[3], p[4], 0}; const unsigned cp = strtoul(hx, nullptr, 16); p += 5;
        if(cp < 0x80) dst[o++] = cp; else if(cp < 0x800) { dst[o++] = 0xC0 | (cp >> 6); dst[o++] = 0x80 | (cp & 63); }
        else { dst[o++] = 0xE0 | (cp >> 12); dst[o++] = 0x80 | ((cp >> 6) & 63); dst[o++] = 0x80 | (cp & 63); }
        continue;
      }
      const char c = *p++;
      dst[o++] = c == 'n' ? ' ' : c == 't' ? ' ' : c;
      continue;
    }
    dst[o++] = *p++;
  }
  dst[o] = 0;
}

static void onEvent(const char *text)
{
  const char *pl = strstr(text, "\"payload\":\"");
  if(pl) unescape(pl + 11, tmp, MSG_MAX); else unescape(text, tmp, MSG_MAX);
  nEvt++;
  double dest = 0;
  if(!jNum(tmp, "DestinationID", &dest)) { if(!dbgLast[0]) { strlcpy(dbgLast, text, sizeof(dbgLast)); for(char *q = dbgLast ; *q ; q++) if(*q == '\\') *q = '/'; } return; }
  nDest++;
  DmrCfg c; dmrGetCfg(&c);
  if(c.tg && (uint32_t)dest != c.tg) return;
  char ev[24] = "", call[12] = "", name[40] = "", place[40] = "";
  jStr(tmp, "Event", ev, sizeof(ev)); if(!jStr(tmp, "SourceCall", call, sizeof(call)) || !call[0]) return;
  jStr(tmp, "SourceName", name, sizeof(name)); jStr(tmp, "DestinationName", place, sizeof(place));
  clipUtf8(name, 22); clipUtf8(place, 26);
  double sid = 0; jNum(tmp, "SourceID", &sid);
  double st = 0, sp = 0; jNum(tmp, "Start", &st); jNum(tmp, "Stop", &sp);
  nOk++;
  if(!strcmp(ev, "Session-Start")) dmrPush(call, name, place, (uint32_t)dest, 0, true, 0, false, (uint32_t)sid);
  else if(!strcmp(ev, "Session-Stop")) dmrPush(call, name, place, (uint32_t)dest, (int)(sp > st ? sp - st : 0), false, 0, false, (uint32_t)sid);
}

static void handleMsg(const char *t)
{
  switch(t[0])
  {
    case '0': { double pi = 0; if(jNum(t, "pingInterval", &pi) && pi > 1000) pingMs = (uint32_t)pi; if(eio >= 4) wsSend(1, "40"); break; }
    case '2': wsSend(1, "3"); break;
    case '4': if(t[1] == '0') dmrSetStatus(2, uiText("LIVE")); else if(t[1] == '2') onEvent(t); break;
    default: break;
  }
}

//
// Hotspot (Pi-Star / WPSD dashboard)
//
static void stripCell(const char *a, const char *b, char *out, size_t n)
{
  size_t o = 0; bool tag = false;
  for(const char *p = a ; p < b && o + 1 < n ; p++)
  {
    if(*p == '<') { tag = true; continue; } if(*p == '>') { tag = false; continue; }
    if(tag) continue;
    if(*p == '&') { const char *q = strchr(p, ';'); if(q && q - p < 8) { if(!strncmp(p, "&nbsp", 5)) out[o++] = ' '; p = q; continue; } }
    out[o++] = *p;
  }
  while(o && out[o - 1] == ' ') o--; size_t s = 0; while(s < o && out[s] == ' ') s++;
  memmove(out, out + s, o - s); out[o - s] = 0;
}
// Host may be an IP, a DNS name or a name.local (mDNS); returns "ip[:port]" in out
static bool resolveHost(const char *host, char *out, size_t n)
{
  static char cacheH[32] = "", cacheO[40] = ""; static uint32_t cacheT = 0;
  if(!strcmp(cacheH, host) && cacheO[0] && millis() - cacheT < 120000) { strlcpy(out, cacheO, n); return true; }
  char name[32]; strlcpy(name, host, sizeof(name)); char port[8] = "";
  char *colon = strchr(name, ':'); if(colon) { strlcpy(port, colon, sizeof(port)); *colon = 0; }
  IPAddress ip; bool ok = ip.fromString(name);
  if(!ok)
  {
    const size_t L = strlen(name); const bool local = L > 6 && !strcasecmp(name + L - 6, ".local");
    char bare[32]; strlcpy(bare, name, sizeof(bare)); if(local) bare[L - 6] = 0;
    if(local) { ip = MDNS.queryHost(bare, 2000); ok = ip != IPAddress((uint32_t)0); }
    if(!ok) ok = WiFi.hostByName(name, ip) == 1;
    if(!ok && !local) { ip = MDNS.queryHost(bare, 2000); ok = ip != IPAddress((uint32_t)0); }   // plain name such as pi-star
  }
  if(!ok) return false;
  snprintf(out, n, "%s%s", ip.toString().c_str(), port);
  strlcpy(cacheH, host, sizeof(cacheH)); strlcpy(cacheO, out, sizeof(cacheO)); cacheT = millis();
  return true;
}
// RadioID.net: name, city and DMR ID of a callsign (HTTPS, one short request, closed right after)
static void lookupOne()
{
  char call[12]; if(!dmrNextLookup(call, sizeof(call))) return;
  for(char *q = call ; *q ; q++) if(*q == '/') { *q = 0; break; }          // PY2ABC/P -> PY2ABC
  NetworkClientSecure *c = new NetworkClientSecure(); if(!c) return;
  c->setCACertBundle(caBundleStart, caBundleEnd - caBundleStart); c->setHandshakeTimeout(8); c->setTimeout(5);
  char name[24] = "", city[28] = "", country[24] = ""; double id = 0; bool ok = false;
  if(c->connect("radioid.net", 443))
  {
    char req[160]; snprintf(req, sizeof(req), "GET /api/dmr/user/?callsign=%s HTTP/1.0\r\nHost: radioid.net\r\nUser-Agent: ATS-Mini\r\nAccept: application/json\r\nConnection: close\r\n\r\n", call);
    c->write((const uint8_t *)req, strlen(req));
    size_t n = 0; const uint32_t t0 = millis();
    while(millis() - t0 < 6000 && n + 1 < MSG_MAX)
    {
      const int r = c->read((uint8_t *)tmp + n, MSG_MAX - 1 - n);
      if(r > 0) { n += r; continue; }
      if(!c->connected() && !c->available()) break;
      delay(10);
    }
    tmp[n] = 0;
    if(n > 12 && strstr(tmp, " 200"))
    {
      const char *body = strstr(tmp, "\r\n\r\n"); body = body ? body + 4 : tmp;
      const char *p;
      if((p = jKey(body, "fname")) && *p == '"') { unescape(p + 1, name, sizeof(name)); char *sp = strchr(name, ' '); if(sp) *sp = 0; }
      if((p = jKey(body, "city")) && *p == '"') unescape(p + 1, city, sizeof(city));
      if((p = jKey(body, "country")) && *p == '"') unescape(p + 1, country, sizeof(country));
      jNum(body, "id", &id); ok = name[0] || id > 0;
    }
  }
  c->stop(); delete c;
  if(!ok) { dmrSetInfo(call, "", "", 0); return; }
  char place[28] = ""; if(city[0]) { strlcpy(place, city, sizeof(place)); } else strlcpy(place, country, sizeof(place));
  clipUtf8(name, 22); clipUtf8(place, 26);
  dmrSetInfo(call, name, place, (uint32_t)id);
}

static int pollHotspot(const DmrCfg &c)
{
  if(!c.host[0]) { dmrSetStatus(3, uiText("Hotspot not found")); return 0; }
  char addr[40]; if(!resolveHost(c.host, addr, sizeof(addr))) { dmrSetStatus(3, uiText("Hotspot not found")); return 0; }
  static const char *const paths[] = {"/mmdvmhost/lh.php", "/"};
  int len = -1;
  for(const char *path : paths)
  {
    char url[96]; snprintf(url, sizeof(url), "http://%s%s", addr, path);
    WiFiClient plain; HTTPClient http; http.setConnectTimeout(3000); http.setTimeout(4000); http.setReuse(false);
    if(!http.begin(plain, url)) continue;
    const int code = http.GET();
    if(code == 200)
    {
      WiFiClient *s = http.getStreamPtr(); size_t n = 0; const uint32_t t0 = millis();
      while((http.connected() || s->available()) && n + 1 < MSG_MAX && millis() - t0 < 4000)
      {
        const int r = s->read((uint8_t *)tmp + n, MSG_MAX - 1 - n);
        if(r > 0) n += r; else delay(5);
      }
      tmp[n] = 0; len = (int)n; http.end(); break;
    }
    http.end();
  }
  if(len < 0) { dmrSetStatus(3, uiText("Hotspot not found")); return 0; }

  // Rows of the Last Heard table: Time | Mode | Callsign | Target | Src | Dur | Loss | BER
  struct Row { char call[12]; uint32_t tg; int dur; int ago; bool live; };
  Row rows[8]; int nr = 0;
  uint8_t nowH = 0, nowM = 0; const bool haveClock = clockGetLocalHM(&nowH, &nowM);
  const char *p = tmp;
  while(nr < 8 && (p = strstr(p, "<tr")))
  {
    const char *end = strstr(p, "</tr>"); if(!end) break;
    char cell[8][40]; int nc = 0; const char *q = p;
    while(nc < 8 && (q = strstr(q, "<td")) && q < end)
    {
      const char *gt = strchr(q, '>'); if(!gt || gt >= end) break;
      const char *te = strstr(gt, "</td>"); if(!te || te > end) te = end;
      stripCell(gt + 1, te, cell[nc], sizeof(cell[nc])); nc++; q = te;
    }
    p = end + 5;
    int mi = -1; for(int i = 0 ; i < nc ; i++) if(strstr(cell[i], "DMR")) { mi = i; break; }
    if(mi < 0 || mi + 3 >= nc) continue;
    Row &r = rows[nr]; memset(&r, 0, sizeof(r));
    strlcpy(r.call, cell[mi + 1], sizeof(r.call)); char *sl = strpbrk(r.call, " /"); if(sl) *sl = 0;
    const char *tgp = cell[mi + 2]; while(*tgp && (*tgp < '0' || *tgp > '9')) tgp++; r.tg = (uint32_t)atol(tgp);
    r.dur = 0; for(int i = mi + 3 ; i < nc ; i++) { if(!strcmp(cell[i], "TX")) { r.dur = 0; break; } if(cell[i][0] >= '0' && cell[i][0] <= '9') { r.dur = (int)atof(cell[i]); break; } }
    r.ago = nr;
    int hh, mm; if(haveClock && mi >= 1 && sscanf(cell[0], "%d:%d", &hh, &mm) == 2) { int d = (nowH * 60 + nowM) - (hh * 60 + mm); if(d < 0) d += 1440; r.ago = d; }
    for(int i = mi + 1 ; i < nc ; i++) if(!strncmp(cell[i], "TX", 2)) { r.live = true; break; }
    if(r.call[0]) nr++;
  }
  if(c.src == 1) dmrClearReal();
  for(int i = nr - 1 ; i >= 0 ; i--) dmrPush(rows[i].call, "", "Hotspot", rows[i].tg, rows[i].dur, rows[i].live, rows[i].ago, c.src == 2 && !rows[i].live);
  dmrSetStatus(2, uiText("LIVE"));
  return nr;
}

//
// Task
//
void dmrRun()
{
  if(!dmrWanted() || !msg || !tmp) return;
  uint32_t lastHot = 0, lastEx = 0, lastStat = 0, lastLk = 0;
  nMsg = nEvt = nDest = nOk = nFr = nBytes = 0; dbgLast[0] = 0;
  for(;;)
  {
    if(!dmrWanted()) { wsClose(); retryAt = 0; return; }
    DmrCfg c; dmrGetCfg(&c);
    if(WiFi.status() != WL_CONNECTED) { dmrSetStatus(3, tr(T_NO_WIFI)); wsClose(); vTaskDelay(pdMS_TO_TICKS(1000)); continue; }
    if(millis() - lastEx > 5000) { lastEx = millis(); dmrExpireLive(); }
    const bool useNet = false, useHot = true;      // hotspot only (the BrandMeister feed code stays for later)
    if(useHot && (!lastHot || millis() - lastHot > 4000)) { pollHotspot(c); lastHot = millis() | 1; }
    if(useHot && !wsUp && WiFi.status() == WL_CONNECTED && millis() - lastLk > 2500) { lastLk = millis(); lookupOne(); }
    if(!useNet) { wsClose(); vTaskDelay(pdMS_TO_TICKS(250)); continue; }
    if(c.net != 0) { if(!useHot) dmrSetStatus(3, uiText("No feed for this network")); wsClose(); vTaskDelay(pdMS_TO_TICKS(500)); continue; }
    if(!wsUp)
    {
      if(millis() < retryAt) { vTaskDelay(pdMS_TO_TICKS(200)); continue; }
      if(!clockGetDate(nullptr, nullptr, nullptr, nullptr)) { dmrSetStatus(3, tr(T_NO_TIME)); retryAt = millis() + 3000; continue; }
      dmrSetStatus(1, uiText("CONNECTING"));
      wsErr[0] = 0; bool ok = wsOpen(4); if(!ok && !strncmp(wsErr, "BM: HTTP", 8)) ok = wsOpen(3);
      if(!ok) { dmrSetStatus(3, wsErr[0] ? wsErr : "BM offline"); retryAt = millis() + 6000; }
      continue;
    }
    // Connected: read what arrived
    const int n = wsRead();
    if(n < 0) { wsClose(); retryAt = millis() + 2000; continue; }
    if(n > 0) { nMsg++; strlcpy(lastTxt, msg, sizeof(lastTxt)); for(char *q = lastTxt ; *q ; q++) if(*q == '"' || *q == '\\') *q = '\''; handleMsg(msg); }
    if(millis() - lastStat > 2000) { lastStat = millis(); char st[48]; snprintf(st, sizeof(st), "%s %lu/%lu/%lu/%lu f%lu o%u t%lus", uiText("LIVE"), (unsigned long)nMsg, (unsigned long)nEvt, (unsigned long)nDest, (unsigned long)nOk, (unsigned long)nFr, (unsigned)lastOp, (unsigned long)((millis() - upAt) / 1000)); dmrSetStatus(2, st); dmrSetDebug(dbgLast[0] ? dbgLast : lastTxt); }
    if(eio == 3 && millis() - lastPing > pingMs - 3000) { wsSend(1, "2"); lastPing = millis(); }
    if(n == 0) vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void dmrNetBegin()
{
  auto big = [](size_t n) -> char * { char *p = (char *)heap_caps_calloc(1, n, MALLOC_CAP_SPIRAM); return p ? p : (char *)calloc(1, n); };
  msg = big(MSG_MAX); tmp = big(MSG_MAX);
}
