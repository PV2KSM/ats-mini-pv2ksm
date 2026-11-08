#include "Dmr.h"
#include "Common.h"
#include "Menu.h"
#include "Themes.h"
#include "Utils.h"
#include "I18n.h"
#include "FontB612.h"
#include "DmrFonts.h"
#include "Flags.h"
#include <Preferences.h>
#include <string.h>
#include <stdio.h>

uint8_t uiDmrTheme = 0;

static DmrCfg cfg = {0, 1, 6, 0, "pi-star.local"};

static const char *const netNames[4] = {"BrandMeister", "TGIF", "DMR+", "FreeDMR"};
static const char *const themeNames[DMR_THEMES] = {"Classic", "Handheld", "Pi-Star", "Phosphor", "Minimal", "Flag Hero", "Badge", "Rows", "Paper"};
const char *dmrNetName(int i) { return netNames[(unsigned)i < 4 ? i : 0]; }
const char *dmrThemeName(int i) { return themeNames[(unsigned)i < DMR_THEMES ? i : 0]; }

//
// Data. DEMO traffic until a live source is connected.
//
struct Heard { char call[10]; char name[22]; char city[26]; uint32_t tg; uint16_t dur; uint8_t ago; uint32_t tms; bool live; uint32_t id; int8_t fl; };
#define DMR_MAX 8
static Heard demoList[DMR_MAX];
static Heard realList[DMR_MAX];
static int nReal = 0;
static bool real = false;                 // true once a live source delivered something
static Heard viewBuf[DMR_MAX];
static const Heard *vw = viewBuf;         // what the theme functions draw
static int nVw = 0;
static int sel = 0;
static uint32_t lastSec = 0;
static uint8_t statusKind = 0;            // 0 demo, 1 info, 2 live, 3 error
static char statusMsg[40] = "";
static char dbgMsg[48] = "";
static SemaphoreHandle_t mtx = nullptr;
static void lock() { if(mtx) xSemaphoreTake(mtx, portMAX_DELAY); }
static void unlock() { if(mtx) xSemaphoreGive(mtx); }

static void loadDemo()
{
  static const Heard d[DMR_MAX] = {
    {"PY2ABC", "Fulano de Tal", "Sao Jose dos Campos, BR", 724, 0, 0, 0, true}, {"PU2XYZ", "Ciclano", "Campinas, BR", 724, 31, 1, 0, false},
    {"PY1QRS", "Beltrano", "Rio de Janeiro, BR", 72451, 12, 2, 0, false}, {"PP5KLM", "Maria", "Florianopolis, BR", 724, 48, 3, 0, false},
    {"PY3DEF", "Joao", "Porto Alegre, BR", 7241, 19, 4, 0, false}, {"PU1GHI", "Ana", "Niteroi, BR", 724, 55, 6, 0, false},
    {"PT2JKL", "Carlos", "Brasilia, BR", 724, 23, 9, 0, false}, {"PY2MNO", "Paulo", "Santos, BR", 724, 41, 10, 0, false}};
  memcpy(demoList, d, sizeof(d));
  static const uint32_t ids[DMR_MAX] = {7241234, 7243390, 7245671, 7240826, 7247715, 7249042, 7242158, 7246303};
  for(int i = 0 ; i < DMR_MAX ; i++) { demoList[i].id = ids[i]; demoList[i].fl = flagFromMcc(ids[i]); }
  demoList[1].id = 3106011; demoList[1].fl = flagFromMcc(3106011); strlcpy(demoList[1].call, "W6ABC", sizeof(demoList[1].call));
  demoList[3].id = 2686120; demoList[3].fl = flagFromMcc(2686120); strlcpy(demoList[3].call, "CT1XYZ", sizeof(demoList[3].call));
  demoList[5].id = 4401234; demoList[5].fl = flagFromMcc(4401234); strlcpy(demoList[5].call, "JA1DEF", sizeof(demoList[5].call));
  demoList[6].id = 7221988; demoList[6].fl = flagFromMcc(7221988); strlcpy(demoList[6].call, "LU5GHI", sizeof(demoList[6].call));
}

static const char *tgName(uint32_t tg)
{
  switch(tg) { case 91: return "WORLD-WIDE"; case 724: return "BRASIL"; case 72451: return "SP-RJ"; case 7241: return "BR REGIONAL"; default: return ""; }
}

//
// Settings
//
void dmrBegin()
{
  Preferences p;
  if(p.begin("dmr", true))
  {
    cfg.net  = 0; cfg.src = 1;                       // hotspot only
    cfg.rows = p.getUChar("rows", cfg.rows); if(cfg.rows < 4 || cfg.rows > 8) cfg.rows = 6;
    cfg.tg   = p.getUInt("tg", cfg.tg);
    String h = p.getString("host", cfg.host); strlcpy(cfg.host, h.c_str(), sizeof(cfg.host));
    p.end();
  }
  loadDemo();
  if(!mtx) mtx = xSemaphoreCreateMutex();
}

void dmrGetCfg(DmrCfg *out) { *out = cfg; }

