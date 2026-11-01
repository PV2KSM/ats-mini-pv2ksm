//
// News screen: headlines and a short summary.
//   Source 1: Google News feed of the chosen country edition (top stories and/or topics).
//   Source 2: a search feed for a city or subject, when one is set on the web page.
//   Source 3: any site or feed address typed on the web page. If it is a normal page the radio looks for
//             its feed (<link rel="alternate" type="application/rss+xml">) in the head of the page.
// Feeds are RSS 2.0 or Atom. Text is folded to ASCII (the screen fonts have no accents).
// The Google News feed address format is not officially documented and may change.
//
#include "News.h"
#include "NetFetch.h"
#include "Utils.h"
#include "Menu.h"
#include "Themes.h"
#include "I18n.h"
#include "NewUI.h"
#include <Preferences.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <esp_heap_caps.h>
NuPal nuCurrentPalette();

#define NEWS_MAX        24                      // Items kept
#define NEWS_HEAD       112
#define NEWS_SUM        176
#define NEWS_SRC        28
#define NEWS_LINK       1001                    // Article address (QR code); longer ones are dropped
#define NEWS_BUF_BIG    (160 * 1024)            // Download buffer (PSRAM)
#define NEWS_BUF_SMALL  (28 * 1024)             // Fallback buffer (internal heap)
#define NEWS_TASK_STACK 12288
#define NEWS_MAX_SRC    11

struct NewsItem
{
  char head[NEWS_HEAD];
  char sum[NEWS_SUM];
  char src[NEWS_SRC];
  char link[NEWS_LINK];
  uint16_t linkLen;                             // Real address length (the address itself is dropped above NEWS_LINK)
  uint32_t ts;                                  // Publication time (UTC epoch), 0 = unknown
  uint8_t from;                                 // Source number it came from
};

struct Edition { const char *name, *hl, *gl, *ceid; };
// Latin-script editions only: the screen font is ASCII
static const Edition EDITIONS[] =
{
  {"Brazil", "pt-BR", "BR", "BR:pt-419"},
  {"Portugal", "pt-PT", "PT", "PT:pt-150"},
  {"United States", "en-US", "US", "US:en"},
  {"United Kingdom", "en-GB", "GB", "GB:en"},
  {"Canada", "en-CA", "CA", "CA:en"},
  {"Australia", "en-AU", "AU", "AU:en"},
  {"India", "en-IN", "IN", "IN:en"},
  {"Spain", "es", "ES", "ES:es"},
  {"Mexico",         "es-419", "MX", "MX:es-419"},
  {"Argentina",      "es-419", "AR", "AR:es-419"},
  {"Chile",          "es-419", "CL", "CL:es-419"},
  {"Colombia",       "es-419", "CO", "CO:es-419"},
  {"France", "fr", "FR", "FR:fr"},
  {"Germany", "de", "DE", "DE:de"},
  {"Italy", "it", "IT", "IT:it"},
  {"Netherlands", "nl", "NL", "NL:nl"},
  {"Poland", "pl", "PL", "PL:pl"},
};
#define EDITION_COUNT ((int)(sizeof(EDITIONS) / sizeof(EDITIONS[0])))
static const char *const ED_LABEL[][LANG_TOTAL] =   // Names on the radio screen (ASCII), per language: pt, en, es
{
  {"Brasil", "Brazil", "Brasil"},   // Brazil
  {"Portugal", "Portugal", "Portugal"},   // Portugal
  {"Estados Unidos", "United States", "Estados Unidos"},   // United States
  {"Reino Unido", "United Kingdom", "Reino Unido"},   // United Kingdom
  {"Canada", "Canada", "Canada"},   // Canada
  {"Australia", "Australia", "Australia"},   // Australia
  {"India", "India", "India"},   // India
  {"Espanha", "Spain", "Espana"},   // Spain
  {"Mexico", "Mexico", "Mexico"},   // Mexico
  {"Argentina", "Argentina", "Argentina"},   // Argentina
  {"Chile", "Chile", "Chile"},   // Chile
  {"Colombia", "Colombia", "Colombia"},   // Colombia
  {"Franca", "France", "Francia"},   // France
  {"Alemanha", "Germany", "Alemania"},   // Germany
  {"Italia", "Italy", "Italia"},   // Italy
  {"Holanda", "Netherlands", "Paises Bajos"},   // Netherlands
  {"Polonia", "Poland", "Polonia"},   // Poland
};
const char *newsEditionLabel(int i) { return (i >= 0 && i < EDITION_COUNT) ? ED_LABEL[i][uiLang < LANG_TOTAL ? uiLang : LANG_EN] : ""; }


static const char *const TOPIC_NAME[NEWS_TOPICS] = {"Top stories", "World", "Nation", "Business", "Technology", "Science", "Sports", "Health", "Entertainment"};
static const char *const TOPIC_CODE[NEWS_TOPICS] = {"", "WORLD", "NATION", "BUSINESS", "TECHNOLOGY", "SCIENCE", "SPORTS", "HEALTH", "ENTERTAINMENT"};

int newsEditionCount() { return EDITION_COUNT; }
const char *newsEditionName(int i) { return (i >= 0 && i < EDITION_COUNT) ? EDITIONS[i].name : ""; }
const char *newsTopicName(int i) { return (i >= 0 && i < NEWS_TOPICS) ? TOPIC_NAME[i] : ""; }

static SemaphoreHandle_t mtx = nullptr;
static NewsCfg cfg;
static NewsCfg pendingCfg;
static bool havePending = false;

// Both arrays live in PSRAM: the internal memory is needed for the TLS connection (see newsBegin)
static NewsItem *items = nullptr;               // Shown
static NewsItem *work = nullptr;                // Being filled by the worker
static int nItems = 0;
static char feedLabel[24] = "";
static char status[80] = "";
static uint32_t lastOkMs = 0;
static uint32_t version = 0, shownVersion = 0xFFFFFFFF;
static volatile bool wakeNow = false;
static int cur = 0;
static uint32_t lastSwitchMs = 0;
static bool paused = false;
static bool qrOpen = false;
static uint32_t qrSince = 0;

// Test button
static volatile bool testReq = false;
static char testUrl[160] = "";
static int testState = 0, testCount = 0;
static int gLinkLong = 0;                         // Links dropped for being longer than NEWS_LINK (test statistics)
static char testMsg[160] = "", testHead[NEWS_HEAD] = "", testSum[NEWS_SUM] = "";

static void lock()   { if(mtx) xSemaphoreTake(mtx, portMAX_DELAY); }
static void unlock() { if(mtx) xSemaphoreGive(mtx); }

static void setStatus(const char *s)
{
  lock();
  snprintf(status, sizeof(status), "%s", s ? s : "");
  version++;
  unlock();
}

// ---- Settings --------------------------------------------------------------

static int autoEdition()
{
  if(uiLang == LANG_PT) return 0;
  if(uiLang == LANG_ES) return 7;
  return 2;
}

static void cfgDefaults(NewsCfg *c)
{
  memset(c, 0, sizeof(*c));
  c->edition = (uint8_t)autoEdition();
  c->topics = 1;
  c->refreshMin = 30;
  c->secs = 12;
  c->show = 3;
}

