//
// "Cockpit" UI layout: airliner radio panel (ACTIVE/STANDBY windows, push-button matrix, signal windows).
// The drawing lives in Cockpit737UI.h with pre-rendered art in Cockpit737Art.h (shared with the PC preview).
//
#include "Common.h"
#include "Themes.h"
#include "Utils.h"
#include "Menu.h"
#include "Draw.h"
#include "Weather.h"
#include "Location.h"
#include "Cockpit737UI.h"

// Animation frames while the main screen is idle on this layout
bool cockpitTick()
{
  static uint32_t shownMs = 0;
  if(uiLayoutIdx != UI_COCKPIT || sleepOn()) return false;
  if(currentCmd != CMD_NONE && currentCmd != CMD_FREQ) return false;
  const uint32_t now = millis();
  if((uint32_t)(now - shownMs) < 100) return false;
  shownMs = now;
  return true;
}

void drawLayoutCockpit()
{
  CkColors c;
  c.bg = TH.bg;            c.text = TH.text;         c.muted = TH.text_muted;  c.warn = TH.text_warn;
  c.label = TH.band_text;  c.freq = TH.freq_text;    c.unit = TH.funit_text;
  c.hl = TH.freq_hl;       c.hlSel = TH.freq_hl_sel; c.station = TH.rds_text;
  c.bar = TH.smeter_bar;   c.barPlus = TH.smeter_bar_plus; c.barEmpty = TH.smeter_bar_empty;
  c.snr = TH.scan_snr;     c.line = TH.scale_line;   c.pointer = TH.scale_pointer;

  static CkIn in;
  const Band *band = getCurrentBand();
  in.freq = currentFrequency;
  in.stby = (vfoInitialized && vfos[vfoIdx ^ 1].freq) ? vfos[vfoIdx ^ 1].freq : currentFrequency;
  in.mode = currentMode;
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

  const float sV = ckStrength(in.rssi, in.mode == 0) / 49.0f;
  float nV = in.snr / 40.0f;
  if(nV > 1) nV = 1;
  if(nV < 0) nV = 0;

  // The side menu replaces the weather card while a command is active
  const bool idle = (currentCmd == CMD_NONE || currentCmd == CMD_FREQ);
  bxDraw(spr, in, c, false, sV, nV);

  // Status icons (same helpers as the other layouts), placed in the header
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

  if(currentCmd == CMD_SCAN)
  {
    drawScanGraphs(currentFrequency);
  }
  else if(!idle)
  {
    drawSideBar(currentCmd, MENU_OFFSET_X, MENU_OFFSET_Y, MENU_DELTA_X);
  }
  drawStatus(STATUS_OFFSET_X + 40, 100);
}
