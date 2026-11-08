#ifndef BLE_MODE_H
#define BLE_MODE_H

#include "Remote.h"
#include "BleUartPeripheral.h"

void bleInit(uint8_t bleMode);
void bleBootInit();   // Start-up: turns Bluetooth off if the last boot crashed
void bleStop();
int8_t getBleStatus();
bool bleLoop(uint8_t bleMode);
// True from a held rotation until release, disconnect, or cancellation.
bool blePushAndRotate();
bool bleConsumeAbortPending(uint8_t bleMode);

#endif
