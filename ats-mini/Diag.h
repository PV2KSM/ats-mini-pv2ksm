#pragma once
#include <stdint.h>
#include <stddef.h>

// Breadcrumbs that survive a crash/watchdog reset (RTC memory), shown on the web Status page,
// and a lock that keeps the network workers (flights, weather, map) from running TLS at the same time.
enum
{
  DG_NONE = 0,
  DG_MAP_START, DG_MAP_FETCH, DG_MAP_DECODE, DG_MAP_RESAMPLE, DG_MAP_PUBLISH, DG_MAP_DONE,
  DG_FL_FETCH, DG_FL_PARSE, DG_FL_DRAW, DG_FL_DRAW_MAP, DG_FL_DRAW_PLANES, DG_FL_DRAW_PANEL,
  DG_WX_FETCH, DG_WX_PARSE
};

void diagBegin();                                 // Call first in setup()
void diagStage(uint8_t stage, uint8_t aux = 0);   // Record where we are (and the free internal heap)
void diagText(char *out, size_t size);            // One line: reset reason, last step, heap at that moment

bool netLock(uint32_t timeoutMs);                 // One TLS conversation at a time
void netUnlock();