void newsGetCfg(NewsCfg *out)
{
  lock();
  *out = cfg;
  unlock();
  if(out->edition >= EDITION_COUNT) out->edition = (uint8_t)autoEdition();
}

const char *newsSetCfg(const NewsCfg &in)
{
  if(in.edition >= EDITION_COUNT) return "Unknown country.";
  NewsCfg c = in;
  c.topics &= (uint16_t)((1u << NEWS_TOPICS) - 1);
  c.query[sizeof(c.query) - 1] = 0;
  c.url[sizeof(c.url) - 1] = 0;
  // Trim spaces at both ends
  for(char *f : {c.query, c.url})
  {
    size_t n = strlen(f);
    while(n && isspace((unsigned char)f[n - 1])) f[--n] = 0;
    size_t lead = 0;
    while(f[lead] && isspace((unsigned char)f[lead])) lead++;
    if(lead) memmove(f, f + lead, strlen(f + lead) + 1);
  }
  if(c.url[0])
  {
    if(strchr(c.url, ' ')) return "The address cannot have spaces.";
    if(strlen(c.url) < 4 || !strchr(c.url, '.')) return "That does not look like a web address.";
    if(!strstr(c.url, "://"))
    {
      char tmp[sizeof(c.url)];
      snprintf(tmp, sizeof(tmp), "https://%s", c.url);
      strlcpy(c.url, tmp, sizeof(c.url));
    }
    if(strncasecmp(c.url, "https://", 8)) return "Only https:// addresses are supported.";
  }
  if(c.refreshMin != 15 && c.refreshMin != 30 && c.refreshMin != 60) c.refreshMin = 30;
  if(c.secs < 6) c.secs = 6;
  if(c.secs > 30) c.secs = 30;
  c.show &= 3;
  if(!c.topics && !c.query[0] && !c.url[0]) c.topics = 1;
  lock();
  pendingCfg = c;
  havePending = true;
  unlock();
  return nullptr;
}

static void loadCfg()
{
  cfgDefaults(&cfg);
  Preferences p;
  if(!p.begin("news", true)) return;
  const uint8_t ed = p.getUChar("ed", 255);
  if(ed < EDITION_COUNT) cfg.edition = ed;
  cfg.topics = p.getUShort("top", cfg.topics);
  String q = p.getString("q", ""); strlcpy(cfg.query, q.c_str(), sizeof(cfg.query));
  String u = p.getString("url", ""); strlcpy(cfg.url, u.c_str(), sizeof(cfg.url));
  cfg.refreshMin = p.getUChar("ref", cfg.refreshMin);
  cfg.secs = p.getUChar("sec", cfg.secs);
  cfg.show = p.getUChar("show", cfg.show);
  p.end();
  if(cfg.refreshMin != 15 && cfg.refreshMin != 30 && cfg.refreshMin != 60) cfg.refreshMin = 30;
  if(cfg.secs < 6 || cfg.secs > 30) cfg.secs = 12;
  if(!cfg.topics && !cfg.query[0] && !cfg.url[0]) cfg.topics = 1;
}

// ---- Text helpers ----------------------------------------------------------

#include "NewsFold.h"   // FOLD[]: Latin-1 / Latin Extended-A letters folded to ASCII

// Append the ASCII form of one Unicode code point
static void putCp(char *out, size_t cap, size_t &n, uint32_t cp)
{
  auto add = [&](const char *s) { while(*s && n + 1 < cap) out[n++] = *s++; };
  if(cp < 0x80) { if(cp < 32) cp = ' '; if(n + 1 < cap) out[n++] = (char)cp; return; }
  if(cp >= 0xC0 && cp <= 0x17F) { add(FOLD[cp - 0xC0]); return; }
  switch(cp)
  {
    case 0xA0: case 0x2002: case 0x2003: case 0x2009: case 0x202F: add(" "); break;
    case 0x2018: case 0x2019: case 0x201A: case 0x2032: add("'"); break;
    case 0x201C: case 0x201D: case 0x201E: case 0x2033: case 0xAB: case 0xBB: add("\""); break;
    case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014: case 0x2015: case 0x2212: add("-"); break;
    case 0x2026: add("..."); break;
    case 0x2022: case 0xB7: add("*"); break;
    case 0x20AC: add("EUR"); break;
    case 0xA3: add("GBP"); break;
    case 0xA9: add("(c)"); break;
    case 0xAE: add("(R)"); break;
    case 0x2122: add("TM"); break;
    case 0xB0: add("o"); break;
    case 0xBD: add("1/2"); break;
    default: break;                              // Not drawable: skipped
  }
}

// Decode the character at s (UTF-8). Returns the code point, advances s.
static uint32_t nextCp(const char *&s)
{
  const uint8_t c = (uint8_t)*s;
  if(c < 0x80) { s++; return c; }
  int extra = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : (c >= 0xC0) ? 1 : 0;
  uint32_t cp = extra == 3 ? (c & 7) : extra == 2 ? (c & 15) : extra == 1 ? (c & 31) : 0xFFFD;
  s++;
  for(int i = 0 ; i < extra && ((uint8_t)*s & 0xC0) == 0x80 ; i++) cp = (cp << 6) | ((uint8_t)*s++ & 0x3F);
  return cp;
}