void dmrSetCfg(const DmrCfg &c)
{
  cfg = c; cfg.net = 0; cfg.src = 1; if(cfg.rows < 4 || cfg.rows > 8) cfg.rows = 6;
  cfg.host[sizeof(cfg.host) - 1] = 0;
  Preferences p;
  if(p.begin("dmr", false))
  {
    p.putUChar("net", cfg.net); p.putUChar("src", cfg.src); p.putUChar("rows", cfg.rows); p.putUInt("tg", cfg.tg); p.putString("host", cfg.host);
    p.end();
  }
}

void dmrOpen() { sel = 0; lastSec = 0; lock(); real = false; nReal = 0; statusKind = 1; snprintf(statusMsg, sizeof(statusMsg), "%s", uiText("CONNECTING")); unlock(); }
void dmrScroll(int16_t d) { const int n = real ? nReal : DMR_MAX; if(n > 0) { sel += d; if(sel < 0) sel = n - 1; if(sel >= n) sel = 0; } }
bool dmrTick()
{
  if(currentCmd != CMD_DMR) return false;
  const uint32_t s = millis() / 1000;
  if(s == lastSec) return false;
  lastSec = s; return true;
}

//
// Drawing helpers (320x170)
//
typedef LGFX_Sprite &G;
static inline uint16_t C(G g, int r, int gg, int b) { return g.color565(r, gg, b); }
// All small text uses anti-aliased Inter (VLW) instead of the 1-bit B612 bitmaps
static void txt(G g, const lgfx::GFXfont &f, const char *s, int x, int y, uint16_t c, int datum = 0)
{
  const uint8_t *v = &f == &FontB612Bold16 ? DmrB17 : &f == &FontB612Bold12 ? DmrB13 : &f == &FontB612Reg12 ? DmrR13 : DmrR11;
  g.loadFont(v); g.setTextDatum((lgfx::textdatum_t)datum); g.setTextColor(c); g.drawString(s, x, y - 2); g.unloadFont();
}
static void vlw(G g, const uint8_t *font, const char *s, int x, int y, uint16_t c, int datum = 0)
{ g.loadFont(font); g.setTextDatum((lgfx::textdatum_t)datum); g.setTextColor(c); g.drawString(s, x, y); g.unloadFont(); }

