#include "I18n.h"

uint8_t uiLang = LANG_EN;   // Default on a fresh install
const char *const uiLangDesc[] = { "Portugues", "English", "Espanol" };   // Order matches LANG_PT/EN/ES
int getTotalLanguages() { return LANG_TOTAL; }

//                    pt                          en                          es
static const char *const T[T_COUNT][LANG_TOTAL] =
{
  /* T_SEARCHING        */ {"Buscando...",              "Searching...",             "Buscando..."},
  /* T_SEARCHING_WX     */ {"Buscando clima...",        "Fetching weather...",      "Buscando clima..."},
  /* T_SEARCHING_METAR  */ {"Buscando METAR...",        "Fetching METAR...",        "Buscando METAR..."},
  /* T_SEARCHING_RATES  */ {"Buscando cotacoes...",     "Fetching rates...",        "Buscando cotizaciones..."},
  /* T_WAITING          */ {"Aguardando",               "Waiting",                  "Esperando"},
  /* T_NO_WIFI          */ {"Sem Wi-Fi",                "No Wi-Fi",                 "Sin Wi-Fi"},
  /* T_NO_TIME          */ {"Sem hora (NTP)",           "No time (NTP)",            "Sin hora (NTP)"},
  /* T_NO_MEM           */ {"Sem memoria",              "Out of memory",            "Sin memoria"},
  /* T_HTTP_ERR         */ {"HTTP erro %d",             "HTTP error %d",            "HTTP error %d"},
  /* T_TLS_ERR          */ {"TLS erro (%d)",            "TLS error (%d)",           "TLS error (%d)"},
  /* T_NET_ERR          */ {"Rede erro %d",             "Network error %d",         "Error de red %d"},
  /* T_BIG_RESP         */ {"Resposta grande",          "Response too big",         "Respuesta grande"},
  /* T_READ_FAIL        */ {"Leitura falhou",           "Read failed",              "Lectura fallida"},
  /* T_JSON_ERR         */ {"Erro JSON",                "JSON error",               "Error JSON"},
  /* T_BAD_DATA         */ {"Dados invalidos",          "Invalid data",             "Datos invalidos"},
  /* T_UNKNOWN_ERR      */ {"Erro desconhecido",        "Unknown error",            "Error desconocido"},
  /* T_SET_ICAO         */ {"Defina o ICAO na web",     "Set the ICAO in the web UI", "Defina el ICAO en la web"},
  /* T_NO_METAR         */ {"Sem METAR p/ %s",          "No METAR for %s",          "Sin METAR de %s"},
  /* T_CALM             */ {"calmo",                    "calm",                     "calma"},
  /* T_NO_CLOUDS        */ {"sem nuvens",               "no clouds",                "sin nubes"},
  /* T_NOW              */ {"agora",                    "now",                      "ahora"},
  /* T_FEELS            */ {"Sens %d  Max %d  Min %d",  "Feels %d  Max %d  Min %d", "Sens %d  Max %d  Min %d"},
  /* T_HUMIDITY         */ {"UMIDADE",                  "HUMIDITY",                 "HUMEDAD"},
  /* T_WIND             */ {"VENTO",                    "WIND",                     "VIENTO"},
  /* T_RAIN             */ {"CHUVA",                    "RAIN",                     "LLUVIA"},
  /* T_PRESSURE         */ {"PRESSAO",                  "PRESSURE",                 "PRESION"},
  /* T_NEXT_DAYS        */ {"PROXIMOS DIAS",            "NEXT DAYS",                "PROXIMOS DIAS"},
  /* T_NEXT_HOURS       */ {"PROXIMAS HORAS",           "NEXT HOURS",               "PROXIMAS HORAS"},
  /* T_TODAY            */ {"Hoje",                     "Today",                    "Hoy"},
  /* T_SUNRISE          */ {"Nasce %s  Poe %s  UV %d",  "Rise %s  Set %s  UV %d",   "Sale %s  Pone %s  UV %d"},
  /* T_BARS_RAIN        */ {"barras: chance de chuva",  "bars: rain chance",        "barras: prob. de lluvia"},
  /* T_VISIB            */ {"VISIB.",                   "VIS.",                     "VISIB."},
  /* T_CEIL             */ {"TETO",                     "CEILING",                  "TECHO"},
  /* T_TEMPDEW          */ {"TEMP/ORV",                 "TEMP/DEW",                 "TEMP/ROCIO"},
  /* T_WX               */ {"TEMPO",                    "WX",                       "TIEMPO"},
  /* T_FETCHED          */ {"buscado %d min",           "fetched %d min ago",       "obtenido hace %d min"},
  /* T_FIN_TITLE        */ {"FINANCAS",                 "FINANCE",                  "FINANZAS"},
  /* T_FIN_USD          */ {"DOLAR (USD)",              "US DOLLAR (USD)",          "DOLAR (USD)"},
  /* T_FIN_EUR         */ {"EURO (EUR)",               "EURO (EUR)",               "EURO (EUR)"},
  /* T_FIN_LOCAL       */ {"moeda local",              "local currency",           "moneda local"},
  /* T_FIN_DERIV_S     */ {"derivado do bitcoin",      "derived from bitcoin",     "derivado del bitcoin"},
  /* T_FIN_REFS        */ {"ref. BCE %s",              "ECB ref. %s",              "ref. BCE %s"},
  /* T_FIN_NO_PRICE     */ {"Sem preco %s",             "No price %s",              "Sin precio %s"},
  /* T_FIN_BAD_PRICE    */ {"Preco invalido",           "Invalid price",            "Precio invalido"},
  /* T_FIN_SOURCES      */ {"Coinbase / BCE (Frankfurter)", "Coinbase / ECB (Frankfurter)", "Coinbase / BCE (Frankfurter)"},
  /* T_FL_HDR           */ {"VOOS %d/%d  %dnm",         "FLIGHTS %d/%d  %dnm",      "VUELOS %d/%d  %dnm"},
  /* T_FL_HDR0          */ {"VOOS 0  %dnm",             "FLIGHTS 0  %dnm",          "VUELOS 0  %dnm"},
  /* T_FL_NONE          */ {"Nenhum aviao",             "No aircraft",              "Ningun avion"},
  /* T_FL_SET_POS       */ {"Defina a posicao",         "Set the position",         "Defina la posicion"},
  /* T_FL_NO_AC         */ {"Sem aeronaves",            "No aircraft",              "Sin aeronaves"},
  /* T_FL_ON_GROUND     */ {"no solo",                  "on ground",                "en tierra"},
  /* T_FL_CLOSE         */ {"ICAO %s  clique: fechar",  "ICAO %s  click: close",    "ICAO %s  clic: cerrar"},
  /* T_FL_ALT           */ {"ALT",                      "ALT",                      "ALT"},
  /* T_FL_SPD           */ {"VEL",                      "SPD",                      "VEL"},
  /* T_FL_HDG           */ {"RUMO",                     "HDG",                      "RUMBO"},
  /* T_FL_VS            */ {"V/S",                      "V/S",                      "V/S"},
  /* T_FL_DIST          */ {"DIST",                     "DIST",                     "DIST"},
  /* T_NET_TITLE        */ {"REDE",                     "NETWORK",                  "RED"},
  /* T_SVC_TITLE        */ {"SERVICOS",                 "SERVICES",                 "SERVICIOS"},
  /* T_NET_CHANNEL      */ {"Canal",                    "Channel",                  "Canal"},
  /* T_NET_TIME         */ {"Hora",                     "Time",                     "Hora"},
  /* T_NET_UPTIME       */ {"Ligado",                   "Uptime",                   "Activo"},
  /* T_NET_MEMORY       */ {"Memoria",                  "Memory",                   "Memoria"},
  /* T_NET_RESET        */ {"Ultimo reset: %.26s",      "Last reset: %.26s",        "Ultimo reset: %.26s"},
  /* T_NET_NEVER        */ {"nunca",                    "never",                    "nunca"},
  /* T_NET_DISCONNECTED */ {"desconectado",             "disconnected",             "desconectado"},
  /* T_NET_NO_NTP       */ {"sem NTP",                  "no NTP",                   "sin NTP"},
  /* T_SVC_FLIGHTS      */ {"Voos",                     "Flights",                  "Vuelos"},
  /* T_SVC_MAP          */ {"Mapa",                     "Map",                      "Mapa"},
  /* T_SVC_WEATHER      */ {"Clima",                    "Weather",                  "Clima"},
  /* T_SVC_FINANCE      */ {"Financas",                 "Finance",                  "Finanzas"},
  /* T_RADAR_NO_WIFI    */ {"Radar: sem Wi-Fi",         "Radar: no Wi-Fi",          "Radar: sin Wi-Fi"},
  /* T_RADAR_NO_TIME    */ {"Radar: sem hora",          "Radar: no time",           "Radar: sin hora"},
  /* T_RADAR_NO_MEM     */ {"Radar: sem memoria",       "Radar: out of memory",     "Radar: sin memoria"},
  /* T_RADAR_BUSY       */ {"Radar: rede ocupada",      "Radar: network busy",      "Radar: red ocupada"},
  /* T_RADAR_MAP        */ {"Radar: mapa %d/%d",        "Radar: map %d/%d",         "Radar: mapa %d/%d"},
  /* T_RADAR_RAIN       */ {"Radar: chuva",             "Radar: rain",              "Radar: lluvia"},
  /* T_RADAR_RAIN_N     */ {"Radar: chuva %d/%d",       "Radar: rain %d/%d",        "Radar: lluvia %d/%d"},
  /* T_RADAR_FAIL       */ {"Radar: falha na rede",     "Radar: network failure",   "Radar: fallo de red"},
  /* T_RADAR_PARTIAL    */ {"parcial",                  "partial",                  "parcial"},
  /* T_RADAR_NO_RAIN    */ {"sem chuva (rede)",         "no rain (network)",        "sin lluvia (red)"},
  /* T_RADAR_TITLE_T    */ {"CHUVA %02d:%02d  z%d",     "RAIN %02d:%02d  z%d",      "LLUVIA %02d:%02d  z%d"},
  /* T_RADAR_TITLE      */ {"RADAR DE CHUVA",           "RAIN RADAR",               "RADAR DE LLUVIA"},
  /* T_RADAR_LOADING    */ {"Carregando radar...",      "Loading radar...",         "Cargando radar..."},
  /* T_RADAR_ZOOM       */ {"clique: zoom",             "click: zoom",              "clic: zoom"},
  /* T_MAP_URL          */ {"Mapa: URL longa",          "Map: URL too long",        "Mapa: URL larga"},
  /* T_MAP_NET          */ {"Mapa: rede (%d)",          "Map: network (%d)",        "Mapa: red (%d)"},
  /* T_MAP_HTTP         */ {"Mapa: HTTP %d",            "Map: HTTP %d",             "Mapa: HTTP %d"},
  /* T_MAP_TILE_BAD     */ {"Mapa: tile invalido",      "Map: bad tile",            "Mapa: tile invalido"},
  /* T_MAP_NO_WIFI      */ {"Mapa: sem Wi-Fi",          "Map: no Wi-Fi",            "Mapa: sin Wi-Fi"},
  /* T_MAP_NO_TIME      */ {"Mapa: sem hora",           "Map: no time",             "Mapa: sin hora"},
  /* T_MAP_NO_MEM       */ {"Mapa: sem memoria",        "Map: out of memory",       "Mapa: sin memoria"},
  /* T_MAP_PNG          */ {"Mapa: PNG invalido",       "Map: bad PNG",             "Mapa: PNG invalido"},
  /* T_MAP_TILE_N       */ {"Mapa: tile %d/%d",         "Map: tile %d/%d",          "Mapa: tile %d/%d"},
  /* T_MAP_PARTIAL      */ {"Mapa parcial",             "Partial map",              "Mapa parcial"},
};

