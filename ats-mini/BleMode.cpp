#include "Common.h"
#include "Themes.h"
#include "Remote.h"
#include "Draw.h"
#include "Utils.h"
#include "Storage.h"
#include "BleHidCentral.h"
#include "BleMode.h"
#include <host/ble_store.h>
#include <Preferences.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include "Diag.h"
#include "I18n.h"

static BleUartPeripheral BLESerial;
static BleHidCentral BLEHid;
static RemoteState remoteBLEState;
static bool hidPushAndRotate = false;

static void bleControllerDisable()
{
  if (BLEDevice::getInitialized() && esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED)
    esp_bt_controller_disable();
}

static bool bleControllerEnable()
{
  if (!BLEDevice::getInitialized()) return true;

  esp_bt_controller_status_t status = esp_bt_controller_get_status();
  if (status == ESP_BT_CONTROLLER_STATUS_ENABLED) return true;
  if (status == ESP_BT_CONTROLLER_STATUS_INITED)
    return esp_bt_controller_enable(ESP_BT_MODE_BLE) == ESP_OK;

  return false;
}

//
// Get current connection status
// (-1 - not connected, 0 - disabled, 1 - connected)
//
int8_t getBleStatus()
{
  if (BLESerial.isStarted())
    return BLESerial.isConnected() ? 1 : -1;

  if (BLEHid.isStarted())
    return BLEHid.isConnected() ? 1 : -1;

  return 0;
}

//
// Stop BLE hardware
//
// We do not deinit the BLE stack fully because it leads to crashes
// Also see: https://github.com/afpineda/NuS-NimBLE-Serial/issues/12#issuecomment-2912732754
//
void bleStop()
{
  hidPushAndRotate = false;
  if (BLESerial.isStarted())
    BLESerial.end();

  if (BLEHid.isStarted())
    BLEHid.end();

  // Experimental attempt to stop consuming power
  // https://github.com/esp32-si4732/ats-mini/discussions/324#discussioncomment-17123023
  bleControllerDisable();
}

#define BLE_MIN_FREE_K  75     // KB of free internal RAM needed to start Bluetooth (measured: ~93k with Wi-Fi off, ~63k with Wi-Fi on)

// Crash guard: a flag is set while Bluetooth starts. If the radio resets before it is cleared (or after a panic / watchdog
// reset with Bluetooth on), the next boot comes up with Bluetooth off instead of looping, so Wi-Fi + Bluetooth can never lock the radio.
static void bleGuard(uint8_t v)
{
  Preferences p; if(!p.begin("bleguard", false)) return;
  if(p.getUChar("g", 0) != v) p.putUChar("g", v);
  p.end();
}
static uint8_t bleGuardGet()
{
  Preferences p; if(!p.begin("bleguard", true)) return 0;
  const uint8_t v = p.getUChar("g", 0); p.end(); return v;
}

void bleBootInit()
{
  const esp_reset_reason_t rr = esp_reset_reason();
  const bool crashed = rr == ESP_RST_PANIC || rr == ESP_RST_INT_WDT || rr == ESP_RST_TASK_WDT || rr == ESP_RST_WDT;
  if(bleModeIdx != BLE_OFF && (bleGuardGet() == 1 || crashed))
  {
    bleGuard(0);
    bleModeIdx = BLE_OFF;
    prefsRequestSave(SAVE_SETTINGS, true);
    return;
  }
  bleGuard(0);
  bleInit(bleModeIdx);
}

void bleInit(uint8_t bleMode)
{
  bleStop();

  if (bleMode == BLE_UNPAIR_ALL)
  {
    // The BLE store is initialized lazily, so initialize it even when the
    // receiver was previously in Off mode.
    if (!BLEDevice::getInitialized())
      BLEDevice::init(RECEIVER_NAME);

    if (BLEDevice::getInitialized())
      ble_store_clear();

    bleControllerDisable();
    bleModeIdx = BLE_OFF;
    prefsRequestSave(SAVE_SETTINGS, true);
    return;
  }

  if (bleMode != BLE_OFF)
  {
    // Bluetooth needs a good chunk of internal RAM; with Wi-Fi and the services running it can run out and the chip resets.
    // Refuse to start instead, and say how much is free.
    diagStage(DG_BLE_INIT, bleMode);
    const size_t freeK = heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024, blockK = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024;
    // Once the stack is initialised its memory is already taken (it is never fully released), so the check only applies to the first start
    if(!BLEDevice::getInitialized() && freeK < BLE_MIN_FREE_K)
    {
      char l2[40]; snprintf(l2, sizeof(l2), "RAM %uk blk %uk (need %uk)", (unsigned)freeK, (unsigned)blockK, (unsigned)BLE_MIN_FREE_K);
      statusShow("Bluetooth off: turn Wi-Fi off", l2, 5000);
      bleModeIdx = BLE_OFF; prefsRequestSave(SAVE_SETTINGS, true);
      return;
    }
    bleGuard(1);
    diagStage(DG_BLE_BEGIN, bleMode);
  }
  if (bleMode != BLE_OFF && !bleControllerEnable()) { bleGuard(0); return; }

  switch(bleMode)
  {
    case BLE_ADHOC:
      BLESerial.begin(RECEIVER_NAME);
      break;
    case BLE_HID:
      BLEHid.begin(RECEIVER_NAME);
      break;
  }
  bleGuard(0);
}

static InputEvent bleReadInput(uint8_t& byte)
{
  return BLESerial.readInput(byte);
}

bool blePushAndRotate()
{
  return hidPushAndRotate;
}

bool bleLoop(uint8_t bleMode)
{
  if(bleMode != BLE_HID) hidPushAndRotate = false;
  if (bleMode == BLE_ADHOC)
    return remoteLoop(&BLESerial, &remoteBLEState, bleReadInput);

  if (bleMode != BLE_HID)
    return false;

  // BLEHid.loop() below connects synchronously, so the status stays up for
  // the whole connection attempt
  if (BLEHid.isConnectPending() && BLEHid.peerName())
  {
    statusShow("Connecting BLE HID", BLEHid.peerName(), 0);
    drawScreen();
    BLEHid.loop();
    statusShow(nullptr);
  }
  else
    BLEHid.loop();

  if (!BLEHid.isConnected())
  {
    hidPushAndRotate = false;
    return false;
  }

  BleHidState input = BLEHid.update();
  if(!input.isPressed) hidPushAndRotate = false;
  else if(input.rotation && !(sleepOn() && sleepModeIdx == SLEEP_LOCKED))
    hidPushAndRotate = true;
  if(input.rotation)
    return handleRotation(input.rotation, input.rotation, input.isPressed);
  if(input.wasClicked || input.wasShortPressed)
    return handleClick(input.wasShortPressed);
  return false;
}

bool bleConsumeAbortPending(uint8_t bleMode)
{
  if (bleMode == BLE_ADHOC)
    return remoteConsumeAbortPending(&remoteBLEState, bleReadInput);

  if (bleMode == BLE_HID && BLEHid.consumeAbortPending())
  {
    hidPushAndRotate = false;
    return true;
  }

  return false;
}
