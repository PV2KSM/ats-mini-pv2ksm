//
// Finance screen: US dollar and Bitcoin in the local currency chosen on the web page.
//   USD -> local: Frankfurter (ECB reference rates, no key). If that fails or does not know the
//                 currency, it is derived from the two Bitcoin prices.
//   Bitcoin:      Coinbase public spot prices (BTC-USD and BTC-<local>), no key.
//
#include "Finance.h"
#include "NetFetch.h"
#include "Utils.h"
#include "Menu.h"
#include "Themes.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include <string.h>
#include <math.h>
#include "I18n.h"
#include "NewUI.h"
NuPal nuCurrentPalette();

#define FIN_REFRESH_MS   (5UL * 60UL * 1000UL)   // While the screen stays open
#define FIN_STALE_MS     (2UL * 60UL * 1000UL)   // Refetch on open if older than this
#define FIN_TASK_STACK   12288

static const char *const CURRENCIES =
  "BRL USD EUR GBP JPY CAD AUD CHF CNY MXN ARS CLP COP INR KRW TRY ZAR SEK NOK DKK PLN SGD HKD NZD";

static SemaphoreHandle_t mtx = nullptr;
static char currency[4] = "BRL";
static char pendingCurrency[4] = "";
static bool havePending = false;

static bool valid = false;
static float usdLocal = 0, eurLocal = 0, btcUsd = 0, btcLocal = 0;
static char rateDate[11] = "";
static bool rateDerived = false, eurDerived = false;
static char status[40] = "";
static uint32_t lastOkMs = 0;
static uint32_t version = 0, shownVersion = 0xFFFFFFFF;
static volatile bool wakeNow = false;

static void lock()   { if(mtx) xSemaphoreTake(mtx, portMAX_DELAY); }
static void unlock() { if(mtx) xSemaphoreGive(mtx); }

static void setStatus(const char *s)
{
  lock();
  snprintf(status, sizeof(status), "%s", s ? s : "");
  version++;
  unlock();
}

const char *financeCurrencyList() { return CURRENCIES; }

void financeGetCurrency(char *out, size_t size)
{
  lock();
  strlcpy(out, currency, size);
  unlock();
}

const char *financeRequestCurrency(const char *code)
{
  if(!code || strlen(code) != 3) return "Currency must be a 3-letter code.";
  char up[4];
  for(int i = 0 ; i < 3 ; i++)
  {
    if(!isalpha((unsigned char)code[i])) return "Currency must be a 3-letter code.";
    up[i] = toupper((unsigned char)code[i]);
  }
  up[3] = 0;
  // Must be one of the supported codes
  bool ok = false;
  for(const char *p = CURRENCIES ; *p ; p += (p[3] ? 4 : 3))
    if(!strncmp(p, up, 3)) { ok = true; break; }
  if(!ok) return "Currency not in the supported list.";
  lock();
  strlcpy(pendingCurrency, up, sizeof(pendingCurrency));
  havePending = true;
  unlock();
  return nullptr;
}

// ---- Fetch -----------------------------------------------------------------

static bool fetchSpot(const char *quote, float *out, char *err, size_t errSize)
{
  char url[96], body[256];
  snprintf(url, sizeof(url), "https://api.coinbase.com/v2/prices/BTC-%s/spot", quote);
  if(netFetchText(url, body, sizeof(body), err, errSize, "application/json") != 200) return false;
  JsonDocument doc;
  if(deserializeJson(doc, body)) { snprintf(err, errSize, "%s", tr(T_JSON_ERR)); return false; }
  const char *amount = doc["data"]["amount"] | (const char *)nullptr;
  if(!amount) { snprintf(err, errSize, tr(T_FIN_NO_PRICE), quote); return false; }
  const float v = (float)atof(amount);
  if(!(v > 0)) { snprintf(err, errSize, "%s", tr(T_FIN_BAD_PRICE)); return false; }
  *out = v;
  return true;
}