static_assert(sizeof(T) / sizeof(T[0]) == T_COUNT, "I18n table and TrId enum are out of step");

static inline int lang() { return uiLang < LANG_TOTAL ? uiLang : LANG_PT; }

const char *tr(TrId id) { return id < T_COUNT ? T[id][lang()] : ""; }

//                       pt                         en                          es
static const char *const WX[][LANG_TOTAL] =
{
  /*  0 */ {"Ceu limpo",            "Clear sky",           "Despejado"},
  /*  1 */ {"Quase limpo",          "Mostly clear",        "Poco nuboso"},
  /*  2 */ {"Parcialmente nublado", "Partly cloudy",       "Parcialmente nuboso"},
  /*  3 */ {"Nublado",              "Overcast",            "Nublado"},
  /*  4 */ {"Neblina",              "Fog",                 "Niebla"},
  /*  5 */ {"Neblina gelada",       "Freezing fog",        "Niebla helada"},
  /*  6 */ {"Garoa fraca",          "Light drizzle",       "Llovizna debil"},
  /*  7 */ {"Garoa",                "Drizzle",             "Llovizna"},
  /*  8 */ {"Garoa forte",          "Heavy drizzle",       "Llovizna fuerte"},
  /*  9 */ {"Garoa gelada",         "Freezing drizzle",    "Llovizna helada"},
  /* 10 */ {"Chuva fraca",          "Light rain",          "Lluvia debil"},
  /* 11 */ {"Chuva",                "Rain",                "Lluvia"},
  /* 12 */ {"Chuva forte",          "Heavy rain",          "Lluvia fuerte"},
  /* 13 */ {"Chuva gelada",         "Freezing rain",       "Lluvia helada"},
  /* 14 */ {"Neve fraca",           "Light snow",          "Nieve debil"},
  /* 15 */ {"Neve",                 "Snow",                "Nieve"},
  /* 16 */ {"Neve forte",           "Heavy snow",          "Nieve fuerte"},
  /* 17 */ {"Graos de neve",        "Snow grains",         "Granos de nieve"},
  /* 18 */ {"Pancadas fracas",      "Light showers",       "Chubascos debiles"},
  /* 19 */ {"Pancadas de chuva",    "Rain showers",        "Chubascos"},
  /* 20 */ {"Pancadas fortes",      "Heavy showers",       "Chubascos fuertes"},
  /* 21 */ {"Pancadas de neve",     "Snow showers",        "Chubascos de nieve"},
  /* 22 */ {"Trovoadas",            "Thunderstorm",        "Tormenta"},
  /* 23 */ {"Trovoada e granizo",   "Thunderstorm, hail",  "Tormenta y granizo"},
  /* 24 */ {"Indefinido",           "Unknown",             "Desconocido"},
};

