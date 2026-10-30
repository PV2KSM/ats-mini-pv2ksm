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
static float usdLocal = 0, btcUsd = 0, btcLocal = 0;
static char rateDate[11] = "";
static bool rateDerived = false;
static char status[40] = "Buscando cotacoes...";
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
  if(deserializeJson(doc, body)) { snprintf(err, errSize, "Erro JSON"); return false; }
  const char *amount = doc["data"]["amount"] | (const char *)nullptr;
  if(!amount) { snprintf(err, errSize, "Sem preco %s", quote); return false; }
  const float v = (float)atof(amount);
  if(!(v > 0)) { snprintf(err, errSize, "Preco invalido"); return false; }
  *out = v;
  return true;
}

static void fetchFinance()
{
  char cur[4];
  financeGetCurrency(cur, sizeof(cur));
  const bool isUsd = !strcmp(cur, "USD");
  char err[40] = "";

  float bUsd = 0, bLoc = 0, rate = 0;
  char date[11] = "";
  bool derived = false;

  if(!fetchSpot("USD", &bUsd, err, sizeof(err))) { setStatus(err); return; }

  if(isUsd) { rate = 1.0f; bLoc = bUsd; }
  else
  {
    // Official daily rate
    char url[112], body[320], e2[40];
    snprintf(url, sizeof(url), "https://api.frankfurter.dev/v1/latest?base=USD&symbols=%s", cur);
    if(netFetchText(url, body, sizeof(body), e2, sizeof(e2), "application/json") == 200)
    {
      JsonDocument doc;
      if(!deserializeJson(doc, body))
      {
        rate = doc["rates"][cur] | 0.0f;
        strlcpy(date, doc["date"] | "", sizeof(date));
      }
    }
    if(!fetchSpot(cur, &bLoc, err, sizeof(err)))
    {
      // Currency not quoted against Bitcoin: derive it from the dollar rate
      if(rate > 0) bLoc = bUsd * rate;
      else { setStatus(err); return; }
    }
    if(!(rate > 0)) { rate = bLoc / bUsd; derived = true; }   // Rate from the Bitcoin prices
  }

  lock();
  usdLocal = rate; btcUsd = bUsd; btcLocal = bLoc;
  strlcpy(rateDate, date, sizeof(rateDate));
  rateDerived = derived;
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
  if(stale) { snprintf(status, sizeof(status), "Buscando cotacoes..."); version++; }
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
    snprintf(status, sizeof(status), "Buscando cotacoes...");
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

// pt-BR style: thousands "." and decimals ","
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
    if(rest > 0 && rest % 3 == 0 && n + 2 < size) out[n++] = '.';
  }
  if(dot && n + 2 < size)
  {
    out[n++] = ',';
    for(char *c = dot + 1 ; *c && n + 1 < size ; c++) out[n++] = *c;
  }
  out[n] = 0;
}

static inline uint16_t rgb(int r, int g, int b) { return lgfx::color565(r, g, b); }

void financeDraw(LGFX_Sprite &spr)
{
  lock();
  const bool ok = valid;
  const float usdL = usdLocal, bUsd = btcUsd, bLoc = btcLocal;
  const bool derived = rateDerived;
  char cur[4], st[40], date[11];
  strlcpy(cur, currency, sizeof(cur));
  strlcpy(st, status, sizeof(st));
  strlcpy(date, rateDate, sizeof(date));
  const uint32_t okMs = lastOkMs;
  unlock();

  const int W = 320, H = 170;
  for(int y = 0 ; y < H ; y++)
    spr.fillRect(0, y, W, 1, rgb(10 + y / 14, 30 + y / 7, 44 + y / 5));    // Deep teal gradient

  const uint16_t cW = 0xFFFF, cDim = rgb(170, 200, 210), cGold = rgb(255, 196, 70), cGreen = rgb(120, 230, 170);

  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(cW);
  spr.drawString("FINANCAS", 8, 5, FONT_SMALL);
  spr.setTextDatum(TR_DATUM);
  if(st[0]) { spr.setTextColor(cGold); spr.drawString(st, W - 8, 8, FONT_DEFAULT); }
  else
  {
    char age[24];
    const int mins = okMs ? (int)((millis() - okMs) / 60000UL) : 0;
    if(mins <= 0) snprintf(age, sizeof(age), "agora"); else snprintf(age, sizeof(age), "%d min", mins);
    spr.setTextColor(cDim);
    spr.drawString(age, W - 8, 8, FONT_DEFAULT);
  }

  if(!ok)
  {
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(cW);
    spr.drawString(st[0] ? st : "Buscando cotacoes...", W / 2, 90, FONT_SMALL);
    return;
  }

  const char *sym = symbolFor(cur);
  char num[32];

  // Card helper: the currency symbol uses the small font (the large font draws "$" as a pound sign)
  auto card = [&](int y, int h, const char *label, const char *symbol, const char *value, const char *sub, uint16_t col)
  {
    spr.fillRectAlpha(6, y, W - 12, h, 90, (uint32_t)0x000000);
    spr.fillRect(6, y, 3, h, col);
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(cDim);
    spr.drawString(label, 16, y + 6, FONT_DEFAULT);
    spr.setTextColor(cDim);
    spr.drawString(symbol, 16, y + 26, FONT_SMALL);
    const int sw = spr.textWidth(symbol, FONT_SMALL);
    spr.setTextColor(cW);
    spr.drawString(value, 16 + sw + 6, y + 19, FONT_LARGE);
    if(sub && sub[0]) { spr.setTextColor(cDim); spr.drawString(sub, 16, y + h - 14, FONT_DEFAULT); }
  };

  char sub[48];
  const bool isUsd = !strcmp(cur, "USD");

  // Dollar
  fmtNum(num, sizeof(num), usdL, usdL >= 100 ? 2 : 4);
  if(isUsd) snprintf(sub, sizeof(sub), "moeda local = dolar");
  else if(derived) snprintf(sub, sizeof(sub), "1 USD  (derivado do bitcoin)");
  else snprintf(sub, sizeof(sub), "1 USD  ref. BCE %s", date);
  card(22, 62, "DOLAR (USD)", sym, num, sub, cGreen);

  // Bitcoin: in the local currency, with the dollar price below
  char usdTxt[32], locTxt[32];
  fmtNum(usdTxt, sizeof(usdTxt), bUsd, bUsd >= 1000 ? 0 : 2);
  fmtNum(locTxt, sizeof(locTxt), bLoc, bLoc >= 1000 ? 0 : 2);
  if(!isUsd) { snprintf(sub, sizeof(sub), "US$ %s", usdTxt); card(90, 62, "BITCOIN (BTC)", sym, locTxt, sub, cGold); }
  else card(90, 62, "BITCOIN (BTC)", "US$", usdTxt, "", cGold);

  spr.setTextDatum(BL_DATUM);
  spr.setTextColor(rgb(120, 150, 160));
  spr.drawString("Coinbase / BCE (Frankfurter)", 8, H - 3, FONT_DEFAULT);
}
