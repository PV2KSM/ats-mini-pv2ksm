#pragma once
#include "Common.h"

void radarBegin();                       // Create the worker task (call once, from setup)
void radarWake();                        // The radar page was opened: build it if missing or old
void radarInvalidate();                  // Position or map settings changed
void radarToggleZoom();                  // Encoder click on the radar page
void radarDraw(LGFX_Sprite &spr);        // Full screen, includes the page dots (page index in the Weather screen)