// Flag w px wide (height w*2/3) from the 36x24 table; tint: 0 colour, 1 = two-tone (ink over bg), dim: shifts brightness right by n bits
static void drawFlag(G g, int x, int y, int f, int w, int dim = 0, uint16_t ink = 0, uint16_t bg = 0)
{
  if(f < 0 || f >= FLAG_N || w < 4) return;
  const int h = w * FLAG_H / FLAG_W; const uint16_t *src = flagPix[f];
  for(int dy = 0 ; dy < h ; dy++)
  {
    for(int dx = 0 ; dx < w ; dx++)
    {
      int r, gr, bl;
      if(w < FLAG_W)    // shrinking: average every source pixel under the target pixel, so thin stripes and stars are kept
      {
        const int x0 = dx * FLAG_W / w, x1 = ((dx + 1) * FLAG_W + w - 1) / w, y0 = dy * FLAG_H / h, y1 = ((dy + 1) * FLAG_H + h - 1) / h;
        int sr = 0, sg = 0, sb = 0, n = 0;
        for(int yy = y0 ; yy < y1 && yy < FLAG_H ; yy++) for(int xx = x0 ; xx < x1 && xx < FLAG_W ; xx++)
        { const uint16_t p = src[yy * FLAG_W + xx]; sr += (p >> 11) & 31; sg += (p >> 5) & 63; sb += p & 31; n++; }
        if(!n) n = 1; r = sr / n; gr = sg / n; bl = sb / n;
      }
      else              // enlarging: bilinear
      {
        const int fy = ((2 * dy + 1) * FLAG_H * 128) / (2 * h) - 64; int y0 = fy >> 7; const int wy = fy & 127; int y1 = y0 + 1; if(y0 < 0) y0 = 0; if(y1 >= FLAG_H) y1 = FLAG_H - 1; if(y0 >= FLAG_H) y0 = FLAG_H - 1;
        const int fx = ((2 * dx + 1) * FLAG_W * 128) / (2 * w) - 64; int x0 = fx >> 7; const int wx = fx & 127; int x1 = x0 + 1; if(x0 < 0) x0 = 0; if(x1 >= FLAG_W) x1 = FLAG_W - 1; if(x0 >= FLAG_W) x0 = FLAG_W - 1;
        const uint16_t a = src[y0 * FLAG_W + x0], b = src[y0 * FLAG_W + x1], c = src[y1 * FLAG_W + x0], d = src[y1 * FLAG_W + x1];
        const int w00 = (128 - wx) * (128 - wy), w10 = wx * (128 - wy), w01 = (128 - wx) * wy, w11 = wx * wy;
        r  = (((a >> 11) & 31) * w00 + ((b >> 11) & 31) * w10 + ((c >> 11) & 31) * w01 + ((d >> 11) & 31) * w11) >> 14;
        gr = (((a >> 5) & 63) * w00 + ((b >> 5) & 63) * w10 + ((c >> 5) & 63) * w01 + ((d >> 5) & 63) * w11) >> 14;
        bl = ((a & 31) * w00 + (b & 31) * w10 + (c & 31) * w01 + (d & 31) * w11) >> 14;
      }
      uint16_t o;
      if(ink != bg)     // two-tone: luminance picks a mix of ink / bg (4 levels), for the LCD look
      {
        const int lum = (r * 8 * 30 + gr * 4 * 59 + bl * 8 * 11) / 100; const int k = lum < 80 ? 3 : lum < 140 ? 2 : lum < 200 ? 1 : 0;
        o = k == 3 ? ink : k == 0 ? bg : g.color565((((ink >> 11) & 31) * k + ((bg >> 11) & 31) * (3 - k)) * 8 / 3, ((((ink >> 5) & 63) * k + ((bg >> 5) & 63) * (3 - k)) * 4 / 3), ((ink & 31) * k + (bg & 31) * (3 - k)) * 8 / 3);
      }
      else { r >>= dim; gr >>= dim; bl >>= dim; o = (r << 11) | (gr << 5) | bl; }
      g.drawPixel(x + dx, y + dy, o);
    }
  }
}
// Full-width dimmed flag background (320 px wide, centred vertically in 170 px)
static void drawFlagBand(G g, int f, int off)
{
  const uint16_t *src = flagPix[f]; const int w = 320, hh = w * FLAG_H / FLAG_W;
  for(int dy = 0 ; dy < 170 ; dy++)
  {
    const int fy = ((2 * (dy + off) + 1) * FLAG_H * 128) / (2 * hh) - 64; int y0 = fy >> 7; const int wy = fy & 127; int y1 = y0 + 1; if(y0 < 0) y0 = 0; if(y1 >= FLAG_H) y1 = FLAG_H - 1; if(y0 >= FLAG_H) y0 = FLAG_H - 1;
    for(int dx = 0 ; dx < w ; dx++)
    {
      const int fx = ((2 * dx + 1) * FLAG_W * 128) / (2 * w) - 64; int x0 = fx >> 7; const int wx = fx & 127; int x1 = x0 + 1; if(x0 < 0) x0 = 0; if(x1 >= FLAG_W) x1 = FLAG_W - 1; if(x0 >= FLAG_W) x0 = FLAG_W - 1;
      const uint16_t a = src[y0 * FLAG_W + x0], b = src[y0 * FLAG_W + x1], c = src[y1 * FLAG_W + x0], d = src[y1 * FLAG_W + x1];
      const int w00 = (128 - wx) * (128 - wy), w10 = wx * (128 - wy), w01 = (128 - wx) * wy, w11 = wx * wy;
      int r  = (((a >> 11) & 31) * w00 + ((b >> 11) & 31) * w10 + ((c >> 11) & 31) * w01 + ((d >> 11) & 31) * w11) >> 14;
      int gr = (((a >> 5) & 63) * w00 + ((b >> 5) & 63) * w10 + ((c >> 5) & 63) * w01 + ((d >> 5) & 63) * w11) >> 14;
      int bl = ((a & 31) * w00 + (b & 31) * w10 + (c & 31) * w01 + (d & 31) * w11) >> 14;
      r = r * 5 / 12; gr = gr * 5 / 12; bl = bl * 5 / 12;
      g.drawPixel(dx, dy, (r << 11) | (gr << 5) | bl);
    }
  }
}
static void idText(char *out, size_t n, const Heard &h)
{
  if(h.id) snprintf(out, n, "ID %lu", (unsigned long)h.id); else out[0] = 0;
}
// Draws a VLW string, cutting it with ".." so that it fits maxw pixels
static void vlwFit(G g, const uint8_t *font, const char *str, int x, int y, uint16_t c, int datum, int maxw)
{
  char b[48]; strlcpy(b, str, sizeof(b)); g.loadFont(font);
  while(strlen(b) > 2 && g.textWidth(b) > maxw) { size_t n = strlen(b); n--; while(n > 1 && (b[n] & 0xC0) == 0x80) n--; b[n] = 0; if(n > 2) { b[n - 1] = '.'; b[n - 2] = '.'; } }
  g.setTextDatum((lgfx::textdatum_t)datum); g.setTextColor(c); g.drawString(b, x, y); g.unloadFont();
}
static bool demoMode() { return !real; }
static int agoMin(const Heard &h, int i) { return (real && h.tms) ? (int)((millis() - h.tms) / 60000UL) : (real ? i : h.ago); }
static void liveText(char *out, size_t n, const Heard &h, bool live)
{
  const int d = live ? (int)(real ? (millis() - h.tms) / 1000UL : (millis() / 1000) % 90) : h.dur;
  snprintf(out, n, "%d:%02d", d / 60, d % 60);
}
static const char *netShown() { return "Hotspot"; }       // internet feeds are not offered for now
static void tgLine(char *out, size_t n, uint32_t tg)
{
  const char *nm = tgName(tg);
  if(nm[0]) snprintf(out, n, "TG %lu  %s", (unsigned long)tg, nm); else snprintf(out, n, "TG %lu", (unsigned long)tg);
}
static void clockText(char *out, size_t n)
{
  uint8_t hh, mm;
  if(clockGetLocalHM(&hh, &mm)) snprintf(out, n, "%02d:%02d", hh, mm); else snprintf(out, n, "--:--");
}
static const char *statusShown() { return demoMode() ? "DEMO" : statusMsg; }   // demo traffic is only used by the Visual preview
static void demoTag(G g, uint16_t c) { txt(g, FontB612Reg10, statusShown(), 160, 160, c, 4); }

