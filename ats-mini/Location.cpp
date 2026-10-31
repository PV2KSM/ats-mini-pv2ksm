#include "Location.h"
#include <Arduino.h>
#include <Preferences.h>
#include <mutex>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "Airports.h"

// Defaults: Sao Jose dos Campos, SP
#define DEF_NAME   "Sao Jose dos Campos"
#define DEF_LABEL  "SJK"
#define DEF_LAT    (-23.2237f)
#define DEF_LON    (-45.9009f)
#define DEF_RADIUS 40

static std::mutex mtx;
static LocationInfo current;
static char cartoKey[64] = "";
static bool havePending = false;
static LocationInfo pendingLoc;
static char pendingKey[64];
static bool pendingKeyChange = false;
static char mapUrl[160] = LOCATION_DEFAULT_TILE_URL;
static bool mapOn = true;
static uint8_t mapStyle = MAP_STYLE_LIGHT;
static uint8_t pendingMapStyle = MAP_STYLE_LIGHT;
static bool havePendingMap = false;
static char pendingMapUrl[160];
static bool pendingMapOn = true;

static void defaults(LocationInfo *l)
{
  memset(l, 0, sizeof(*l));
  strlcpy(l->name, DEF_NAME, sizeof(l->name));
  strlcpy(l->label, DEF_LABEL, sizeof(l->label));
  l->lat = DEF_LAT;
  l->lon = DEF_LON;
  l->radiusNm = DEF_RADIUS;
}

void locationBegin()
{
  std::lock_guard<std::mutex> lock(mtx);
  defaults(&current);
  Preferences p;
  if(p.begin("location", true))
  {
    if(p.isKey("lat"))
    {
      p.getString("name", current.name, sizeof(current.name));
      p.getString("label", current.label, sizeof(current.label));
      p.getString("icao", current.icao, sizeof(current.icao));
      current.lat = p.getFloat("lat", DEF_LAT);
      current.lon = p.getFloat("lon", DEF_LON);
      current.radiusNm = constrain(p.getInt("radius", DEF_RADIUS), 5, 110);
      p.getString("ckey", cartoKey, sizeof(cartoKey));
    }
    if(p.isKey("murl")) p.getString("murl", mapUrl, sizeof(mapUrl));
    mapOn = p.getBool("mon", true);
    mapStyle = constrain(p.getUChar("mstyle", MAP_STYLE_LIGHT), 0, 2);
    if(!mapUrl[0] || !strcmp(mapUrl, LOCATION_OLD_TILE_URL) || !strcmp(mapUrl, LOCATION_DARK_TILE_URL) || !strcmp(mapUrl, LOCATION_LIGHT_TILE_URL)) strlcpy(mapUrl, LOCATION_DEFAULT_TILE_URL, sizeof(mapUrl));
    p.end();
  }
  current.hasCartoKey = cartoKey[0] != 0;
}

LocationInfo locationGet()
{
  std::lock_guard<std::mutex> lock(mtx);
  return current;
}

void locationGetCartoKey(char *out, size_t size)
{
  std::lock_guard<std::mutex> lock(mtx);
  strlcpy(out, cartoKey, size);
}

void locationGetMapCfg(char *url, size_t size, bool *on, uint8_t *style)
{
  std::lock_guard<std::mutex> lock(mtx);
  // Show the values just submitted (still pending) so the web page is up to date right after Save
  if(url) strlcpy(url, havePendingMap ? pendingMapUrl : mapUrl, size);
  if(on) *on = havePendingMap ? pendingMapOn : mapOn;
  if(style) *style = havePendingMap ? pendingMapStyle : mapStyle;
}

const char *locationRequestMapCfg(const char *url, bool on, uint8_t style)
{
  if(!url) url = "";
  if(strlen(url) >= sizeof(pendingMapUrl)) return "Tile URL too long.";
  if(*url)
  {
    if(strncmp(url, "https://", 8)) return "Tile URL must start with https://";
    if(!strstr(url, "{z}") || !strstr(url, "{x}") || !strstr(url, "{y}")) return "Tile URL needs {z}, {x} and {y}.";
    for(const char *c = url ; *c ; c++) if((unsigned char)*c < 33 || (unsigned char)*c > 126) return "Tile URL has invalid characters.";
  }
  std::lock_guard<std::mutex> lock(mtx);
  strlcpy(pendingMapUrl, (*url && strcmp(url, LOCATION_OLD_TILE_URL) && strcmp(url, LOCATION_DARK_TILE_URL) && strcmp(url, LOCATION_LIGHT_TILE_URL)) ? url : LOCATION_DEFAULT_TILE_URL, sizeof(pendingMapUrl));
  pendingMapOn = on;
  pendingMapStyle = style > 2 ? MAP_STYLE_LIGHT : style;
  havePendingMap = true;
  return nullptr;
}

int locationAirportCount() { return airportCount; }

