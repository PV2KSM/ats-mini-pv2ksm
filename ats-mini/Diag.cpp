#include "Diag.h"
#include <Arduino.h>
#include <esp_system.h>
#include <esp_attr.h>
#include <esp_heap_caps.h>

#define DIAG_MAGIC 0xD1A60001u

static RTC_NOINIT_ATTR uint32_t rtcMagic;
static RTC_NOINIT_ATTR uint8_t rtcStage, rtcAux;
static RTC_NOINIT_ATTR uint32_t rtcHeapFree, rtcHeapBlock, rtcUptimeS;

static const char *resetName = "?";
static uint8_t prevStage = 0, prevAux = 0;
static uint32_t prevHeapFree = 0, prevHeapBlock = 0, prevUptime = 0;
static bool prevValid = false;
static SemaphoreHandle_t netMtx = nullptr;

static const char *stageName(uint8_t s)
{
  switch(s)
  {
    case DG_MAP_START:      return "mapa: inicio";
    case DG_MAP_FETCH:      return "mapa: baixando tile";
    case DG_MAP_DECODE:     return "mapa: decodificando PNG";
    case DG_MAP_RESAMPLE:   return "mapa: montando imagem";
    case DG_MAP_PUBLISH:    return "mapa: publicando";
    case DG_MAP_DONE:       return "mapa: pronto";
    case DG_FL_FETCH:       return "voos: baixando";
    case DG_FL_PARSE:       return "voos: lendo JSON";
    case DG_FL_DRAW:        return "voos: desenho";
    case DG_FL_DRAW_MAP:    return "voos: desenho do mapa";
    case DG_FL_DRAW_PLANES: return "voos: desenho dos avioes";
    case DG_FL_DRAW_PANEL:  return "voos: desenho dos paineis";
    case DG_WX_FETCH:       return "clima: baixando";
    case DG_WX_PARSE:       return "clima: lendo JSON";
    default:                return "nenhum";
  }
}

void diagBegin()
{
  if(!netMtx) netMtx = xSemaphoreCreateMutex();
  switch(esp_reset_reason())
  {
    case ESP_RST_POWERON:   resetName = "ligado"; break;
    case ESP_RST_SW:        resetName = "software"; break;
    case ESP_RST_PANIC:     resetName = "PANIC (erro de programa)"; break;
    case ESP_RST_INT_WDT:   resetName = "watchdog de interrupcao"; break;
    case ESP_RST_TASK_WDT:  resetName = "watchdog de tarefa"; break;
    case ESP_RST_WDT:       resetName = "watchdog"; break;
    case ESP_RST_BROWNOUT:  resetName = "queda de tensao (brownout)"; break;
    case ESP_RST_DEEPSLEEP: resetName = "sono profundo"; break;
    default:                resetName = "outro"; break;
  }
  if(rtcMagic == DIAG_MAGIC)
  {
    prevValid = true;
    prevStage = rtcStage; prevAux = rtcAux;
    prevHeapFree = rtcHeapFree; prevHeapBlock = rtcHeapBlock; prevUptime = rtcUptimeS;
  }
  rtcMagic = DIAG_MAGIC;
  rtcStage = DG_NONE; rtcAux = 0; rtcHeapFree = 0; rtcHeapBlock = 0; rtcUptimeS = 0;
}

void diagStage(uint8_t stage, uint8_t aux)
{
  rtcStage = stage;
  rtcAux = aux;
  rtcHeapFree = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  rtcHeapBlock = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  rtcUptimeS = millis() / 1000;
}

void diagText(char *out, size_t size)
{
  if(prevValid && prevStage)
    snprintf(out, size, "%s; ultima etapa: %s (%u); heap livre %u KB, maior bloco %u KB; ligado %us",
             resetName, stageName(prevStage), (unsigned)prevAux, (unsigned)(prevHeapFree / 1024), (unsigned)(prevHeapBlock / 1024), (unsigned)prevUptime);
  else
    snprintf(out, size, "%s", resetName);
}

bool netLock(uint32_t timeoutMs)
{
  if(!netMtx) return true;
  return xSemaphoreTake(netMtx, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void netUnlock()
{
  if(netMtx) xSemaphoreGive(netMtx);
}