// 1. Classic (amber)
static void thClassic(G g, int s)
{
  const uint16_t BG = C(g, 3, 9, 15), AMB = C(g, 255, 176, 0), WH = C(g, 240, 244, 248), GR = C(g, 140, 152, 166), GRN = C(g, 60, 220, 120), RED = C(g, 235, 50, 50), BL = C(g, 70, 150, 255), P = C(g, 14, 24, 34);
  g.fillSprite(BG); const Heard &h = vw[s]; char b[40];
  g.fillRoundRect(8, 5, 67, 22, 5, P); g.fillCircle(20, 16, 4, GRN); txt(g, FontB612Bold12, "DMR", 30, 7, WH);
  g.fillRoundRect(140, 6, 116, 17, 4, BL); txt(g, FontB612Bold12, netShown(), 198, 8, WH, 1);
  clockText(b, sizeof(b)); txt(g, FontB612Bold12, b, 312, 6, WH, 2);
  g.fillRect(0, 26, 320, 2, AMB);
  const bool live = vw[s].live;
  if(live) { g.fillRoundRect(8, 33, 86, 16, 4, RED); g.fillCircle(19, 41, 3, WH); txt(g, FontB612Bold12, uiText("TALKING"), 27, 34, WH); }
  txt(g, FontB612Reg10, "TS2 CC1", 312, 38, GR, 2);
  vlw(g, DmrCallBig, h.call, 8, 48, WH, 0);
  drawFlag(g, 258, 56, h.fl, 54);
  vlw(g, DmrName, h.name, 9, 98, AMB, 0);
  idText(b, sizeof(b), h); if(b[0]) txt(g, FontB612Bold12, b, 10, 122, WH);
  vlwFit(g, DmrName, h.city, 312, 118, GR, 2, 190);
  g.fillRoundRect(8, 144, 150, 18, 4, P); tgLine(b, sizeof(b), h.tg); txt(g, FontB612Reg12, b, 14, 147, AMB);
  liveText(b, sizeof(b), h, live); txt(g, FontB612Bold16, b, 312, 143, live ? GRN : GR, 2);
  txt(g, FontB612Reg10, statusShown(), 312, 160, GR, 2);
}

// 2. Handheld LCD
static void thHandheld(G g, int s)
{
  const uint16_t BG = C(g, 176, 206, 196), INK = C(g, 18, 34, 30), MID = C(g, 70, 100, 92);
  g.fillSprite(BG); const Heard &h = vw[s]; char b[40];
  g.fillRect(0, 0, 320, 17, INK);
  txt(g, FontB612Bold12, "DMR  TS2 CC1", 7, 2, BG);
  for(int i = 0; i < 5; i++) g.fillRect(165 + i * 7, 13 - i * 2, 5, 2 + i * 2, BG);
  clockText(b, sizeof(b)); txt(g, FontB612Bold12, b, 312, 2, BG, 2);
  tgLine(b, sizeof(b), h.tg); txt(g, FontB612Bold16, b, 160, 22, INK, 1);
  drawFlag(g, 10, 21, h.fl, 30, 0, INK, BG);
  g.fillRect(10, 41, 300, 1, MID);
  vlw(g, DmrCallBig, h.call, 160, 42, INK, 1);
  vlw(g, DmrName, h.name, 160, 92, INK, 1);
  idText(b, sizeof(b), h); strlcat(b, "  ", sizeof(b)); strlcat(b, h.city, sizeof(b));
  vlwFit(g, DmrName, b, 160, 113, MID, 1, 300);
  g.fillRoundRect(8, 141, 304, 25, 4, INK);
  txt(g, FontB612Bold16, h.live ? "RX" : "LH", 18, 146, BG); txt(g, FontB612Reg12, netShown(), 55, 148, BG);
  liveText(b, sizeof(b), h, h.live); txt(g, FontB612Bold16, b, 302, 146, BG, 2);
}

