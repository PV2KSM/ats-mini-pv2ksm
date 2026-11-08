#pragma once
#include "Common.h"

#define FLIGHTS_REFRESH_MS        15000
#define FLIGHTS_MAX               24

// Set to 1 to print [FLIGHT] diagnostics on the USB serial port. Keep it 0 in
// normal use: the serial port is also used by the radio's remote-control protocol.
#define FLIGHTS_DEBUG             0

void flightsBegin();                                      // Create the worker task (call once, from setup)
void flightsOpen();                                       // Called when the Flights screen is opened
bool flightsIsActive();                                   // True while the Flights screen is shown
bool flightsTick();                                       // True when the screen needs a redraw
void flightsScroll(int16_t delta);                        // Encoder rotation: select the next/previous aircraft
void flightsToggleInfo();                                 // Encoder click: open/close the detail card
void flightsDraw(LGFX_Sprite &spr);