// Raw feed text (may have CDATA, entities, tags) -> short ASCII text
static void cleanText(const char *s, const char *e, char *out, size_t cap)
{
  out[0] = 0;
  if(!s || !e || e <= s || cap < 2) return;
  if(e - s > 1500) e = s + 1500;
  // CDATA wrapper
  while(s < e && isspace((unsigned char)*s)) s++;
  if(e - s >= 12 && !strncmp(s, "<![CDATA[", 9)) { s += 9; const char *x = e; while(x > s + 2 && !(x[-1] == '>' && x[-2] == ']' && x[-3] == ']')) x--; e = (x > s + 2) ? x - 3 : e; }

  // Phase 1: decode entities into a UTF-8 scratch string
  static char tmp[1700];
  size_t n = 0;
  for(const char *p = s ; p < e && n + 5 < sizeof(tmp) ;)
  {
    if(*p != '&') { tmp[n++] = *p++; continue; }
    const char *semi = (const char *)memchr(p, ';', (e - p) > 10 ? 10 : (e - p));
    if(!semi) { tmp[n++] = *p++; continue; }
    const size_t L = semi - p - 1;
    uint32_t cp = 0; bool ok = true;
    if(L == 3 && !strncmp(p + 1, "amp", 3)) cp = '&';
    else if(L == 2 && !strncmp(p + 1, "lt", 2)) cp = '<';
    else if(L == 2 && !strncmp(p + 1, "gt", 2)) cp = '>';
    else if(L == 4 && !strncmp(p + 1, "quot", 4)) cp = '"';
    else if(L == 4 && !strncmp(p + 1, "apos", 4)) cp = '\'';
    else if(L == 4 && !strncmp(p + 1, "nbsp", 4)) cp = ' ';
    else if(L >= 2 && p[1] == '#')
    {
      char num[10]; size_t k = 0;
      for(const char *q = p + 2 ; q < semi && k < sizeof(num) - 1 ; q++) num[k++] = *q;
      num[k] = 0;
      cp = (num[0] == 'x' || num[0] == 'X') ? (uint32_t)strtoul(num + 1, nullptr, 16) : (uint32_t)strtoul(num, nullptr, 10);
      if(!cp) ok = false;
    }
    else ok = false;
    if(!ok) { tmp[n++] = *p++; continue; }
    // Emit as UTF-8
    if(cp < 0x80) tmp[n++] = (char)cp;
    else if(cp < 0x800) { tmp[n++] = (char)(0xC0 | (cp >> 6)); tmp[n++] = (char)(0x80 | (cp & 0x3F)); }
    else { tmp[n++] = (char)(0xE0 | (cp >> 12)); tmp[n++] = (char)(0x80 | ((cp >> 6) & 0x3F)); tmp[n++] = (char)(0x80 | (cp & 0x3F)); }
    p = semi + 1;
  }
  tmp[n] = 0;

  // Phase 2: drop tags, fold to ASCII, collapse spaces
  size_t o = 0;
  bool inTag = false, space = true;
  for(const char *p = tmp ; *p && o + 4 < cap ;)
  {
    if(inTag) { if(*p == '>') inTag = false; p++; continue; }
    if(*p == '<' && (isalpha((unsigned char)p[1]) || p[1] == '/' || p[1] == '!')) { inTag = true; if(!space) { out[o++] = ' '; space = true; } p++; continue; }
    const uint32_t cp = nextCp(p);
    char one[8]; size_t k = 0;
    putCp(one, sizeof(one), k, cp);
    for(size_t i = 0 ; i < k && o + 4 < cap ; i++)
    {
      if(one[i] == ' ') { if(!space) { out[o++] = ' '; space = true; } }
      else { out[o++] = one[i]; space = false; }
    }
  }
  while(o && out[o - 1] == ' ') o--;
  out[o] = 0;
}

// ---- Dates -----------------------------------------------------------------