// 3. Pi-Star like table
static void thTable(G g, int s)
{
  const uint16_t BG = C(g, 14, 20, 30), HD = C(g, 38, 52, 78), ST = C(g, 20, 29, 44), WH = C(g, 225, 232, 240), GR = C(g, 130, 146, 166), GRN = C(g, 70, 210, 110), YL = C(g, 240, 200, 60);
  g.fillSprite(BG); char b[40];
  txt(g, FontB612Bold12, "Last Heard", 8, 4, WH); txt(g, FontB612Reg10, netShown(), 100, 6, GR);
  clockText(b, sizeof(b)); txt(g, FontB612Reg12, b, 312, 4, GR, 2);
  g.fillRect(0, 20, 320, 12, HD);
  txt(g, FontB612Bold12, "Time", 6, 20, WH); txt(g, FontB612Bold12, "Callsign", 52, 20, WH); txt(g, FontB612Bold12, "Name", 140, 20, WH);
  txt(g, FontB612Bold12, "ID", 212, 20, WH); txt(g, FontB612Bold12, "Dur", 312, 20, WH, 2);
  const int rows = cfg.rows > 8 ? 8 : cfg.rows, rh = 17;
  for(int i = 0; i < rows && i < nVw; i++)
  {
    const int y = 35 + i * rh; const Heard &h = vw[i];
    if(i == s) g.fillRect(0, y - 1, 320, rh, C(g, 52, 62, 30)); else if(i & 1) g.fillRect(0, y - 1, 320, rh, ST);
    { const int am = agoMin(h, i); if(h.live || am <= 0) snprintf(b, sizeof(b), "now"); else snprintf(b, sizeof(b), "-%dm", am); } txt(g, FontB612Reg12, b, 6, y, GR);
    drawFlag(g, 40, y + 1, h.fl, 18); txt(g, FontB612Bold12, h.call, 62, y, h.live ? YL : WH); g.setClipRect(140, y, 64, rh); txt(g, FontB612Reg12, h.name, 140, y, WH); g.clearClipRect();
    if(h.id) snprintf(b, sizeof(b), "%lu", (unsigned long)h.id); else snprintf(b, sizeof(b), "-"); txt(g, FontB612Reg12, b, 212, y, GR);
    liveText(b, sizeof(b), h, h.live); txt(g, FontB612Reg12, b, 312, y, h.live ? GRN : WH, 2);
  }
  demoTag(g, GR);
}

// 4. Phosphor
static void thPhosphor(G g, int s)
{
  const uint16_t BG = C(g, 2, 10, 4), GN = C(g, 60, 255, 110), D = C(g, 24, 120, 50), M = C(g, 40, 190, 80);
  g.fillSprite(BG); const Heard &h = vw[s]; char b[48], up[26];
  snprintf(b, sizeof(b), "> DMR/%s  TG%lu", netShown(), (unsigned long)h.tg); txt(g, FontB612Bold12, b, 8, 5, GN);
  clockText(b, sizeof(b)); txt(g, FontB612Bold12, b, 312, 5, GN, 2);
  g.fillRect(0, 22, 320, 1, D);
  txt(g, FontB612Reg12, vw[s].live ? "RX  TS2 CC1" : "LAST HEARD", 8, 28, M);
  vlw(g, DmrCallBig, h.call, 8, 44, GN, 0);
  for(size_t i = 0; i < sizeof(up) - 1 && h.name[i]; i++) up[i] = (h.name[i] >= 'a' && h.name[i] <= 'z') ? h.name[i] - 32 : h.name[i];
  up[strnlen(h.name, sizeof(up) - 1)] = 0;
  vlw(g, DmrName, up, 9, 100, M, 0);
  { char t[40]; idText(t, sizeof(t), h); if(h.fl >= 0) { strlcat(t, "  [", sizeof(t)); strlcat(t, flagCode[h.fl], sizeof(t)); strlcat(t, "]", sizeof(t)); } txt(g, FontB612Bold12, t, 9, 124, GN); }
  g.fillRect(0, 143, 320, 1, D);
  char l[48]; snprintf(l, sizeof(l), "%s  %s  %s", vw[(s + 1) % nVw].call, vw[(s + 2) % nVw].call, vw[(s + 3) % nVw].call);
  txt(g, FontB612Reg12, l, 8, 150, D);
  liveText(b, sizeof(b), h, h.live); txt(g, FontB612Bold16, b, 312, 148, GN, 2);
}

// 5. Minimal
static void thMinimal(G g, int s)
{
  const uint16_t BG = C(g, 0, 0, 0), WH = C(g, 245, 245, 245), GR = C(g, 120, 126, 134), AC = C(g, 255, 176, 0);
  g.fillSprite(BG); const Heard &h = vw[s]; char b[48];
  tgLine(b, sizeof(b), h.tg); txt(g, FontB612Reg12, b, 160, 12, GR, 1);
  drawFlag(g, 10, 6, h.fl, 30);
  vlw(g, DmrCallBig, h.call, 160, 40, WH, 1);
  g.fillRect(125, 103, 70, 2, AC);
  vlwFit(g, DmrName, h.name, 160, 110, WH, 1, 300);
  char t[16]; liveText(t, sizeof(t), h, h.live); idText(b, sizeof(b), h); if(b[0]) txt(g, FontB612Bold12, b, 160, 134, GR, 1);
  snprintf(b, sizeof(b), "%s  %s", netShown(), t);
  txt(g, FontB612Reg12, b, 160, 153, GR, 1);
}

