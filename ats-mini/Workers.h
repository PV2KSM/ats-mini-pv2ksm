#pragma once
// Background workers. Only one service screen is open at a time, so the services share two tasks instead of owning one each
// (saves ~60 KB of internal RAM, which Bluetooth and TLS need).
void workersBegin();
void financeWork(); void newsWork(); void weatherWork(); void flightsWork(); void mapWork(); void radarWork();
