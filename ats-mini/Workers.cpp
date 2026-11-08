// Shared background workers for the service screens (Finance, News, Weather, Flights, DMR, Map, Radar).
//   Worker A (16 KB stack): Finance, News, Weather, Flights and the DMR feed (the DMR loop blocks while its screen is open).
//   Worker B (20 KB stack): Map and Radar tile builds (they never run together: different screens).
// Flights/Map and Weather/Radar can still work at the same time, since each pair is split over A and B.
#include "Workers.h"
#include "Dmr.h"
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <mbedtls/platform.h>

// TLS (HTTPS) buffers and certificate parsing take 40-50 KB, and the stock build puts all of it in internal RAM, which Wi-Fi, Bluetooth
// and the tasks already fill (mbedTLS error -32512). Put them in PSRAM instead; small blocks stay internal.
static void *tlsCalloc(size_t n, size_t size)
{
  void *p = nullptr;
  if(n * size >= 256) p = heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if(!p) p = heap_caps_calloc(n, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  return p;
}
static void tlsFree(void *p) { heap_caps_free(p); }

static void workerA(void *)
{
  for(;;)
  {
    financeWork();
    newsWork();
    weatherWork();
    flightsWork();
    dmrRun();                                  // DMR screen open: runs here until the screen is left
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

static void workerB(void *)
{
  for(;;)
  {
    mapWork();
    radarWork();
    vTaskDelay(pdMS_TO_TICKS(400));
  }
}

void workersBegin()
{
  if(heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0) mbedtls_platform_set_calloc_free(tlsCalloc, tlsFree);
  xTaskCreatePinnedToCore(workerA, "svcA", 16384, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(workerB, "svcB", 20480, nullptr, 1, nullptr, 1);
}
