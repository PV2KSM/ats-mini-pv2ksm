#pragma once
#include "Common.h"

void netStatusOpen();                       // Network status screen opened
void netStatusScroll(int16_t delta);        // Encoder: switch page (connection / services)
bool netStatusTick();                       // True when the screen needs a redraw (once per second)
void netStatusDraw(LGFX_Sprite &spr);