static uint32_t parseDate(const char *s, const char *e)
{
  char b[48];
  size_t n = 0;
  while(s < e && isspace((unsigned char)*s)) s++;
  while(s < e && n < sizeof(b) - 1) b[n++] = *s++;
  b[n] = 0;
  while(n && isspace((unsigned char)b[n - 1])) b[--n] = 0;
  if(n < 10) return 0;
  int y = 0, mo = 0, d = 0, h = 0, mi = 0, sec = 0, tz = 0;
  if(isdigit((unsigned char)b[0]) && b[4] == '-')                       // 2026-10-28T14:03:00Z / +03:00
  {
    if(sscanf(b, "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &sec) < 5) { sec = 0; if(sscanf(b, "%d-%d-%d", &y, &mo, &d) < 3) return 0; }
    const char *z = strpbrk(b + 10, "+-Z");
    if(z && *z != 'Z' && strlen(z) >= 6) tz = (z[0] == '-' ? -1 : 1) * (atoi(z + 1) * 60 + atoi(z + 4));
  }
  else                                                                  // Wed, 28 Oct 2026 14:03:00 GMT / +0200
  {
    const char *p = b;
    const char *comma = strchr(b, ',');
    if(comma) p = comma + 1;
    char mon[4] = "";
    if(sscanf(p, " %d %3s %d %d:%d:%d", &d, mon, &y, &h, &mi, &sec) < 5) { sec = 0; if(sscanf(p, " %d %3s %d %d:%d", &d, mon, &y, &h, &mi) < 5) return 0; }
    static const char *const MON = "JanFebMarAprMayJunJulAugSepOctNovDec";
    mo = 0;
    for(int i = 0 ; i < 12 ; i++) if(!strncasecmp(mon, MON + i * 3, 3)) { mo = i + 1; break; }
    if(!mo) return 0;
    if(y < 100) y += 2000;
    const char *z = strrchr(b, ' ');
    if(z && (z[1] == '+' || z[1] == '-') && strlen(z + 1) >= 5) tz = (z[1] == '-' ? -1 : 1) * ((z[2] - '0') * 600 + (z[3] - '0') * 60 + (z[4] - '0') * 10 + (z[5] - '0'));
  }
  uint32_t epoch;
  if(!clockUTCDateTimeToEpoch(y, mo, d, h, mi, sec, &epoch)) return 0;
  const int64_t t = (int64_t)epoch - (int64_t)tz * 60;
  return t > 0 ? (uint32_t)t : 0;
}

// ---- Feed parsing ----------------------------------------------------------

static const char *sfind(const char *s, const char *e, const char *pat)
{
  const size_t L = strlen(pat);
  if(e - s < (long)L) return nullptr;
  for(const char *p = s ; p + L <= e ; p++)
    if(*p == *pat && !memcmp(p, pat, L)) return p;
  return nullptr;
}

// Content of <tag ...>...</tag> inside [s, e). Matches "<tag" followed by > space or /
static bool getTag(const char *s, const char *e, const char *tag, const char **cs, const char **ce)
{
  char open[24], close[24];
  snprintf(open, sizeof(open), "<%s", tag);
  snprintf(close, sizeof(close), "</%s>", tag);
  const size_t ol = strlen(open);
  const char *p = s;
  for(;;)
  {
    p = sfind(p, e, open);
    if(!p) return false;
    const char c = p[ol];
    if(c == '>' || c == ' ' || c == '/' || c == '\t' || c == '\r' || c == '\n') break;
    p += ol;
  }
  const char *gt = (const char *)memchr(p, '>', e - p);
  if(!gt) return false;
  if(gt[-1] == '/') { *cs = *ce = gt + 1; return true; }
  const char *end = sfind(gt + 1, e, close);
  if(!end) return false;
  *cs = gt + 1; *ce = end;
  return true;
}

// Case-insensitive substring search (strcasestr is not always available)
static const char *cifind(const char *s, const char *pat)
{
  const size_t L = strlen(pat);
  if(!L) return s;
  for( ; *s ; s++) if(!strncasecmp(s, pat, L)) return s;
  return nullptr;
}

static const char *cifindN(const char *s, const char *e, const char *pat)
{
  const size_t L = strlen(pat);
  for( ; s + L <= e ; s++) if(!strncasecmp(s, pat, L)) return s;
  return nullptr;
}

static bool endsWith(const char *s, const char *suffix)
{
  const size_t a = strlen(s), b = strlen(suffix);
  return a >= b && !strcasecmp(s + a - b, suffix);
}

// Fills out[0..] from the feed text. Returns the number of items.
// Article address of an item: <link>text</link> (RSS) or <link href="..."/> (Atom). Empty when missing or too long.
static int getLink(const char *it, const char *ie, char *out, size_t cap)
{
  out[0] = 0;
  int found = 0;                                    // Real length of the address, even when it is dropped
  const char *a, *z;
  const char *p = it;
  for(;;)
  {
    p = cifindN(p, ie, "<link");
    if(!p) return 0;
    const char *gt = (const char *)memchr(p, '>', ie - p);
    if(!gt) return 0;
    if(gt[-1] == '/' || memchr(p, '=', gt - p))
    {
      // Atom style attribute; skip rel="self" and similar
      char tag[300]; size_t L = gt - p; if(L >= sizeof(tag)) L = sizeof(tag) - 1;
      memcpy(tag, p, L); tag[L] = 0;
      p = gt + 1;
      if(cifind(tag, "rel=\"self\"") || cifind(tag, "rel=\"enclosure\"") || cifind(tag, "rel=\"replies\"")) continue;
      const char *hr = cifind(tag, "href=");
      if(!hr) continue;
      hr += 5;
      const char q = *hr;
      if(q != '"' && q != '\'') continue;
      hr++;
      const char *qe = strchr(hr, q);
      if(!qe) continue;
      size_t n = 0;
      for(const char *c = hr ; c < qe && n + 1 < cap ; c++) out[n++] = *c;
      out[n] = 0;
      found = (int)(qe - hr);
      if(qe - hr >= (long)cap - 1) { out[0] = 0; gLinkLong++; }
      break;
    }
    // RSS: text between the tags
    if(!getTag(p, ie, "link", &a, &z)) return 0;
    while(a < z && (*a == ' ' || *a == '\n' || *a == '\r' || *a == '\t')) a++;
    if(z - a >= 12 && !strncmp(a, "<![CDATA[", 9)) { a += 9; if(z - a >= 3 && !strncmp(z - 3, "]]>", 3)) z -= 3; }
    while(z > a && (z[-1] == ' ' || z[-1] == '\n' || z[-1] == '\r' || z[-1] == '\t')) z--;
    found = (int)(z - a);
    if(z - a >= (long)cap - 1) { gLinkLong++; return found; }
    size_t n = 0;
    for(const char *c = a ; c < z && n + 1 < cap ; c++) out[n++] = *c;
    out[n] = 0;
    break;
  }
  // &amp; -> &
  for(char *q = out ; *q ; q++) if(!strncmp(q, "&amp;", 5)) memmove(q + 1, q + 5, strlen(q + 5) + 1);
  if(strncasecmp(out, "http", 4)) out[0] = 0;
  return found;
}

static int parseFeed(const char *b, size_t len, bool google, NewsItem *out, int maxOut, uint8_t from)
{
  const char *end = b + len;
  const bool atom = !sfind(b, end, "<item") && sfind(b, end, "<entry");
  const char *open = atom ? "<entry" : "<item", *close = atom ? "</entry>" : "</item>";
  const size_t ol = strlen(open);
  int n = 0;
  const char *p = b;
  while(n < maxOut)
  {
    const char *it = sfind(p, end, open);
    if(!it) break;
    const char c = it[ol];
    if(!(c == '>' || c == ' ' || c == '\t' || c == '\r' || c == '\n')) { p = it + ol; continue; }
    const char *ie = sfind(it, end, close);
    if(!ie) break;                                // Cut by the buffer: drop the last, incomplete one
    p = ie + strlen(close);

    const char *a, *z;
    NewsItem &m = out[n];
    memset(&m, 0, sizeof(m));
    m.from = from;
    if(!getTag(it, ie, "title", &a, &z)) continue;
    cleanText(a, z, m.head, sizeof(m.head));
    if(!m.head[0]) continue;
    if(getTag(it, ie, "source", &a, &z)) cleanText(a, z, m.src, sizeof(m.src));
    // Google puts "Headline - Outlet" in the title
    if(google)
    {
      if(m.src[0])
      {
        char suf[NEWS_SRC + 4];
        snprintf(suf, sizeof(suf), " - %s", m.src);
        if(endsWith(m.head, suf)) m.head[strlen(m.head) - strlen(suf)] = 0;
      }
      else
      {
        char *dash = strstr(m.head, " - ");
        char *last = nullptr;
        while(dash) { last = dash; dash = strstr(dash + 3, " - "); }
        if(last && strlen(last + 3) < NEWS_SRC) { strlcpy(m.src, last + 3, sizeof(m.src)); *last = 0; }
      }
    }
    // Summary: not for Google (its description is a list of links), and never a copy of the title
    if(!google)
    {
      if(getTag(it, ie, "description", &a, &z) || getTag(it, ie, "summary", &a, &z)) cleanText(a, z, m.sum, sizeof(m.sum));
      const size_t hl = strlen(m.head);
      const size_t cmp = hl < 24 ? hl : 24;
      if(cmp && !strncasecmp(m.sum, m.head, cmp)) m.sum[0] = 0;
    }
    m.linkLen = (uint16_t)getLink(it, ie, m.link, sizeof(m.link));
    if(getTag(it, ie, "pubDate", &a, &z) || getTag(it, ie, "published", &a, &z) || getTag(it, ie, "updated", &a, &z) || getTag(it, ie, "dc:date", &a, &z))
      m.ts = parseDate(a, z);
    n++;
  }
  return n;
}

static bool looksLikeFeed(const char *b, size_t len)
{
  const char *end = b + (len > 3000 ? 3000 : len);
  const char *x = sfind(b, end, "<rss"); if(x) return true;
  x = sfind(b, end, "<feed"); if(x) return true;
  x = sfind(b, end, "<rdf:RDF"); if(x) return true;
  if(sfind(b, end, "<?xml") && (sfind(b, b + len, "<item") || sfind(b, b + len, "<entry"))) return true;
  return false;
}

// Looks for <link rel="alternate" type="application/rss+xml|atom+xml" href="..."> in a page
static bool findFeedLink(const char *b, size_t len, const char *base, char *out, size_t cap)
{
  const char *end = b + len;
  for(const char *p = b ; p < end ;)
  {
    p = cifindN(p, end, "<link");
    if(!p) break;
    const char *gt = (const char *)memchr(p, '>', end - p);
    if(!gt) break;
    char tag[400];
    size_t L = gt - p; if(L >= sizeof(tag)) L = sizeof(tag) - 1;
    memcpy(tag, p, L); tag[L] = 0;
    p = gt + 1;
    if(!cifind(tag, "rss+xml") && !cifind(tag, "atom+xml")) continue;
    const char *h = cifind(tag, "href");
    if(!h) continue;
    h += 4;
    while(*h == ' ' || *h == '=') h++;
    const char q = *h;
    if(q != '"' && q != '\'') continue;
    h++;
    const char *he = strchr(h, q);
    if(!he) continue;
    char href[220]; size_t hl = he - h; if(hl >= sizeof(href)) continue;
    memcpy(href, h, hl); href[hl] = 0;
    if(!strncasecmp(href, "http", 4)) strlcpy(out, href, cap);
    else if(href[0] == '/' && href[1] == '/') snprintf(out, cap, "https:%s", href);
    else
    {
      // Relative: scheme + host from the page address
      const char *s = strstr(base, "://");
      if(!s) return false;
      const char *slash = strchr(s + 3, '/');
      const size_t hostLen = slash ? (size_t)(slash - base) : strlen(base);
      if(href[0] == '/') snprintf(out, cap, "%.*s%s", (int)hostLen, base, href);
      else snprintf(out, cap, "%.*s/%s", (int)hostLen, base, href);
    }
    return out[0] != 0;
  }
  return false;
}

// ---- Fetching --------------------------------------------------------------

static char lastNetErr[80] = "";                  // Last connection error text from the fetch layer

static char *bufAlloc(size_t *cap)
{
  char *b = (char *)heap_caps_malloc(NEWS_BUF_BIG, MALLOC_CAP_SPIRAM);
  if(b) { *cap = NEWS_BUF_BIG; return b; }
  b = (char *)malloc(NEWS_BUF_SMALL);
  *cap = b ? NEWS_BUF_SMALL : 0;
  return b;
}

static const char *ACCEPT = "application/rss+xml, application/atom+xml, application/xml;q=0.9, text/xml;q=0.9, text/html;q=0.8";

// Result codes below 0: -100 no Wi-Fi, -101 no time, -104 too big, -105 read failed (from netFetch);
// -200 feed without items, -201 no feed in the page, -202 no memory. > 0: HTTP status.
static int loadSource(const char *url, bool google, NewsItem *out, int maxOut, uint8_t from, bool discover)
{
  size_t cap = 0;
  char *buf = bufAlloc(&cap);
  if(!buf) return -202;
  char err[80];
  size_t len = 0;
  int code = netFetchTextTrunc(url, buf, cap, err, sizeof(err), &len, ACCEPT);
  if(code < 0) strlcpy(lastNetErr, err, sizeof(lastNetErr));
  int result = code;
  if(code == 200)
  {
    if(!looksLikeFeed(buf, len))
    {
      char feedUrl[220] = "";
      if(discover && findFeedLink(buf, len, url, feedUrl, sizeof(feedUrl)))
      {
        len = 0;
        code = netFetchTextTrunc(feedUrl, buf, cap, err, sizeof(err), &len, ACCEPT);
        if(code < 0) strlcpy(lastNetErr, err, sizeof(lastNetErr));
        result = code;
        if(code == 200 && !looksLikeFeed(buf, len)) result = -201;
      }
      else result = -201;
    }
    if(result == 200)
    {
      const int n = parseFeed(buf, len, google, out, maxOut, from);
      result = n > 0 ? n : -200;                  // n > 0: number of items (never collides with 200 as maxOut <= 24)
    }
  }
  free(buf);
  return result;
}

static void errText(int code, char *out, size_t size)
{
  switch(code)
  {
    case -100: snprintf(out, size, "%s", tr(T_NO_WIFI)); break;
    case -101: snprintf(out, size, "%s", tr(T_NO_TIME)); break;
    case -104: snprintf(out, size, "%s", tr(T_BIG_RESP)); break;
    case -105: snprintf(out, size, "%s", tr(T_READ_FAIL)); break;
    case -200: snprintf(out, size, "%s", tr(T_NEWS_EMPTY)); break;
    case -201: snprintf(out, size, "%s", tr(T_NEWS_NO_FEED)); break;
    case -202: snprintf(out, size, "%s", tr(T_NO_MEM)); break;
    default:
      if(code > 0) snprintf(out, size, tr(T_HTTP_ERR), code);
      else if(lastNetErr[0]) snprintf(out, size, "%s", lastNetErr);
      else snprintf(out, size, tr(T_NET_ERR), code);
  }
}

static void urlEncode(const char *s, char *out, size_t cap)
{
  size_t n = 0;
  for( ; *s && n + 4 < cap ; s++)
  {
    const uint8_t c = (uint8_t)*s;
    if(isalnum(c) || c == '-' || c == '_' || c == '.') out[n++] = (char)c;
    else n += snprintf(out + n, cap - n, "%%%02X", c);
  }
  out[n] = 0;
}

struct Source { char url[240]; bool google; char label[24]; };
static Source *gSrc = nullptr;                    // Only used by the worker task (PSRAM)

static void gnUrl(char *out, size_t cap, const Edition &e, int topic, const char *query)
{
  if(query && *query)
  {
    char q[160];
    urlEncode(query, q, sizeof(q));
    snprintf(out, cap, "https://news.google.com/rss/search?q=%s&hl=%s&gl=%s&ceid=%s", q, e.hl, e.gl, e.ceid);
  }
  else if(topic == 0) snprintf(out, cap, "https://news.google.com/rss?hl=%s&gl=%s&ceid=%s", e.hl, e.gl, e.ceid);
  else snprintf(out, cap, "https://news.google.com/rss/headlines/section/topic/%s?hl=%s&gl=%s&ceid=%s", TOPIC_CODE[topic], e.hl, e.gl, e.ceid);
}

static const char *hostOf(const char *url, char *out, size_t cap)
{
  const char *s = strstr(url, "://");
  s = s ? s + 3 : url;
  if(!strncasecmp(s, "www.", 4)) s += 4;
  size_t n = 0;
  while(s[n] && s[n] != '/' && s[n] != ':' && n + 1 < cap) { out[n] = s[n]; n++; }
  out[n] = 0;
  return out;
}

static int buildSources(const NewsCfg &c, Source *src)
{
  int n = 0;
  const Edition &e = EDITIONS[c.edition < EDITION_COUNT ? c.edition : autoEdition()];
  if(c.query[0] && n < NEWS_MAX_SRC)
  {
    gnUrl(src[n].url, sizeof(src[n].url), e, 0, c.query);
    src[n].google = true;
    strlcpy(src[n].label, newsEditionLabel((int)(&e - EDITIONS)), sizeof(src[n].label));
    n++;
  }
  for(int t = 0 ; t < NEWS_TOPICS && n < NEWS_MAX_SRC - 1 ; t++)
    if(c.topics & (1u << t))
    {
      gnUrl(src[n].url, sizeof(src[n].url), e, t, nullptr);
      src[n].google = true;
      strlcpy(src[n].label, newsEditionLabel((int)(&e - EDITIONS)), sizeof(src[n].label));
      n++;
    }
  if(c.url[0] && n < NEWS_MAX_SRC)
  {
    strlcpy(src[n].url, c.url, sizeof(src[n].url));
    src[n].google = !strncasecmp(c.url, "https://news.google.", 20);
    hostOf(c.url, src[n].label, sizeof(src[n].label));
    n++;
  }
  return n;
}

static void fetchNews()
{
  if(!items || !work || !gSrc) { setStatus(tr(T_NO_MEM)); return; }
  NewsCfg c;
  lock(); c = cfg; unlock();
  Source *src = gSrc;
  const int ns = buildSources(c, src);
  if(!ns) { setStatus(tr(T_NEWS_NONE)); return; }

  const int quota = (NEWS_MAX / ns) < 3 ? 3 : (NEWS_MAX / ns);
  int start[NEWS_MAX_SRC + 1], total = 0, firstErr = 0;
  for(int i = 0 ; i < ns && total < NEWS_MAX ; i++)
  {
    start[i] = total;
    const int room = NEWS_MAX - total;
    const int want = quota < room ? quota : room;
    const int r = loadSource(src[i].url, src[i].google, work + total, want, (uint8_t)i, true);
    if(r > 0) total += r;
    else if(!firstErr) firstErr = r;
  }
  for(int i = ns ; i <= NEWS_MAX_SRC ; i++) start[i] = total;

  if(total == 0)
  {
    char msg[80];
    errText(firstErr ? firstErr : -200, msg, sizeof(msg));
    setStatus(msg);
    return;
  }

  // Interleave the sources so every refresh mixes them
  uint8_t ord[NEWS_MAX];
  int k = 0;
  for(int round = 0 ; k < total ; round++)
    for(int i = 0 ; i < ns ; i++)
    {
      const int idx = start[i] + round;
      const int lim = (i + 1 < ns) ? start[i + 1] : total;
      if(idx < lim && k < total) ord[k++] = (uint8_t)idx;
    }

  lock();
  for(int i = 0 ; i < total ; i++) items[i] = work[ord[i]];
  nItems = total;
  if(cur >= nItems) cur = 0;
  strlcpy(feedLabel, src[0].label, sizeof(feedLabel));
  lastOkMs = millis();
  status[0] = 0;
  lastSwitchMs = millis();
  version++;
  unlock();
}

// Web page test: one source, shows the first headline
static void doTest()
{
  if(!items || !work || !gSrc) { lock(); testState = 3; snprintf(testMsg, sizeof(testMsg), "%s", tr(T_NO_MEM)); unlock(); return; }
  char url[160];
  lock(); strlcpy(url, testUrl, sizeof(url)); unlock();
  NewsCfg c;
  lock(); c = cfg; unlock();

  Source one;
  if(url[0])
  {
    if(!strstr(url, "://")) { char t[160]; snprintf(t, sizeof(t), "https://%s", url); strlcpy(url, t, sizeof(url)); }
    strlcpy(one.url, url, sizeof(one.url));
    one.google = !strncasecmp(url, "https://news.google.", 20);
  }
  else
  {
    NewsCfg t = c; t.url[0] = 0;
    Source *s = gSrc;
    if(!buildSources(t, s)) { lock(); testState = 3; snprintf(testMsg, sizeof(testMsg), "%s", tr(T_NEWS_NONE)); unlock(); return; }
    one = s[0];
  }
  gLinkLong = 0;
  const int r = loadSource(one.url, one.google, work, NEWS_MAX, 0, true);
  int lOk = 0, lSum = 0, lMax = 0;
  for(int i = 0 ; i < r ; i++) { const int L = strlen(work[i].link); if(L) { lOk++; lSum += L; if(L > lMax) lMax = L; } }
  lock();
  testCount = r > 0 ? r : 0;
  if(r > 0)
  {
    testState = 2;
    snprintf(testMsg, sizeof(testMsg), tr(T_NEWS_FOUND), r);
    {
      const size_t l0 = strlen(testMsg);
      snprintf(testMsg + l0, sizeof(testMsg) - l0, tr(T_NEWS_LINKSTAT), lOk, lOk ? lSum / lOk : 0, lMax, gLinkLong, NEWS_LINK - 1, r - lOk - gLinkLong);
    }
    strlcpy(testHead, work[0].head, sizeof(testHead));
    strlcpy(testSum, work[0].sum[0] ? work[0].sum : work[0].src, sizeof(testSum));
  }
  else
  {
    testState = 3;
    errText(r, testMsg, sizeof(testMsg));
    testHead[0] = testSum[0] = 0;
  }
  unlock();
}

void newsRequestTest(const char *url)
{
  lock();
  strlcpy(testUrl, url ? url : "", sizeof(testUrl));
  testState = 1; testMsg[0] = testHead[0] = testSum[0] = 0; testCount = 0;
  unlock();
  testReq = true;
}

int newsTestResult(char *msg, size_t msgSize, char *head, size_t headSize, char *sum, size_t sumSize, int *count)
{
  lock();
  const int st = testState;
  strlcpy(msg, testMsg, msgSize); strlcpy(head, testHead, headSize); strlcpy(sum, testSum, sumSize);
  *count = testCount;
  unlock();
  return st;
}

// ---- Task and main loop ----------------------------------------------------

static bool newsWanted() { return currentCmd == CMD_NEWS; }

static void newsTask(void *)
{
  uint32_t last = 0;
  for(;;)
  {
    if(testReq) { testReq = false; doTest(); }
    if(newsWanted())
    {
      uint32_t period;
      lock(); period = (nItems ? (uint32_t)cfg.refreshMin * 60000UL : 60000UL); unlock();
      const bool due = wakeNow || !last || (uint32_t)(millis() - last) >= period;
      if(due)
      {
        wakeNow = false;
        fetchNews();
        last = millis() | 1;
      }
    }
    else last = 0;
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void newsBegin()
{
  if(!mtx) mtx = xSemaphoreCreateMutex();
  if(!mtx) return;
  loadCfg();
  // The statics of this module would take ~21 KB of internal memory; the TLS handshake needs it (error -32512)
  auto big = [](size_t n) -> void * { void *p = heap_caps_calloc(1, n, MALLOC_CAP_SPIRAM); return p ? p : calloc(1, n); };
  items = (NewsItem *)big(sizeof(NewsItem) * NEWS_MAX);
  work = (NewsItem *)big(sizeof(NewsItem) * NEWS_MAX);
  gSrc = (Source *)big(sizeof(Source) * NEWS_MAX_SRC);
  xTaskCreatePinnedToCore(newsTask, "news", NEWS_TASK_STACK, nullptr, 1, nullptr, 1);
}

void newsOpen()
{
  lock();
  const bool stale = !nItems || !lastOkMs || (uint32_t)(millis() - lastOkMs) > 120000UL;
  if(stale) { snprintf(status, sizeof(status), "%s", tr(T_NEWS_SEARCHING)); version++; }
  cur = 0; paused = false; qrOpen = false; lastSwitchMs = millis();
  unlock();
  if(stale) wakeNow = true;
}

bool newsIsActive() { return currentCmd == CMD_NEWS; }

void newsScroll(int16_t delta)
{
  lock();
  qrOpen = false;
  if(nItems > 1)
  {
    cur = (cur + (delta > 0 ? 1 : nItems - 1)) % nItems;
    lastSwitchMs = millis();
    version++;
  }
  unlock();
}

bool newsQrActive() { return qrOpen; }
void newsQrClose() { lock(); qrOpen = false; paused = false; lastSwitchMs = millis(); version++; unlock(); }
bool newsQrOpen()
{
  lock();
  const bool ok = nItems > 0 && items[cur].link[0];
  if(ok) { qrOpen = true; qrSince = millis(); version++; }
  unlock();
  return ok;
}

bool newsClick()
{
  lock();
  const bool ok = nItems > 1;
  if(ok) { paused = !paused; lastSwitchMs = millis(); version++; }
  unlock();
  return ok;
}

bool newsTick()
{
  lock();
  const bool apply = havePending;
  NewsCfg c = pendingCfg;
  havePending = false;
  unlock();
  if(apply)
  {
    Preferences p;
    if(p.begin("news", false))
    {
      p.putUChar("ed", c.edition); p.putUShort("top", c.topics);
      p.putString("q", c.query); p.putString("url", c.url);
      p.putUChar("ref", c.refreshMin); p.putUChar("sec", c.secs); p.putUChar("show", c.show);
      p.end();
    }
    lock();
    cfg = c;
    nItems = 0; cur = 0; lastOkMs = 0;           // Old items are for another setup
    snprintf(status, sizeof(status), "%s", tr(T_NEWS_SEARCHING));
    version++;
    unlock();
    wakeNow = true;
  }
  if(!newsIsActive()) { shownVersion = 0xFFFFFFFF; return false; }

  static uint32_t lastMs = 0;
  const uint32_t now = millis();
  bool redraw = false;
  lock();
  if(qrOpen && (uint32_t)(now - qrSince) > 120000UL) { qrOpen = false; version++; }
  if(!qrOpen && !paused && nItems > 1 && (uint32_t)(now - lastSwitchMs) >= (uint32_t)cfg.secs * 1000UL)
  {
    cur = (cur + 1) % nItems;
    lastSwitchMs = now;
    version++;
  }
  if(version != shownVersion) { shownVersion = version; redraw = true; }
  unlock();
  lock(); const bool empty = (nItems == 0); const bool qr = qrOpen; unlock();
  if(qr) return redraw;                             // The QR screen is static
  if((uint32_t)(now - lastMs) >= (empty ? 150U : 500U)) { lastMs = now; redraw = true; }   // Progress line, age text
  return redraw;
}

// ---- Drawing ---------------------------------------------------------------

// Word wrap: draws up to maxLines lines, returns the lines used; the last one ends with ".." if the text is cut
static int wrapText(LGFX_Sprite &g, const lgfx::GFXfont &f, const char *t, int x, int y, int w, int lh, int maxLines, uint16_t col)
{
  g.setFont(&f);
  char line[200];
  line[0] = 0;
  int ln = 0;
  const char *p = t;
  while(*p && ln < maxLines)
  {
    char word[64];
    int k = 0;
    while(p[k] && p[k] != ' ' && k < 63) { word[k] = p[k]; k++; }
    word[k] = 0;
    char tryl[270];
    snprintf(tryl, sizeof(tryl), "%s%s%s", line, line[0] ? " " : "", word);
    if(line[0] && g.textWidth(tryl) > w)
    {
      if(ln == maxLines - 1) { strlcat(line, "..", sizeof(line)); break; }
      ckText(g, f, line, x, y + ln * lh, col);
      ln++;
      line[0] = 0;
      continue;
    }
    strlcpy(line, tryl, sizeof(line));
    p += k;
    while(*p == ' ') p++;
  }
  if(line[0] && ln < maxLines) { ckText(g, f, line, x, y + ln * lh, col); ln++; }
  return ln;
}

static void ageText(char *out, size_t size, uint32_t ts)
{
  out[0] = 0;
  const time_t now = time(NULL);
  if(!ts || now < 1700000000 || (int64_t)ts > (int64_t)now + 3600) return;
  const long d = (long)(now - ts);
  char u[16];
  if(d < 90) { snprintf(out, size, "%s", tr(T_NOW)); return; }
  if(d < 3600) snprintf(u, sizeof(u), "%ld min", d / 60);
  else if(d < 86400) snprintf(u, sizeof(u), "%ld h", d / 3600);
  else snprintf(u, sizeof(u), "%ld d", d / 86400);
  snprintf(out, size, tr(T_NEWS_AGO), u);
}

// Background of the News screen: diagonal broadcast stripes and arcs, tinted with the user's colours
static void newsBack(LGFX_Sprite &g, const NuPal &p)
{
  nu::bgFill(g, p);
  const uint16_t st = ckMix(p.bg1, p.acc, 16);
  for(int k = -2 ; k < 9 ; k++)
  {
    const int x = k * 34;
    g.fillTriangle(x, 170, x + 14, 170, x + 74, 64, st);
    g.fillTriangle(x, 170, x + 74, 64, x + 60, 64, st);
  }
  for(int r = 40 ; r <= 160 ; r += 30) g.drawCircle(300, -10, r, ckMix(p.bg1, p.acc, 26));
}

// Moving band of "NEWS +++": dir > 0 runs to the right, dir < 0 to the left
static void newsBand(LGFX_Sprite &g, const NuPal &p, int y, bool solid, int dir)
{
  const uint16_t bg = solid ? p.acc : ckMix(p.bg1, 0x0000, 200), fg = solid ? p.bg1 : p.acc;
  g.fillRect(0, y, 320, 24, bg);
  g.fillRect(0, y + 22, 320, 2, solid ? ckMix(p.acc, 0x0000, 120) : p.acc);
  g.setClipRect(0, y, 320, 22);
  const int P = 250;
  int off = (int)((millis() / 8) % P);
  if(dir < 0) off = P - off;
  for(int x = -P + off ; x < 340 ; x += P) ckText(g, FontB612Bold12, "NEWS  +++  NEWS  +++  NEWS  +++", x + 4, y + 4, fg);
  g.clearClipRect();
}

void newsDraw(LGFX_Sprite &spr)
{
  NewsItem it;
  char st[48], label[24];
  int n, idx;
  bool pause;
  uint32_t since;
  uint8_t secs, show;
  lock();
  n = nItems; idx = cur; pause = paused;
  const bool qr = qrOpen;
  if(n) it = items[idx];
  strlcpy(st, status, sizeof(st));
  strlcpy(label, feedLabel, sizeof(label));
  since = (uint32_t)(millis() - lastSwitchMs);
  secs = cfg.secs; show = cfg.show;
  const uint8_t ed = cfg.edition;
  unlock();

  if(qr && n && it.link[0])
  {
    const NuPal qp = nuCurrentPalette();
    nu::bgFill(spr, qp);
    for(int r = 30; r <= 150; r += 24) spr.drawCircle(250, 40, r, ckMix(qp.bg1, qp.acc, 22));
    spr.fillRect(0, 0, 170, 170, 0xFFFF);             // White plate: the quiet zone the camera needs
    // Smallest version that holds the address (error correction low): one try, so the stack use stays small
    static const uint16_t CAP[] = {17,32,53,78,106,134,154,192,230,271,321,367,425,458,520,586,644,718,792,858,929,1003};
    uint8_t ver = 1;
    while(ver < 22 && strlen(it.link) > CAP[ver - 1]) ver++;
    // Whole-pixel modules, all the same size (what scanners expect), centred in the white square
    spr.qrcode(it.link, 5, 5, 160, ver);
    spr.fillRoundRect(184, 34, 126, 4, 2, qp.acc);
    ckText(spr, FontB612Bold16, tr(T_NEWS_SCAN1), 247, 48, 0xFFFF, 1);
    ckText(spr, FontB612Bold16, tr(T_NEWS_SCAN2), 247, 70, 0xFFFF, 1);
    spr.fillRoundRect(184, 100, 126, 4, 2, qp.accDim);
    ckText(spr, FontB612Reg10, tr(T_NEWS_WEB1), 247, 108, qp.txt2, 1);
    ckText(spr, FontB612Reg10, tr(T_NEWS_WEB2), 247, 121, qp.txt2, 1);
    ckText(spr, FontB612Reg12, tr(T_NEWS_BACK), 247, 142, qp.txt2, 1);
    return;
  }

  const NuPal np = nuCurrentPalette();
  newsBack(spr, np);
  const bool searching = !n && !(st[0] && strcmp(st, tr(T_NEWS_SEARCHING)));
  if(searching)
  {
    newsBand(spr, np, 0, true, +1);                   // Top band to the right, bottom band to the left
    newsBand(spr, np, 146, false, -1);
    spr.fillRoundRect(40, 54, 240, 60, 8, ckMix(np.bg1, 0x0000, 190));
    spr.drawRoundRect(40, 54, 240, 60, 8, np.cardLine);
    spr.fillRect(52, 59, 216, 2, np.acc);
    ckText(spr, FontB612Bold16, tr(T_NEWS_SEARCHING), 160, 66, 0xFFFF, 1);
    ckText(spr, FontB612Reg12, newsEditionLabel(ed < EDITION_COUNT ? ed : autoEdition()), 160, 86, np.txt2, 1);
    const int dots = 1 + (int)((millis() / 400) % 3);
    char dd[4];
    for(int i = 0 ; i < 3 ; i++) dd[i] = i < dots ? '.' : ' ';
    dd[3] = 0;
    ckText(spr, FontB612Bold16, dd, 160, 98, np.acc, 1);
    return;
  }

  if(!n)
  {
    spr.setFont(&FontB612Bold12);
    const char *title = tr(T_NEWS_TITLE);
    const int cw = spr.textWidth(title) + 16;
    spr.fillRoundRect(8, 5, cw, 16, 5, np.acc);
    ckText(spr, FontB612Bold12, title, 8 + cw / 2, 7, np.bg1, 1);
  }
  if(!n)
  {
    const bool fail = st[0] && strcmp(st, tr(T_NEWS_SEARCHING));
    if(!fail)
    {
      spr.setFont(&FontB612Bold16);
      const int w = spr.textWidth(tr(T_NEWS_SEARCHING)) + 28;
      spr.fillRoundRect(160 - w / 2, 118, w, 34, 8, ckMix(np.bg1, 0x0000, 170));
      spr.drawRoundRect(160 - w / 2, 118, w, 34, 8, np.cardLine);
      ckText(spr, FontB612Bold16, tr(T_NEWS_SEARCHING), 160, 122, np.txt, 1);
      char fl[24]; strlcpy(fl, label, sizeof(fl));
      if(fl[0]) ckText(spr, FontB612Reg10, fl, 160, 140, np.txt2, 1);
    }
    else ckText(spr, FontB612Bold16, tr(T_NEWS_NONE), 160, 52, np.txt, 1);
    if(fail)
    {
      wrapText(spr, FontB612Reg12, st, 24, 86, 272, 15, 3, np.txt2);
      char r[48];
      snprintf(r, sizeof(r), tr(T_NEWS_RETRY), 1);
      ckText(spr, FontB612Reg10, r, 160, 150, np.muted, 1);
    }
    return;
  }

  // TV look: top bar (live badge, outlet, clock), tag, headline band, footer
  const uint16_t band = ckMix(np.bg1, 0x0000, 205);
  spr.fillRect(0, 0, 320, 24, ckMix(np.bg1, 0x0000, 200));
  const char *liveTxt = "NEWS";                       // Same word in every language
  spr.setFont(&FontB612Bold12);
  const int lw = spr.textWidth(liveTxt) + 26;
  spr.fillRoundRect(8, 5, lw, 16, 4, 0xB800);
  spr.fillCircle(17, 13, 3, 0xFFFF);
  ckText(spr, FontB612Bold12, liveTxt, 25, 7, 0xFFFF);
  int clkW = 0;
  const char *tm = clockGet();                        // Local time (UTC offset setting applied)
  if(tm)
  {
    ckText(spr, FontB612Bold12, tm, 312, 7, np.txt, 2);
    clkW = 52;
  }
  if(show & 2)
  {
    spr.setClipRect(8 + lw + 8, 0, 304 - lw - 8 - clkW, 24);
    ckText(spr, FontB612Reg12, it.src[0] ? it.src : label, 8 + lw + 8, 7, np.txt2);
    spr.clearClipRect();
  }
  spr.fillRect(0, 64, 320, 86, band);
  spr.fillRect(0, 64, 320, 2, np.acc);
  spr.fillRect(0, 148, 320, 2, np.acc);
  const char *tag = label[0] ? label : tr(T_NEWS_TITLE);
  spr.setFont(&FontB612Bold12);
  int tw = spr.textWidth(tag) + 20; if(tw > 200) tw = 200;
  spr.fillRect(10, 56, tw, 16, np.acc);
  spr.setClipRect(10, 56, tw, 16);
  ckText(spr, FontB612Bold12, tag, 10 + tw / 2, 58, np.bg1, 1);
  spr.clearClipRect();

  const bool hasSum = (show & 1) && it.sum[0];
  wrapText(spr, FontB612Bold16, it.head, 12, 78, 296, 19, hasSum ? 2 : 3, 0xFFFF);
  if(hasSum) wrapText(spr, FontB612Reg12, it.sum, 12, 119, 296, 14, 2, np.txt2);

  // Footer: dots at the left, position, age and pause at the right, progress line at the bottom
  char age[24], foot[64];
  ageText(age, sizeof(age), it.ts);
  snprintf(foot, sizeof(foot), "%d/%d%s%s", idx + 1, n, ((show & 2) && age[0]) ? "  .  " : "", (show & 2) ? age : "");
  if(pause) { char pz[64]; snprintf(pz, sizeof(pz), "%s  .  %s", foot, tr(T_NEWS_PAUSED)); strlcpy(foot, pz, sizeof(foot)); }
  ckText(spr, FontB612Reg10, foot, 312, 153, np.txt2, 2);
  if(n > 1)
  {
    const int dots = n > 12 ? 12 : n;
    const int cdot = n > 12 ? (idx * 12 / n) : idx;
    for(int k = 0 ; k < dots ; k++) spr.fillRoundRect(10 + k * 9, 156, 6, 4, 2, k == cdot ? np.acc : np.cardLine);
    if(!pause && secs)
    {
      int w = (int)((uint32_t)312 * since / ((uint32_t)secs * 1000UL));
      if(w > 312) w = 312;
      spr.fillRect(4, 167, w, 2, np.accDim);
    }
  }
}

// For the web page: one line per item, with the address length and whether the QR code is possible
bool newsListItem(int i, char *head, size_t headSize, char *src, size_t srcSize, int *linkLen, bool *qrOk)
{
  bool ok = false;
  lock();
  if(i >= 0 && i < nItems)
  {
    strlcpy(head, items[i].head, headSize); strlcpy(src, items[i].src, srcSize);
    *linkLen = items[i].linkLen; *qrOk = items[i].link[0] != 0;
    ok = true;
  }
  unlock();
  return ok;
}

// For the web page: the article address of item i (false when none)
bool newsLinkItem(int i, char *out, size_t size)
{
  bool ok = false;
  lock();
  if(i >= 0 && i < nItems && items[i].link[0]) { strlcpy(out, items[i].link, size); ok = true; }
  unlock();
  return ok;
}