const char *trWx(int code)
{
  int i;
  switch(code)
  {
    case 0: i = 0; break;   case 1: i = 1; break;   case 2: i = 2; break;   case 3: i = 3; break;
    case 45: i = 4; break;  case 48: i = 5; break;  case 51: i = 6; break;  case 53: i = 7; break;
    case 55: i = 8; break;  case 56: case 57: i = 9; break;
    case 61: i = 10; break; case 63: i = 11; break; case 65: i = 12; break; case 66: case 67: i = 13; break;
    case 71: i = 14; break; case 73: i = 15; break; case 75: i = 16; break; case 77: i = 17; break;
    case 80: i = 18; break; case 81: i = 19; break; case 82: i = 20; break; case 85: case 86: i = 21; break;
    case 95: i = 22; break; case 96: case 99: i = 23; break;
    default: i = 24; break;
  }
  return WX[i][lang()];
}

const char *trDay(int dow)
{
  static const char *const D[7][LANG_TOTAL] =
  {
    {"Dom", "Sun", "Dom"}, {"Seg", "Mon", "Lun"}, {"Ter", "Tue", "Mar"}, {"Qua", "Wed", "Mie"},
    {"Qui", "Thu", "Jue"}, {"Sex", "Fri", "Vie"}, {"Sab", "Sat", "Sab"},
  };
  return (dow >= 0 && dow < 7) ? D[dow][lang()] : "---";
}
