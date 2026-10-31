//
// PV2KSM new main screens (Neo Analog, Neo S-Meter, Vintage, Clock, Memories, Minimal, VFD, LCD, Neon). Drawing lives in NewUI.h (shared with the PC preview).
//
#include "Common.h"
#include "Themes.h"
#include "Utils.h"
#include "Menu.h"
#include "Draw.h"
#include "Weather.h"
#include "Memories.h"
#include "NewUI.h"
#include "NewUI2.h"

// Palette from the user's options
NuPal nuCurrentPalette() { return nuPalette(uiAccent, uiTextWhite, uiBgIdx); }

// Animation frames while the main screen is idle
bool newTick()
{
  static uint32_t shownMs = 0;
  if(uiLayoutIdx < UI_NEW_FIRST || sleepOn()) return false;
  if(currentCmd != CMD_NONE && currentCmd != CMD_FREQ) return false;
  const uint32_t now = millis();
  if((uint32_t)(now - shownMs) < (uiLayoutIdx == UI_CLOCK ? 1000u : 100u)) return false;
  shownMs = now;
  return true;
}

// Memories of the current band, refreshed twice a second
static NuMem memCache[24];
static int memCount = 0;
static void refreshMemories()
{
  static uint32_t at = 0;
  static const Band *lastBand = nullptr;
  const Band *band = getCurrentBand();
  const uint32_t now = millis();
  if(band == lastBand && memCount >= 0 && (uint32_t)(now - at) < 500 && at) return;
  at = now; lastBand = band; memCount = 0;
  for(int i = 0 ; i < getTotalMemories() && memCount < 24 ; i++)
  {
    const Memory m = getMemory(i);
    if(!m.freq || !isMemoryInBand(band, &m)) continue;
    memCache[memCount].freq = m.freq;
    strncpy(memCache[memCount].name, m.name, 9); memCache[memCount].name[9] = 0;
    if(!memCache[memCount].name[0]) snprintf(memCache[memCount].name, 10, "%u", (unsigned)(m.freq / (m.mode == FM ? 10000 : 1000)));
    memCount++;
  }
}

void drawLayoutNew()
{
  static NuIn n;
  CkIn &in = n.k;
  const Band *band = getCurrentBand();
  in.freq = currentFrequency;
  in.mode = currentMode;
  in.band = band->bandName;
  in.modeName = bandModeDesc[currentMode];
  const char *name = getStationName();
  in.station = (*name == (char)0xFF) ? name + 1 : name;
  in.rssi = rssi;
  in.snr = snr;
  in.stereo = (currentMode == FM) && (fmStereoIdx != FM_STEREO_MONO) && rx.getCurrentPilot();
  in.step = getCurrentStep()->desc;
  in.bw = getCurrentBandwidth()->desc;
  static char agcText[8];
  if(!agcNdx && !agcIdx) { in.agcLabel = "AGC"; in.agc = "On"; }
  else { snprintf(agcText, sizeof(agcText), "%2.2d", agcNdx); in.agcLabel = "ATT"; in.agc = agcText; }
  in.volume = (muteOn(MUTE_MAIN) || muteOn(MUTE_SQUELCH)) ? -1 : volume;
  in.minFreq = band->minimumFreq;
  in.maxFreq = band->maximumFreq;
  in.hl = (currentCmd == CMD_FREQ) ? getFreqInputPos() + (pushAndRotate ? 0x80 : 0) : 100;
  in.t = millis() / 1000.0f;

  uint8_t hh, mm;
  in.haveTime = clockGetHM(&hh, &mm);
  in.hh = hh; in.mm = mm;
  uint16_t yr; uint8_t mo, dy, wd;
  if(clockGetDate(&yr, &mo, &dy, &wd)) { n.month = mo; n.day = dy; n.wday = wd % 7; } else n.month = 0;

  static char wxStatus[24];
  in.haveWx = weatherGetSnapshot(&in.wx, wxStatus, sizeof(wxStatus)) && in.wx.valid;
  in.wxStatus = in.haveWx ? "" : "Open Weather";

  // Smoothed needles (fast attack, slower release)
  static float sN = 0, nN = 0;
  const float st = ckStrength(in.rssi, in.mode == 0) / 49.0f;
  float nt = in.snr / 40.0f; if(nt > 1) nt = 1; if(nt < 0) nt = 0;
  sN += (st - sN) * (st > sN ? 0.45f : 0.25f);
  nN += (nt - nN) * (nt > nN ? 0.45f : 0.25f);
  n.sN = sN; n.nN = nN;

  refreshMemories();
  n.mem = memCache; n.nMem = memCount;

  const NuPal p = nuCurrentPalette();
  switch(uiLayoutIdx)
  {
    case UI_NSMETER:  nu::layoutSMeter(spr, n, p);   break;
    case UI_VINTAGE:  nu::layoutVintage(spr, n, p);  break;
    case UI_CLOCK:    nu::layoutClock(spr, n, p);    break;
    case UI_MEMORIES: nu::layoutMemories(spr, n, p); break;
    case UI_MINIMAL:  nu::layoutMinimal(spr, n, p);  break;
    case UI_VFD:      nu::layoutVfd(spr, n, p);      break;
    case UI_LCD:      nu::layoutLcd(spr, n, p);      break;
    case UI_NEON:     nu::layoutNeon(spr, n, p);     break;
    default:          nu::layoutAnalog(spr, n, p);   break;
  }

  const bool idle = (currentCmd == CMD_NONE || currentCmd == CMD_FREQ);

  drawSaveIndicator(190, 0);
  drawBleIndicator(204, 0);
  bool has_voltage = drawBattery(BATT_OFFSET_X, BATT_OFFSET_Y);
  drawWiFiIndicator(has_voltage ? WIFI_OFFSET_X : BATT_OFFSET_X - 13, WIFI_OFFSET_Y);

  if(switchThemeEditor())
  {
    spr.setFont(FONT_SMALL);
    spr.setTextDatum(TR_DATUM);
    spr.setTextColor(TH.text_warn);
    spr.drawString(TH.name, 319, BATT_OFFSET_Y + 17);
  }

  if(currentCmd == CMD_SCAN) drawScanGraphs(currentFrequency);
  else if(!idle) drawSideBar(currentCmd, MENU_OFFSET_X, MENU_OFFSET_Y, MENU_DELTA_X);
  drawStatus(STATUS_OFFSET_X + 40, 100);
}
