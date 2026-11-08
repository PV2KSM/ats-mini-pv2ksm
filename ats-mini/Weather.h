#pragma once
#include "Common.h"

void weatherBegin();                    // Create the worker task (call once, from setup)
void weatherOpen();                     // Called when the Weather screen is opened
bool weatherIsActive();
int weatherPage();                      // Current page of the Weather screen (4 = rain radar)
bool weatherClick();                    // Encoder click; true when handled by the current page                 // True while the Weather screen is shown
bool weatherTick();                     // True when the screen needs a redraw
void weatherScroll(int16_t delta);      // Encoder rotation: change page
void weatherDraw(LGFX_Sprite &spr);
struct WxData;
bool weatherGetSnapshot(WxData *out, char *status, size_t statusSize);   // Copy of the latest data; true when valid