static void fetchFinance()
{
  char cur[4];
  financeGetCurrency(cur, sizeof(cur));
  const bool isUsd = !strcmp(cur, "USD"), isEur = !strcmp(cur, "EUR");
  char err[40] = "";

  float bUsd = 0, bLoc = 0, rate = 0, eurPerUsd = 0;
  char date[11] = "";
  bool derived = false;

  if(!fetchSpot("USD", &bUsd, err, sizeof(err))) { setStatus(err); return; }

  // Official daily rates (ECB via Frankfurter): the local currency and the euro, per US dollar
  {
    char url[128], body[320], e2[40];
    if(isUsd || isEur) snprintf(url, sizeof(url), "https://api.frankfurter.dev/v1/latest?base=USD&symbols=EUR");
    else snprintf(url, sizeof(url), "https://api.frankfurter.dev/v1/latest?base=USD&symbols=%s,EUR", cur);
    if(netFetchText(url, body, sizeof(body), e2, sizeof(e2), "application/json") == 200)
    {
      JsonDocument doc;
      if(!deserializeJson(doc, body))
      {
        eurPerUsd = doc["rates"]["EUR"] | 0.0f;
        if(!isUsd) rate = doc["rates"][cur] | 0.0f;
        strlcpy(date, doc["date"] | "", sizeof(date));
      }
    }
  }

  if(isUsd) { rate = 1.0f; bLoc = bUsd; }
  else
  {
    if(!fetchSpot(cur, &bLoc, err, sizeof(err)))
    {
      // Currency not quoted against Bitcoin: derive it from the dollar rate
      if(rate > 0) bLoc = bUsd * rate;
      else { setStatus(err); return; }
    }
    if(!(rate > 0)) { rate = bLoc / bUsd; derived = true; }   // Rate from the Bitcoin prices
  }

  // One euro in the local currency
  float eurL = 0;
  bool eurDer = false;
  if(isEur) eurL = 1.0f;
  else if(eurPerUsd > 0) eurL = rate / eurPerUsd;
  else
  {
    float bEur = 0;
    char e3[40];
    if(fetchSpot("EUR", &bEur, e3, sizeof(e3)) && bEur > 0) { eurL = bLoc / bEur; eurDer = true; }
  }

  lock();
  usdLocal = rate; eurLocal = eurL; btcUsd = bUsd; btcLocal = bLoc;
  strlcpy(rateDate, date, sizeof(rateDate));
  rateDerived = derived;
  eurDerived = eurDer;
  valid = true;
  lastOkMs = millis();
  status[0] = 0;
  version++;
  unlock();
}

static bool financeWanted() { return currentCmd == CMD_FINANCE; }

