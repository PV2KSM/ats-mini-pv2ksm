//
// "Analog" UI layout: big frequency and analog S-meter / SNR dials. Drawing lives in CessnaUI.h.
//
#include "Common.h"
#include "Themes.h"
#include "Utils.h"
#include "Menu.h"
#include "Draw.h"
#include "StatusChips.h"
#include "CessnaUI.h"

// Needle animation frames while the main screen is idle on this layout
bool avionicsTick()
{
  static uint32_t shownMs = 0;
  if(uiLayoutIdx != UI_AVIONICS || sleepOn()) return false;
  if(currentCmd != CMD_NONE && currentCmd != CMD_FREQ) return false;
  const uint32_t now = millis();
  if((uint32_t)(now - shownMs) < 80) return false;
  shownMs = now;
  return true;
}

void drawLayoutAvionics()
{
  CkColors c;
  c.bg = TH.bg;            c.text = TH.text;         c.muted = TH.text_muted;  c.warn = TH.text_warn;
  c.label = TH.band_text;  c.freq = TH.freq_text;    c.unit = TH.funit_text;
  c.hl = TH.freq_hl;       c.hlSel = TH.freq_hl_sel; c.station = TH.rds_text;
  c.bar = TH.smeter_bar;   c.barPlus = TH.smeter_bar_plus; c.barEmpty = TH.smeter_bar_empty;
  c.snr = TH.scan_snr;     c.line = TH.scale_line;   c.pointer = TH.scale_pointer;
  c.box = TH.box_bg;

  static CkIn in;
  const Band *band = getCurrentBand();
  in.freq = currentFrequency;
  in.mode = currentMode;
  in.band = band->bandName;
  in.modeName = bandModeDesc[currentMode];
  const char *name = getStationName();
  in.station = (*name == (char)0xFF) ? name + 1 : name;
  in.minFreq = band->minimumFreq;
  in.maxFreq = band->maximumFreq;
  in.rssi = rssi;
  in.snr = snr;
  in.stereo = (currentMode == FM) && (fmStereoIdx != FM_STEREO_MONO) && rx.getCurrentPilot();
  in.squelch = currentSquelch[currentMode] & 0x7f;
  in.sqlSnr = (currentSquelch[currentMode] & 0x80) != 0;
  in.muted = muteOn(MUTE_MAIN);
  in.hl = (currentCmd == CMD_FREQ) ? getFreqInputPos() + (pushAndRotate ? 0x80 : 0) : 100;

  // Smoothed needles (fast attack, a little slower release)
  static float sN = 0, nN = 0;
  const float st = anSTarget(in), nt = anNTarget(in);
  sN += (st - sN) * (st > sN ? 0.45f : 0.25f);
  nN += (nt - nN) * (nt > nN ? 0.45f : 0.25f);

  csDraw(spr, in, c, sN, nN, 0);

  const bool idle = (currentCmd == CMD_NONE || currentCmd == CMD_FREQ);

  if(currentCmd == CMD_SCAN) drawScanForLayout(in, SCAN_STYLE_AVIONICS);
  else scDraw(spr, in, SC_AVION, 0, 0, 0);
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

  if(!idle && currentCmd != CMD_SCAN) drawSideBar(currentCmd, MENU_OFFSET_X, MENU_OFFSET_Y, MENU_DELTA_X);
  drawStatus(STATUS_OFFSET_X + 40, 100);
}
