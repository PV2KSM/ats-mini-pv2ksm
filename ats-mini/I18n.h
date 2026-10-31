#ifndef I18N_H
#define I18N_H

#include <stdint.h>

// Language of the extra screens (Weather, Flights, Finance, Network). ASCII only: the screen
// fonts have no accents, so texts are written without them ("Sao Jose", "Presion").
#define LANG_PT 0
#define LANG_EN 1
#define LANG_ES 2
#define LANG_TOTAL 3

extern uint8_t uiLang;                 // Saved in the preferences
extern const char *const uiLangDesc[]; // "Portugues", "English", "Espanol"
int getTotalLanguages();

// Texts. Add a new id here AND a row (pt, en, es) in I18n.cpp, in the same order.
enum TrId : uint8_t
{
  T_SEARCHING, T_SEARCHING_WX, T_SEARCHING_METAR, T_SEARCHING_RATES, T_WAITING,
  T_NO_WIFI, T_NO_TIME, T_NO_MEM, T_HTTP_ERR, T_TLS_ERR, T_NET_ERR, T_BIG_RESP, T_READ_FAIL,
  T_JSON_ERR, T_BAD_DATA, T_UNKNOWN_ERR,
  T_SET_ICAO, T_NO_METAR, T_CALM, T_NO_CLOUDS,
  T_NOW, T_FEELS, T_HUMIDITY, T_WIND, T_RAIN, T_PRESSURE, T_NEXT_DAYS, T_NEXT_HOURS, T_TODAY,
  T_SUNRISE, T_BARS_RAIN,
  T_VISIB, T_CEIL, T_TEMPDEW, T_WX, T_FETCHED,
  T_FIN_TITLE, T_FIN_USD, T_FIN_EUR, T_FIN_LOCAL, T_FIN_DERIV_S, T_FIN_REFS, T_FIN_NO_PRICE,
  T_FIN_BAD_PRICE, T_FIN_SOURCES,
  T_FL_HDR, T_FL_HDR0, T_FL_NONE, T_FL_SET_POS, T_FL_NO_AC, T_FL_ON_GROUND, T_FL_CLOSE,
  T_FL_ALT, T_FL_SPD, T_FL_HDG, T_FL_VS, T_FL_DIST,
  T_NET_TITLE, T_SVC_TITLE, T_NET_CHANNEL, T_NET_TIME, T_NET_UPTIME, T_NET_MEMORY, T_NET_RESET,
  T_NET_NEVER, T_NET_DISCONNECTED, T_NET_NO_NTP,
  T_SVC_FLIGHTS, T_SVC_MAP, T_SVC_WEATHER, T_SVC_FINANCE,
  T_RADAR_NO_WIFI, T_RADAR_NO_TIME, T_RADAR_NO_MEM, T_RADAR_BUSY, T_RADAR_MAP, T_RADAR_RAIN,
  T_RADAR_RAIN_N, T_RADAR_FAIL, T_RADAR_PARTIAL, T_RADAR_NO_RAIN, T_RADAR_TITLE_T, T_RADAR_TITLE,
  T_RADAR_LOADING, T_RADAR_ZOOM,
  T_MAP_URL, T_MAP_NET, T_MAP_HTTP, T_MAP_TILE_BAD, T_MAP_NO_WIFI, T_MAP_NO_TIME, T_MAP_NO_MEM,
  T_MAP_PNG, T_MAP_TILE_N, T_MAP_PARTIAL,
  T_COUNT
};

const char *tr(TrId id);               // Text in the current language
const char *trWx(int wmoCode);         // Weather condition for an Open-Meteo (WMO) code
const char *trDay(int dow);            // 0 = Sunday ... 6 = Saturday

#endif // I18N_H