static void financeTask(void *)
{
  uint32_t last = 0;
  for(;;)
  {
    if(financeWanted())
    {
      const uint32_t period = valid ? FIN_REFRESH_MS : 60000UL;
      bool due = wakeNow || !last || (uint32_t)(millis() - last) >= period;
      if(due)
      {
        wakeNow = false;
        fetchFinance();
        last = millis() | 1;
      }
    }
    else last = 0;
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void financeBegin()
{
  if(!mtx) mtx = xSemaphoreCreateMutex();
  if(!mtx) return;
  Preferences p;
  if(p.begin("finance", true))
  {
    String c = p.getString("cur", "BRL");
    if(c.length() == 3) strlcpy(currency, c.c_str(), sizeof(currency));
    p.end();
  }
  xTaskCreatePinnedToCore(financeTask, "finance", FIN_TASK_STACK, nullptr, 1, nullptr, 1);
}

void financeOpen()
{
  lock();
  const bool stale = !valid || !lastOkMs || (uint32_t)(millis() - lastOkMs) > FIN_STALE_MS;
  if(stale) { snprintf(status, sizeof(status), "%s", tr(T_SEARCHING_RATES)); version++; }
  unlock();
  if(stale) wakeNow = true;
}

bool financeIsActive() { return currentCmd == CMD_FINANCE; }

void financeInfo(char *st, size_t size, uint32_t *okMs)
{
  lock(); strlcpy(st, status, size); *okMs = lastOkMs; unlock();
}

// Main loop
bool financeTick()
{
  lock();
  const bool apply = havePending;
  char cur[4];
  strlcpy(cur, pendingCurrency, sizeof(cur));
  havePending = false;
  unlock();
  if(apply)
  {
    Preferences p;
    if(p.begin("finance", false)) { p.putString("cur", cur); p.end(); }
    lock();
    strlcpy(currency, cur, sizeof(currency));
    valid = false;                 // Old numbers are for another currency
    lastOkMs = 0;
    snprintf(status, sizeof(status), "%s", tr(T_SEARCHING_RATES));
    version++;
    unlock();
    wakeNow = true;
  }
  if(!financeIsActive()) { shownVersion = 0xFFFFFFFF; return false; }
  static uint32_t lastMs = 0;
  const uint32_t now = millis();
  bool redraw = false;
  if(version != shownVersion) { shownVersion = version; redraw = true; }
  if((uint32_t)(now - lastMs) >= 15000) { lastMs = now; redraw = true; }   // Age text
  return redraw;
}

// ---- Drawing ---------------------------------------------------------------

static const char *symbolFor(const char *cur)
{
  if(!strcmp(cur, "BRL")) return "R$";
  if(!strcmp(cur, "USD")) return "US$";
  if(!strcmp(cur, "EUR")) return "EUR";
  if(!strcmp(cur, "GBP")) return "GBP";
  return cur;
}

// pt/es style: thousands "." and decimals ","; English: the opposite
static void fmtNum(char *out, size_t size, float v, int decimals)
{
  char raw[32];
  snprintf(raw, sizeof(raw), "%.*f", decimals, (double)v);
  char *dot = strchr(raw, '.');
  const int intLen = dot ? (int)(dot - raw) : (int)strlen(raw);
  size_t n = 0;
  for(int i = 0 ; i < intLen && n + 2 < size ; i++)
  {
    out[n++] = raw[i];
    const int rest = intLen - 1 - i;
    if(rest > 0 && rest % 3 == 0 && n + 2 < size) out[n++] = (uiLang == LANG_EN) ? ',' : '.';
  }
  if(dot && n + 2 < size)
  {
    out[n++] = (uiLang == LANG_EN) ? '.' : ',';
    for(char *c = dot + 1 ; *c && n + 1 < size ; c++) out[n++] = *c;
  }
  out[n] = 0;
}

static inline uint16_t rgb(int r, int g, int b) { return lgfx::color565(r, g, b); }

void financeDraw(LGFX_Sprite &spr)
{
  lock();
  const bool ok = valid;
  const float usdL = usdLocal, eurL = eurLocal, bUsd = btcUsd, bLoc = btcLocal;
  const bool derived = rateDerived, eurDer = eurDerived;
  char cur[4], st[40], date[11];
  strlcpy(cur, currency, sizeof(cur));
  strlcpy(st, status, sizeof(st));
  strlcpy(date, rateDate, sizeof(date));
  const uint32_t okMs = lastOkMs;
  unlock();

  const int W = 320, H = 170;
  const NuPal np = nuCurrentPalette();
  if(uiModern) nu::bgFill(spr, np);
  else for(int y = 0 ; y < H ; y++)
    spr.fillRect(0, y, W, 1, rgb(10 + y / 14, 30 + y / 7, 44 + y / 5));    // Deep teal gradient

  const uint16_t cW = uiModern ? np.txt : 0xFFFF, cDim = uiModern ? np.txt2 : rgb(170, 200, 210), cGold = rgb(255, 196, 70), cGreen = rgb(120, 230, 170), cBlue = rgb(120, 175, 255);

  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(uiModern ? np.acc : cW);
  spr.drawString(tr(T_FIN_TITLE), 8, 5, FONT_SMALL);
  spr.setTextDatum(TR_DATUM);
  if(st[0]) { spr.setTextColor(cGold); spr.drawString(st, W - 8, 8, FONT_DEFAULT); }
  else
  {
    char age[24];
    const int mins = okMs ? (int)((millis() - okMs) / 60000UL) : 0;
    if(mins <= 0) snprintf(age, sizeof(age), "%s", tr(T_NOW)); else snprintf(age, sizeof(age), "%d min", mins);
    spr.setTextColor(cDim);
    spr.drawString(age, W - 8, 8, FONT_DEFAULT);
  }

  if(!ok)
  {
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(cW);
    spr.drawString(st[0] ? st : tr(T_SEARCHING_RATES), W / 2, 90, FONT_SMALL);
    return;
  }

  const char *sym = symbolFor(cur);
  char num[32];

  // Card helper: label and note on the top line; the currency symbol uses the small font
  // (the large font draws "$" as a pound sign)
  auto card = [&](int y, int h, const char *label, const char *note, const char *symbol, const char *value, uint16_t col)
  {
    if(uiModern) { nu::card(spr, 6, y, W - 12, h, 8, np); spr.fillRoundRect(10, y + 6, 3, h - 12, 1, col); }
    else { spr.fillRectAlpha(6, y, W - 12, h, 90, (uint32_t)0x000000); spr.fillRect(6, y, 3, h, col); }
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(cDim);
    spr.drawString(label, 16, y + 3, FONT_DEFAULT);
    if(note && note[0]) { spr.setTextDatum(TR_DATUM); spr.drawString(note, W - 12, y + 3, FONT_DEFAULT); spr.setTextDatum(TL_DATUM); }
    spr.drawString(symbol, 16, y + 17, FONT_SMALL);
    const int sw = spr.textWidth(symbol, FONT_SMALL);
    spr.setTextColor(cW);
    spr.drawString(value, 16 + sw + 6, y + 12, FONT_LARGE);
  };

  char note[40];
  const bool isUsd = !strcmp(cur, "USD"), isEur = !strcmp(cur, "EUR");

  // Dollar
  fmtNum(num, sizeof(num), usdL, usdL >= 100 ? 2 : 4);
  if(isUsd) snprintf(note, sizeof(note), "%s", tr(T_FIN_LOCAL));
  else if(derived) snprintf(note, sizeof(note), "%s", tr(T_FIN_DERIV_S));
  else snprintf(note, sizeof(note), tr(T_FIN_REFS), date);
  card(22, 42, tr(T_FIN_USD), note, sym, num, cGreen);

  // Euro
  if(eurL > 0) fmtNum(num, sizeof(num), eurL, eurL >= 100 ? 2 : 4); else strlcpy(num, "--", sizeof(num));
  if(isEur) snprintf(note, sizeof(note), "%s", tr(T_FIN_LOCAL));
  else if(eurDer) snprintf(note, sizeof(note), "%s", tr(T_FIN_DERIV_S));
  else snprintf(note, sizeof(note), tr(T_FIN_REFS), date);
  card(66, 42, tr(T_FIN_EUR), note, sym, num, cBlue);

  // Bitcoin: in the local currency, with the dollar price at the right of the label
  char usdTxt[32], locTxt[32];
  fmtNum(usdTxt, sizeof(usdTxt), bUsd, bUsd >= 1000 ? 0 : 2);
  fmtNum(locTxt, sizeof(locTxt), bLoc, bLoc >= 1000 ? 0 : 2);
  if(!isUsd) { snprintf(note, sizeof(note), "US$ %s", usdTxt); card(110, 42, "BITCOIN (BTC)", note, sym, locTxt, cGold); }
  else card(110, 42, "BITCOIN (BTC)", "", "US$", usdTxt, cGold);

  spr.setTextDatum(BL_DATUM);
  spr.setTextColor(uiModern ? np.muted : rgb(120, 150, 160));
  spr.drawString(tr(T_FIN_SOURCES), 8, H - 3, FONT_DEFAULT);
}
