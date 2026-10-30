#pragma once
#include <Arduino.h>

// Whole-radio backup: every saved setting, memory, band, position and map option as one JSON file.
// Wi-Fi networks, the web login and the map key are secrets and only included on request.
bool backupExport(String &out, bool includeSecrets);
int backupImport(const char *json, size_t len, char *msg, size_t msgSize);   // Items written, or -1 on error (msg says why)
void backupRestartSoon();                                                    // Restart in about 2 seconds
void backupTick();                                                           // Main loop: performs the restart