// 6. Flag Hero: the country flag fills the screen, dimmed, with the station on top
static void shadow(G g, const uint8_t *font, const char *s, int x, int y, uint16_t c, int datum)
{ vlw(g, font, s, x + 2, y + 2, 0, datum); vlw(g, font, s, x, y, c, datum); }
static void thHero(G g, int s)
{
  const uint16_t WH = C(g, 255, 255, 255), GR = C(g, 205, 212, 222), GRN = C(g, 90, 235, 140), RED = C(g, 240, 60, 60), BG = C(g, 8, 14, 22);
  const Heard &h = vw[s]; char b[48];
  g.fillSprite(BG);
  if(h.fl >= 0) { const int w = 320, hh = w * FLAG_H / FLAG_W; const int off = (hh - 170) / 2; drawFlagBand(g, h.fl, off); }
  g.fillRect(0, 0, 320, 26, C(g, 6, 10, 16)); g.fillRect(0, 144, 320, 26, C(g, 6, 10, 16));
  tgLine(b, sizeof(b), h.tg); txt(g, FontB612Bold12, b, 8, 6, WH);
  clockText(b, sizeof(b)); txt(g, FontB612Bold12, b, 312, 6, GR, 2);
  if(h.live) { g.fillRoundRect(190, 5, 78, 17, 4, RED); txt(g, FontB612Bold12, uiText("TALKING"), 229, 7, WH, 1); }
  shadow(g, DmrCallBig, h.call, 160, 34, WH, 1);
  shadow(g, DmrName, h.name, 160, 90, WH, 1);
  idText(b, sizeof(b), h); if(b[0]) { g.fillRoundRect(110, 116, 100, 20, 5, C(g, 6, 10, 16)); txt(g, FontB612Bold12, b, 160, 119, WH, 1); }
  txt(g, FontB612Reg12, netShown(), 8, 150, GR);
  liveText(b, sizeof(b), h, h.live); txt(g, FontB612Bold16, b, 312, 148, h.live ? GRN : WH, 2);
}

// 7. Badge: ID card with a big flag and the last callsigns underneath
static void thBadge(G g, int s)
{
  const uint16_t BG = C(g, 10, 16, 28), CARD = C(g, 22, 32, 52), WH = C(g, 240, 244, 250), GR = C(g, 140, 152, 170), AC = C(g, 90, 190, 255), GRN = C(g, 80, 220, 130), RED = C(g, 235, 60, 60);
  const Heard &h = vw[s]; char b[48];
  g.fillSprite(BG);
  g.fillRect(0, 0, 320, 24, CARD); txt(g, FontB612Bold12, "DMR", 8, 5, AC); txt(g, FontB612Reg12, netShown(), 52, 6, GR);
  clockText(b, sizeof(b)); txt(g, FontB612Bold12, b, 312, 5, WH, 2);
  g.fillRoundRect(6, 30, 308, 104, 8, CARD);
  g.fillRoundRect(14, 38, 100, 70, 4, WH); drawFlag(g, 16, 40, h.fl, 96);
  if(h.fl < 0) txt(g, FontB612Bold16, "DMR", 64, 64, BG, 1);
  vlw(g, DmrCallMid, h.call, 126, 36, WH, 0);
  vlwFit(g, DmrName, h.name, 127, 74, AC, 0, 180);
  idText(b, sizeof(b), h); if(b[0]) txt(g, FontB612Bold12, b, 128, 98, WH);
  tgLine(b, sizeof(b), h.tg); txt(g, FontB612Reg12, b, 128, 116, GR);
  liveText(b, sizeof(b), h, h.live); txt(g, FontB612Bold16, b, 304, 112, h.live ? GRN : GR, 2);
  if(h.live) { g.fillRoundRect(190, 4, 78, 17, 4, RED); txt(g, FontB612Bold12, uiText("TALKING"), 229, 6, WH, 1); }
  int x = 8; for(int k = 1 ; k < nVw && k <= 4 ; k++) { const Heard &o = vw[(s + k) % nVw]; drawFlag(g, x, 142, o.fl, 24); txt(g, FontB612Reg10, o.call, x + 27, 142, GR); x += 78; }
}