int locationNearbyAirports(float lat, float lon, float dNorthNm, float dEastNm, NearbyAirport *out, int maxOut)
{
  const float cosLat = cosf(lat * (float)M_PI / 180.0f);
  int n = 0;
  float dist2[24];
  const int cap = maxOut < 24 ? maxOut : 24;
  for(int i = 0 ; i < airportCount ; i++)
  {
    AirportRec rec;
    memcpy_P(&rec, &airportTable[i], sizeof(rec));
    const float north = (rec.lat - lat) * 60.0f;
    float dlon = rec.lon - lon;
    if(dlon > 180.0f) dlon -= 360.0f; else if(dlon < -180.0f) dlon += 360.0f;
    const float east = dlon * 60.0f * cosLat;
    if(fabsf(north) > dNorthNm || fabsf(east) > dEastNm) continue;
    const float d2 = north * north + east * east;
    if(d2 < 0.25f) continue;                            // The position itself
    // Insert sorted, keeping the nearest `cap`
    int pos;
    if(n < cap) { pos = n; n++; }
    else { if(d2 >= dist2[cap - 1]) continue; pos = cap - 1; }
    while(pos > 0 && dist2[pos - 1] > d2) { dist2[pos] = dist2[pos - 1]; out[pos] = out[pos - 1]; pos--; }
    memcpy(out[pos].icao, rec.icao, 5);
    out[pos].northNm = north; out[pos].eastNm = east;
    dist2[pos] = d2;
  }
  return n;
}

bool locationLookupIcao(const char *icao, LocationInfo *out)
{
  if(!icao || strlen(icao) != 4) return false;
  char key[4];
  for(int i = 0 ; i < 4 ; i++)
  {
    if(!isalpha((unsigned char)icao[i])) return false;
    key[i] = toupper((unsigned char)icao[i]);
  }
  int lo = 0, hi = airportCount - 1;
  while(lo <= hi)
  {
    int mid = (lo + hi) / 2;
    int c = memcmp(key, airportTable[mid].icao, 4);
    if(!c)
    {
      if(out)
      {
        AirportRec rec;
        memcpy_P(&rec, &airportTable[mid], sizeof(rec));
        memset(out, 0, sizeof(*out));
        memcpy(out->icao, key, 4);
        strlcpy(out->name, rec.name, sizeof(out->name));
        out->lat = rec.lat;
        out->lon = rec.lon;
      }
      return true;
    }
    if(c < 0) hi = mid - 1; else lo = mid + 1;
  }
  return false;
}

// Keep only printable ASCII, trimmed
static void clean(char *dst, size_t size, const char *src, bool upperAlnum)
{
  size_t n = 0;
  for( ; src && *src && n + 1 < size ; src++)
  {
    unsigned char c = *src;
    if(upperAlnum) { if(!isalnum(c)) continue; c = toupper(c); }
    else if(c < 32 || c > 126 || c == '"' || c == '<' || c == '>' || c == '&') continue;
    dst[n++] = c;
  }
  dst[n] = 0;
  while(n && dst[n - 1] == ' ') dst[--n] = 0;
}

const char *locationRequestSet(const LocationInfo &in, const char *key)
{
  if(!isfinite(in.lat) || !isfinite(in.lon)) return "Invalid coordinates.";
  if(in.lat < -90.0f || in.lat > 90.0f || in.lon < -180.0f || in.lon > 180.0f) return "Coordinates out of range.";
  if(in.lat == 0.0f && in.lon == 0.0f) return "Coordinates cannot both be zero.";
  if(in.radiusNm < 5 || in.radiusNm > 110) return "Range must be 5 to 110 nm.";
  if(key && strlen(key) > 60) return "Key too long.";

  std::lock_guard<std::mutex> lock(mtx);
  pendingLoc = in;
  clean(pendingLoc.name, sizeof(pendingLoc.name), in.name, false);
  clean(pendingLoc.label, sizeof(pendingLoc.label), in.label, true);
  clean(pendingLoc.icao, sizeof(pendingLoc.icao), in.icao, true);
  if(!pendingLoc.name[0]) strlcpy(pendingLoc.name, "Local", sizeof(pendingLoc.name));
  pendingKeyChange = key != nullptr;
  if(key) clean(pendingKey, sizeof(pendingKey), key, false);
  havePending = true;
  return nullptr;
}

bool locationTick()
{
  std::unique_lock<std::mutex> lock(mtx);
  if(havePendingMap)
  {
    havePendingMap = false;
    strlcpy(mapUrl, pendingMapUrl, sizeof(mapUrl));
    mapOn = pendingMapOn;
    mapStyle = pendingMapStyle;
    const uint8_t st = mapStyle;
    char u[160];
    strlcpy(u, mapUrl, sizeof(u));
    bool on = mapOn;
    lock.unlock();
    Preferences mp;
    if(mp.begin("location", false)) { mp.putString("murl", u); mp.putBool("mon", on); mp.putUChar("mstyle", st); mp.end(); }
    mapInvalidate();
    radarInvalidate();
    lock.lock();
    if(!havePending) return true;
  }
  if(!havePending) return false;
  havePending = false;
  bool moved = fabsf(pendingLoc.lat - current.lat) > 0.00005f || fabsf(pendingLoc.lon - current.lon) > 0.00005f
            || pendingLoc.radiusNm != current.radiusNm;
  current = pendingLoc;
  const bool keyChanged = pendingKeyChange;
  if(keyChanged) strlcpy(cartoKey, pendingKey, sizeof(cartoKey));
  current.hasCartoKey = cartoKey[0] != 0;
  LocationInfo saved = current;
  char keyCopy[64];
  strlcpy(keyCopy, cartoKey, sizeof(keyCopy));
  lock.unlock();

  Preferences p;
  if(p.begin("location", false))
  {
    p.putString("name", saved.name);
    p.putString("label", saved.label);
    p.putString("icao", saved.icao);
    p.putFloat("lat", saved.lat);
    p.putFloat("lon", saved.lon);
    p.putInt("radius", saved.radiusNm);
    p.putString("ckey", keyCopy);
    p.end();
  }
  if(moved || keyChanged) { mapInvalidate(); radarInvalidate(); }
  if(moved) { flightsLocationChanged(); weatherLocationChanged(); }
  return true;
}
