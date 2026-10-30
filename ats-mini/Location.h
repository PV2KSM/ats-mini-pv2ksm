#pragma once
#include <stddef.h>
#include <stdint.h>

// Observer position shared by the Flights and Weather screens.
struct LocationInfo
{
  char name[28];     // Display name (ASCII)
  char label[8];     // Short label drawn at the observer position, e.g. "SJK"
  char icao[5];      // Optional airport ICAO code the position came from
  float lat;
  float lon;
  int radiusNm;      // Flights radius in nautical miles
  bool hasCartoKey;  // A CARTO basemap key is stored (the key itself is never shown back)
};

void locationBegin();                                       // Load from NVS (call once from setup)
LocationInfo locationGet();                                 // Thread-safe copy
int locationAirportCount();
// Airports around the position, inside +-dNorthNm / +-dEastNm, sorted by distance (nearest first)
struct NearbyAirport { char icao[5]; float northNm, eastNm; };
int locationNearbyAirports(float lat, float lon, float dNorthNm, float dEastNm, NearbyAirport *out, int maxOut);
bool locationLookupIcao(const char *icao, LocationInfo *out); // Embedded airport table; fills name/lat/lon/icao
// Queue a change from the web task. cartoKey: nullptr = keep the stored key, "" = erase it.
// Returns nullptr on success or a short error message.
const char *locationRequestSet(const LocationInfo &loc, const char *cartoKey);
bool locationTick();                                        // Main loop: apply queued change, true if changed
void locationGetCartoKey(char *out, size_t size);           // For the map layer

// Basemap for the Flights radar. url is a tile template with {z} {x} {y} (and optional {key}).
enum { MAP_STYLE_DARK = 0, MAP_STYLE_LIGHT = 1, MAP_STYLE_AUTO = 2 };   // Auto: light 06:00-18:00 local time
void locationGetMapCfg(char *url, size_t size, bool *on, uint8_t *style = nullptr);
const char *locationRequestMapCfg(const char *url, bool on, uint8_t style = MAP_STYLE_LIGHT);  // nullptr on success, else an error message
#define LOCATION_DEFAULT_TILE_URL "https://basemaps.cartocdn.com/rastertiles/{style}/{z}/{x}/{y}.png"
#define LOCATION_LIGHT_TILE_URL   "https://basemaps.cartocdn.com/rastertiles/light_all/{z}/{x}/{y}.png"
#define LOCATION_DARK_TILE_URL    "https://basemaps.cartocdn.com/rastertiles/dark_all/{z}/{x}/{y}.png"
#define LOCATION_OLD_TILE_URL     "https://basemaps.cartocdn.com/dark_all/{z}/{x}/{y}.png"   // r5 default (no longer served without a key)

// Implemented by the screens that depend on the position
void flightsLocationChanged();
void weatherLocationChanged();
void radarInvalidate();                                    // Radar.cpp: rebuild the rain radar
void mapInvalidate();                                       // Map.cpp: rebuild the radar basemap