// 8. Rows: three big rows, each with its flag
static void thRows(G g, int s)
{
  const uint16_t BG = C(g, 8, 12, 20), P = C(g, 20, 28, 44), SELC = C(g, 34, 52, 84), WH = C(g, 240, 244, 250), GR = C(g, 140, 152, 170), AC = C(g, 255, 176, 0), GRN = C(g, 80, 220, 130);
  g.fillSprite(BG); char b[48];
  g.fillRect(0, 0, 320, 24, P); txt(g, FontB612Bold12, "DMR", 8, 5, AC); { char t[40]; snprintf(t, sizeof(t), "%s  TG %lu", netShown(), (unsigned long)vw[s].tg); txt(g, FontB612Reg12, t, 52, 6, GR); }
  clockText(b, sizeof(b)); txt(g, FontB612Bold12, b, 312, 5, WH, 2);
  int first = s - 1; if(first < 0) first = 0; if(first > nVw - 3) first = nVw - 3; if(first < 0) first = 0;
  for(int i = 0 ; i < 3 && first + i < nVw ; i++)
  {
    const int idx = first + i; const Heard &h = vw[idx]; const int y = 28 + i * 47;
    g.fillRoundRect(4, y, 312, 44, 6, idx == s ? SELC : P);
    g.fillRoundRect(9, y + 6, 46, 32, 3, WH); drawFlag(g, 10, y + 7, h.fl, 44);
    txt(g, FontB612Bold16, h.call, 64, y + 4, h.live ? GRN : WH);
    liveText(b, sizeof(b), h, h.live); txt(g, FontB612Bold16, b, 308, y + 4, h.live ? GRN : GR, 2);
    vlwFit(g, DmrName, h.name, 64, y + 22, AC, 0, 150);
    idText(b, sizeof(b), h); if(b[0]) txt(g, FontB612Reg12, b, 308, y + 26, GR, 2);
  }
  }

// 9. Paper: light theme for sunlight
static void thPaper(G g, int s)
{
  const uint16_t BG = C(g, 244, 238, 222), INK = C(g, 24, 24, 28), MID = C(g, 100, 96, 88), RD = C(g, 190, 40, 40), LINE = C(g, 200, 190, 165), GRN = C(g, 30, 120, 60);
  g.fillSprite(BG); const Heard &h = vw[s]; char b[48];
  g.fillRect(0, 0, 320, 24, INK); txt(g, FontB612Bold12, "DMR", 8, 5, BG); txt(g, FontB612Reg12, netShown(), 52, 6, C(g, 200, 196, 186));
  clockText(b, sizeof(b)); txt(g, FontB612Bold12, b, 312, 5, BG, 2);
  g.fillRect(0, 24, 320, 3, RD);
  tgLine(b, sizeof(b), h.tg); txt(g, FontB612Bold12, b, 10, 34, MID);
  if(h.live) { g.fillRoundRect(236, 32, 76, 16, 4, RD); txt(g, FontB612Bold12, uiText("TALKING"), 274, 33, BG, 1); }
  vlw(g, DmrCallBig, h.call, 8, 50, INK, 0);
  g.drawRect(255, 86, 58, 38, INK); drawFlag(g, 257, 88, h.fl, 54);
  vlw(g, DmrName, h.name, 9, 100, RD, 0);
  g.drawFastHLine(8, 127, 304, LINE);
  idText(b, sizeof(b), h); if(b[0]) txt(g, FontB612Bold12, b, 10, 131, INK);
  vlwFit(g, DmrName, h.city, 312, 128, MID, 2, 190);
  liveText(b, sizeof(b), h, h.live); txt(g, FontB612Bold16, b, 312, 150, h.live ? GRN : MID, 2);
  txt(g, FontB612Reg10, statusShown(), 10, 156, MID);
}

static void thEmpty(G g)
{
  const bool errState = (statusKind == 3);
  g.fillSprite(C(g, 3, 9, 15)); char b[48];
  txt(g, FontB612Bold12, "DMR", 10, 8, C(g, 240, 244, 248)); txt(g, FontB612Reg12, netShown(), 310, 8, C(g, 140, 152, 166), 2);
  g.fillRect(0, 26, 320, 2, C(g, 255, 176, 0));
  snprintf(b, sizeof(b), "%s", cfg.host[0] ? cfg.host : netShown());
  txt(g, FontB612Bold16, b, 160, 70, C(g, 255, 176, 0), 1);
  txt(g, FontB612Reg12, uiText("Waiting for traffic"), 160, 100, C(g, 240, 244, 248), 1);
  if(dbgMsg[0]) txt(g, FontB612Reg10, dbgMsg, 160, 150, C(g, 140, 152, 166), 1);
  txt(g, FontB612Reg12, statusMsg, 160, 130, errState ? C(g, 235, 80, 80) : C(g, 140, 152, 166), 1);
}

static void drawTheme(G g, int theme, int s)
{
  if(s < 0 || s >= nVw) s = 0;
  if(nVw <= 0) { thEmpty(g); return; }
  switch(theme) { case 1: thHandheld(g, s); break; case 2: thTable(g, s); break; case 3: thPhosphor(g, s); break; case 4: thMinimal(g, s); break; case 5: thHero(g, s); break; case 6: thBadge(g, s); break; case 7: thRows(g, s); break; case 8: thPaper(g, s); break; default: thClassic(g, s); }
}

void dmrDraw(LGFX_Sprite &spr)
{
  lock();
  memcpy(viewBuf, realList, sizeof(viewBuf)); nVw = real ? nReal : 0;     // no live data yet: the empty screen shows the status
  unlock();
  drawTheme(spr, uiDmrTheme, sel);
  spr.pushSprite(0, 0);
}
void dmrDrawPreview(LGFX_Sprite &spr, int theme)
{
  const bool r = real; real = false;          // the preview always shows demonstration traffic
  memcpy(viewBuf, demoList, sizeof(viewBuf)); nVw = DMR_MAX;
  drawTheme(spr, theme, 0);
  real = r;
}

