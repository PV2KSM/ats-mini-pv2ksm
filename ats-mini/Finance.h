#pragma once
#include "Common.h"

void financeBegin();                      // Create the worker task (call once, from setup)
void financeOpen();                       // Called when the Finance screen is opened
bool financeIsActive();                   // True while the Finance screen is shown
bool financeTick();                       // Main loop: apply a queued currency change; true when the screen needs a redraw
void financeDraw(LGFX_Sprite &spr);

// Local currency used for the conversions (ISO 4217, e.g. BRL)
void financeGetCurrency(char *out, size_t size);
const char *financeRequestCurrency(const char *code);   // nullptr on success, else an error message
const char *financeCurrencyList();                      // Space separated list of the supported codes
