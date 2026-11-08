#pragma once
#include "Common.h"

// DMR screen: who is talking (call sign, name, talkgroup) on a DMR network and/or on the user's hotspot.
// Five looks (Visual -> DMR theme). Settings come from the web page (Services -> DMR).
// Data: until a live feed is connected the radio shows demonstration traffic, marked DEMO on screen.

#define DMR_THEMES 9
extern uint8_t uiDmrTheme;                // 0 Classic, 1 Handheld, 2 Pi-Star, 3 Phosphor, 4 Minimal, 5 Flag Hero, 6 Badge, 7 Rows, 8 Paper

struct DmrCfg
{
  uint8_t net;                            // 0 BrandMeister, 1 TGIF, 2 DMR+, 3 FreeDMR
  uint8_t src;                            // 0 network, 1 hotspot, 2 both
  uint8_t rows;                           // Lines of the list (4..8)
  uint32_t tg;                            // Preferred talkgroup (0 = all)
  char host[32];                          // Hotspot address (name or IP)
};

void dmrBegin();                          // Load the saved settings (call once, from setup)
void dmrOpen();                           // Called when the DMR screen is opened
bool dmrTick();                           // Main loop: true when a redraw is needed
void dmrDraw(LGFX_Sprite &spr);
void dmrScroll(int16_t delta);            // Encoder: previous / next station
void dmrDrawPreview(LGFX_Sprite &spr, int theme);   // Visual menu: demo traffic in the look being chosen

void dmrGetCfg(DmrCfg *out);
void dmrSetCfg(const DmrCfg &cfg);        // Saved at once
const char *dmrNetName(int i);            // 4 names
const char *dmrThemeName(int i);          // English names, translated with uiLbl()

// Worker interface (DmrNet.cpp)
bool dmrWanted();                                        // The DMR screen is open
void dmrSetStatus(uint8_t kind, const char *msg);        // 1 info, 2 live, 3 error
void dmrPush(const char *call, const char *name, const char *place, uint32_t tg, int dur, bool live, int agoMinutes, bool dedupe, uint32_t id = 0);
void dmrSetDebug(const char *msg);
bool dmrNextLookup(char *call, size_t n);                // A listed callsign without name/ID yet (marks it as asked)
void dmrSetInfo(const char *call, const char *name, const char *city, uint32_t id);   // RadioID answer (empty = not found)
void dmrClearReal();
void dmrExpireLive();
void dmrNetBegin();                                      // Allocate the buffers (call once, from setup)
void dmrRun();                                           // Runs the DMR worker until the screen is left; called from a network task that has the stack for TLS