//
// Worker interface (DmrNet.cpp)
//
bool dmrWanted() { return currentCmd == CMD_DMR; }
void dmrSetStatus(uint8_t kind, const char *msg)
{
  lock(); statusKind = kind; snprintf(statusMsg, sizeof(statusMsg), "%s", msg ? msg : ""); unlock();
}
// Callsign cache filled by the RadioID lookup (the hotspot page only gives callsigns)
struct DmrInfo { char call[10]; char name[22]; char city[26]; uint32_t id; };
#define DMR_CACHE 24
static DmrInfo infoCache[DMR_CACHE]; static int infoN = 0, infoPos = 0;
static DmrInfo *infoFind(const char *call)
{ for(int i = 0 ; i < infoN ; i++) if(!strcmp(infoCache[i].call, call)) return &infoCache[i]; return nullptr; }
static void applyInfo(Heard &h)
{
  const DmrInfo *e = infoFind(h.call); if(!e) return;
  if(!h.name[0]) strlcpy(h.name, e->name, sizeof(h.name));
  if(!h.id && e->id) { h.id = e->id; const int f = flagFromMcc(e->id); if(f >= 0) h.fl = (int8_t)f; }
  if((!h.city[0] || !strcmp(h.city, "Hotspot")) && e->city[0]) strlcpy(h.city, e->city, sizeof(h.city));
}
void dmrPush(const char *call, const char *name, const char *place, uint32_t tg, int dur, bool live, int agoMinutes, bool dedupe, uint32_t id)
{
  lock();
  if(!real) { real = true; nReal = 0; memset(realList, 0, sizeof(realList)); }
  int idx = -1;
  for(int i = 0 ; i < nReal ; i++)
  {
    if(strcmp(realList[i].call, call) || realList[i].tg != tg) continue;
    if(realList[i].live && !dedupe) { idx = i; break; }
    if(dedupe && abs((int)realList[i].dur - dur) <= 1) { unlock(); return; }
  }
  if(idx < 0)
  {
    if(nReal < DMR_MAX) nReal++;
    for(int i = nReal - 1 ; i > 0 ; i--) realList[i] = realList[i - 1];
    idx = 0;
    memset(&realList[0], 0, sizeof(Heard));
    strlcpy(realList[0].call, call, sizeof(realList[0].call));
    realList[0].tg = tg;
    realList[0].id = id; realList[0].fl = (int8_t)(id ? flagFromMcc(id) : -1); if(realList[0].fl < 0) realList[0].fl = (int8_t)flagFromCall(call);
    realList[0].tms = live ? millis() : millis() - (uint32_t)agoMinutes * 60000UL;
    applyInfo(realList[0]);
  }
  Heard &h = realList[idx];
  if(id && !h.id) { h.id = id; const int f = flagFromMcc(id); if(f >= 0) h.fl = (int8_t)f; }
  if(name && name[0]) strlcpy(h.name, name, sizeof(h.name));
  if(place && place[0]) strlcpy(h.city, place, sizeof(h.city));
  h.dur = dur < 0 ? 0 : (dur > 3599 ? 3599 : dur);
  if(!live) h.live = false; else h.live = true;
  if(!dedupe && !live && idx == 0) { /* keep tms of the start */ }
  unlock();
}
bool dmrNextLookup(char *call, size_t n)      // First listed callsign nobody asked about yet; marks it as asked
{
  bool found = false;
  lock();
  for(int i = 0 ; i < nReal && !found ; i++)
  {
    if(!realList[i].call[0] || infoFind(realList[i].call)) continue;
    DmrInfo *e = &infoCache[infoPos]; infoPos = (infoPos + 1) % DMR_CACHE; if(infoN < DMR_CACHE) infoN++;
    memset(e, 0, sizeof(*e)); strlcpy(e->call, realList[i].call, sizeof(e->call)); strlcpy(call, e->call, n); found = true;
  }
  unlock(); return found;
}
void dmrSetInfo(const char *call, const char *name, const char *city, uint32_t id)
{
  lock();
  DmrInfo *e = infoFind(call);
  if(e) { strlcpy(e->name, name ? name : "", sizeof(e->name)); strlcpy(e->city, city ? city : "", sizeof(e->city)); e->id = id; }
  for(int i = 0 ; i < nReal ; i++) if(!strcmp(realList[i].call, call)) applyInfo(realList[i]);
  unlock();
}
void dmrSetDebug(const char *m) { lock(); snprintf(dbgMsg, sizeof(dbgMsg), "%s", m ? m : ""); unlock(); }
void dmrClearReal() { lock(); nReal = 0; unlock(); }     // keeps "live" mode, the list is refilled right after
void dmrExpireLive()
{
  lock();
  for(int i = 0 ; i < nReal ; i++) if(realList[i].live && (millis() - realList[i].tms) > 90000UL) realList[i].live = false;
  unlock();
}
