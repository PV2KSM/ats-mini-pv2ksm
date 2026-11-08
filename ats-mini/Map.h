#pragma once
#include "Common.h"

// Basemap for the Flights radar: raster tiles (PNG) are downloaded, decoded and
// resampled into a full-screen RGB565 image centred on the observer.
#define MAP_W      320      // The basemap fills the whole screen
#define MAP_H      170
#define MAP_R      85       // Pixels from the screen centre to the top/bottom edge = the configured range (nm)

void mapBegin();                                  // Create the worker task (call once, from setup)
void mapEnsure();                                 // Flights screen opened: build the map if missing/outdated
void mapInvalidate();                             // Position, radius, key or tile URL changed
bool mapDraw(LGFX_Sprite &spr, int x, int y);     // Draw the basemap if available; false = nothing drawn
const char *mapStatus();                          // Short text for the screen ("" = nothing to report)

// Tile URL from the configured template and key (used by the rain radar for its base map)
bool mapTileUrl(char *out, size_t size, int z, int x, int y, bool light);
