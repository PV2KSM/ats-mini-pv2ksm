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
// Items that can be shown: 0 USD, 1 EUR, 2 GBP (fiat), 3 BTC, 4 ETH (crypto)
#define FIN_ITEMS 5
static const char *const ITEM_CODE[FIN_ITEMS] = {"USD", "EUR", "GBP", "BTC", "ETH"};
static uint8_t slots[3] = {0, 1, 3};
static uint8_t pendingSlots[3];
static bool havePendingSlots = false;
static float itemLocal[FIN_ITEMS], itemUsd[FIN_ITEMS];
static bool itemDer[FIN_ITEMS];
static char rateDate[11] = "";
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

void financeGetSlots(uint8_t *out)
{
  lock();
  memcpy(out, havePendingSlots ? pendingSlots : slots, 3);
  unlock();
}

const char *financeItemName(int item) { return item >= 0 && item < FIN_ITEMS ? ITEM_CODE[item] : ""; }
int financeItemCount() { return FIN_ITEMS; }

const char *financeRequestSlots(const uint8_t *s)
{
  for(int i = 0 ; i < 3 ; i++) if(s[i] >= FIN_ITEMS) return "Unknown finance item.";
  lock();
  memcpy(pendingSlots, s, 3);
  havePendingSlots = true;
  unlock();
  return nullptr;
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

static bool fetchSpot(const char *coin, const char *quote, float *out, char *err, size_t errSize)
{
  char url[96], body[256];
  snprintf(url, sizeof(url), "https://api.coinbase.com/v2/prices/%s-%s/spot", coin, quote);
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
  const bool isUsd = !strcmp(cur, "USD");
  uint8_t sl[3];
  financeGetSlots(sl);
  bool need[FIN_ITEMS] = {false};
  for(int i = 0 ; i < 3 ; i++) need[sl[i]] = true;
  char err[40] = "";

  // Official daily rates (ECB via Frankfurter): the local currency, euro and pound per US dollar
  float rate = isUsd ? 1.0f : 0, per[FIN_ITEMS] = {0};
  char date[11] = "";
  {
    char url[128], body[320], e2[40];
    char syms[16] = "";
    auto add = [&](const char *code) { if(strcmp(code, "USD") && !strstr(syms, code)) { if(syms[0]) strlcat(syms, ",", sizeof(syms)); strlcat(syms, code, sizeof(syms)); } };
    add(cur); add("EUR"); add("GBP");
    snprintf(url, sizeof(url), "https://api.frankfurter.dev/v1/latest?base=USD&symbols=%s", syms);
    if(netFetchText(url, body, sizeof(body), e2, sizeof(e2), "application/json") == 200)
    {
      JsonDocument doc;
      if(!deserializeJson(doc, body))
      {
        if(!isUsd) rate = doc["rates"][cur] | 0.0f;
        per[1] = doc["rates"]["EUR"] | 0.0f;
        per[2] = doc["rates"]["GBP"] | 0.0f;
        strlcpy(date, doc["date"] | "", sizeof(date));
      }
    }
  }

  // Bitcoin in dollars and in the local currency: needed for the Bitcoin card and as a fallback for the rates
  float bUsd = 0, bLoc = 0;
  bool haveBtc = false;
  auto btc = [&]() -> bool {
    if(haveBtc) return true;
    if(!fetchSpot("BTC", "USD", &bUsd, err, sizeof(err))) return false;
    if(isUsd) bLoc = bUsd;
    else if(!fetchSpot("BTC", cur, &bLoc, err, sizeof(err)))
    {
      if(rate > 0) bLoc = bUsd * rate; else return false;   // Not quoted against Bitcoin: derive it from the dollar rate
    }
    haveBtc = true;
    return true;
  };

  bool derivedRate = false;
  if(!(rate > 0))
  {
    if(!btc()) { setStatus(err); return; }
    rate = bLoc / bUsd;
    derivedRate = true;
  }

  float val[FIN_ITEMS] = {0}, usd[FIN_ITEMS] = {0};
  bool der[FIN_ITEMS] = {false};
  int got = 0;
  for(int i = 0 ; i < FIN_ITEMS ; i++)
  {
    if(!need[i]) continue;
    if(i <= 2)
    {
      if(!strcmp(ITEM_CODE[i], cur)) { val[i] = 1.0f; got++; }
      else if(i == 0) { val[i] = rate; der[i] = derivedRate; got++; }
      else if(per[i] > 0) { val[i] = rate / per[i]; got++; }
      else
      {
        float b = 0;                                         // Derive from Bitcoin prices
        char e3[40];
        if(btc() && fetchSpot("BTC", ITEM_CODE[i], &b, e3, sizeof(e3)) && b > 0) { val[i] = bLoc / b; der[i] = true; got++; }
      }
    }
    else if(i == 3) { if(btc()) { val[i] = bLoc; usd[i] = bUsd; got++; } }
    else
    {
      float eu = 0, el = 0;
      if(fetchSpot("ETH", "USD", &eu, err, sizeof(err)))
      {
        if(isUsd) el = eu;
        else { char e4[40]; if(!fetchSpot("ETH", cur, &el, e4, sizeof(e4))) el = eu * rate; }
        val[i] = el; usd[i] = eu; got++;
      }
    }
  }
  if(!got) { setStatus(err[0] ? err : tr(T_UNKNOWN_ERR)); return; }

  lock();
  for(int i = 0 ; i < FIN_ITEMS ; i++) { itemLocal[i] = val[i]; itemUsd[i] = usd[i]; itemDer[i] = der[i]; }
  strlcpy(rateDate, date, sizeof(rateDate));
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
    uint8_t sv[3];
    if(p.getBytes("slots", sv, 3) == 3 && sv[0] < FIN_ITEMS && sv[1] < FIN_ITEMS && sv[2] < FIN_ITEMS) memcpy(slots, sv, 3);
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
  const bool applySlots = havePendingSlots;
  uint8_t ns[3];
  memcpy(ns, pendingSlots, 3);
  havePendingSlots = false;
  unlock();
  if(applySlots)
  {
    Preferences p;
    if(p.begin("finance", false)) { p.putBytes("slots", ns, 3); p.end(); }
    lock();
    memcpy(slots, ns, 3);
    valid = false;
    lastOkMs = 0;
    snprintf(status, sizeof(status), "%s", tr(T_SEARCHING_RATES));
    version++;
    unlock();
    wakeNow = true;
  }
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
  float vL[FIN_ITEMS], vU[FIN_ITEMS];
  bool vD[FIN_ITEMS];
  memcpy(vL, itemLocal, sizeof(vL)); memcpy(vU, itemUsd, sizeof(vU)); memcpy(vD, itemDer, sizeof(vD));
  uint8_t sl[3];
  memcpy(sl, slots, 3);
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
  const bool isUsd = !strcmp(cur, "USD");
  const uint16_t cols[FIN_ITEMS] = {cGreen, cBlue, rgb(200, 150, 255), cGold, rgb(150, 160, 255)};
  for(int k = 0 ; k < 3 ; k++)
  {
    const int it = sl[k], y = 22 + 44 * k;
    const float v = vL[it];
    const char *label = it == 0 ? tr(T_FIN_USD) : it == 1 ? tr(T_FIN_EUR) : it == 2 ? tr(T_FIN_GBP) : it == 3 ? "BITCOIN (BTC)" : "ETHEREUM (ETH)";
    if(it <= 2)
    {
      if(v > 0) fmtNum(num, sizeof(num), v, v >= 100 ? 2 : 4); else strlcpy(num, "--", sizeof(num));
      if(!strcmp(ITEM_CODE[it], cur)) snprintf(note, sizeof(note), "%s", tr(T_FIN_LOCAL));
      else if(vD[it]) snprintf(note, sizeof(note), "%s", tr(T_FIN_DERIV_S));
      else snprintf(note, sizeof(note), tr(T_FIN_REFS), date);
      card(y, 42, label, note, sym, num, cols[it]);
    }
    else
    {
      char usdTxt[32];
      fmtNum(usdTxt, sizeof(usdTxt), vU[it], vU[it] >= 1000 ? 0 : 2);
      fmtNum(num, sizeof(num), v, v >= 1000 ? 0 : 2);
      if(!isUsd) { snprintf(note, sizeof(note), "US$ %s", usdTxt); card(y, 42, label, note, sym, num, cols[it]); }
      else card(y, 42, label, "", "US$", usdTxt, cols[it]);
    }
  }

  spr.setTextDatum(BL_DATUM);
  spr.setTextColor(uiModern ? np.muted : rgb(120, 150, 160));
  spr.drawString(tr(T_FIN_SOURCES), 8, H - 3, FONT_DEFAULT);
}
