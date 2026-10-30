# PV2KSM mod r28 — menu guide

Unofficial community mod based on ATS-Mini v2.45 (MIT). Only the **OSPI** variant is tested on real hardware. Anything not described here works as in the [official manual](../source/manual.md).

## Controls (encoder)
| Action | What it does |
|---|---|
| Rotate | Tune, move in menus, change values |
| Click | Open the menu / enter an item / go back |
| Short press (hold ~0.5 s) | Context action (see each screen below) |
| Long press (>2 s) | Sleep on/off |

## Main menu
Mode · Band · Volume · VFO A/B · Step · Seek · Scan · Memory · Squelch · Bandwidth · AGC/ATTN · AVC · SoftMute · **Flights** · **Weather** · **Finance** · **Network** · Settings

The items in **bold** are new in this mod; the others are the original ATS-Mini items.

### Flights
Live aircraft around your position (adsb.lol), drawn on a map with nearby airports.
- Rotate: select the next/previous aircraft.
- Short press: open/close the details panel (callsign, registration, type, altitude, speed, heading, vertical speed, distance).
- Click: leave the screen.
- Colours show altitude: orange < 5 000 ft, yellow < 15 000 ft, green < 30 000 ft, blue above.
- Set your position in the web UI → **Location** (airport ICAO, or latitude/longitude).
- Limitation: in dense areas (many aircraft) it is crowded and updates can be slow or fail.

### Weather
Rotate to change page (5 pages): **now → next days → next hours → METAR → rain radar**.
- On the radar page, short press zooms in/out.
- Click: leave the screen.
- Data is fetched only while this screen is open. Needs Wi-Fi and correct time.
- METAR uses the ICAO set in the web UI → Location.

### Finance
US dollar and Bitcoin converted to your local currency (choose it in the web UI → **Finance**). Click to leave.

### Network
Two pages (rotate): **Network** (Wi-Fi name, IP, channel, MAC, time, uptime, memory, last reset) and **Services** (status of Flights, Map, Weather, METAR, Radar, Finance). Useful when something doesn't load.

### Settings (relevant items)
- **UI Layout**: Default, S-Meter, **Cockpit**, **Analog**, **Avionics**.
- **Theme**: includes **Analog** and **Avionics** themes that match the new layouts.
- **Wi-Fi**: Connect, Sync Only, Off (see the radio). Turning Wi-Fi off saves power; Off and Sync Only disable the web interface.
- **Update FW**, **DSP Patches**, **Date/Time**, **UTC Offset** (São Paulo, UTC-3, by default), and the rest as in the official manual.

## UI layouts
- **Cockpit** — airliner radio panel. ACTIVE = current frequency; STANDBY = the other VFO (VFO A/B).
- **Analog** — tuning scale with a red pointer on the selected station, analog S and SNR dials, RDS in the header.
- **Avionics** — general-aviation radio look, red 7-segment frequency, LED bars.

## Web interface (open the radio's IP in a browser)
| Tab | Use |
|---|---|
| Status | "You are tuned to" card, weather, system info |
| **Control** | Live radio screen, tuning buttons, go to frequency, band/mode, volume |
| Memory | Edit memory slots (names, band, frequency in Hz, mode) |
| **Location** | Airport ICAO or lat/lon, name and label, map key/URL |
| **Finance** | Local currency |
| **Backup** | Export/import settings. Leave "include Wi-Fi networks and keys" **off** if you share the file |
| Config | Date/time, UI layout, theme, scrolling |
| Wi-Fi | Mode and saved networks |
| Update | Check for updates / manual `.bin` upload |
| Patches | Experimental Si4732 patch sets |

## Map key (optional)
The map background needs your own free CARTO basemaps key. Get one at <https://carto.com/basemaps/apikey/> and enter it in the web UI → **Location** (the key is stored only in your radio and is never shown back).

## Known limitations
- Flights in dense areas don't work well.
- With a weak battery the radio can keep rebooting while Wi-Fi is on; charge it or turn Wi-Fi off.
- Only OSPI is tested; QSPI and LilyGO T-Embed are compiled only. Cockpit and Avionics need more real-hardware testing.
- Open-Meteo is free for non-commercial use only; other services keep their own terms (see `AVISOS-DE-TERCEIROS.md`).

## Plans
A **lite version** (lighter map, or no map option at all) is planned. No date yet.

Feedback and bug reports: https://github.com/PV2KSM/ats-mini-pv2ksm/issues — 73, PV2KSM
