# ATS-Mini v2.45 — PV2KSM r30D (community mod)

**Unofficial community mod by PV2KSM, free, no warranty.** Based on the open-source [ESP32-SI4732 / ATS-Mini](https://github.com/esp32-si4732/ats-mini) firmware v2.45 (MIT, see `LICENSE` and `NOTICE.txt`). Not affiliated with or endorsed by the original project.

## Tested hardware — read first
| Variant | Status |
|---|---|
| ATS-Mini, OSPI | Built by PV2KSM; r30D screens are PC renders, real-radio test in progress |
| ATS-Mini, QSPI | Compiled only — **not tested on hardware** |
| LilyGO T-Embed (Si473x) | Compiled only — **not tested on hardware** |

Please report results in *Issues*. Not confirmed on hardware: the radar styles, city geocoding, the new weather sky, accented News text and the English/Spanish versions of the new texts.

## What's new in r30D

### 1. Flights radar styles
Besides the map, four cockpit-inspired looks: **A32X**, **ATCAS**, **BTCAS**, **B7X**, with traffic colours by risk and labels placed so they do not overlap. Pick it on the web **Flights** page or on the radio at **Visual → Flights radar** (a live preview is shown behind the menu).

![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30D/docs/pv2ksm/screenshots/r30D-radar-estilos-v2b.png)
![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30D/docs/pv2ksm/screenshots/r30D-radar-estilos-v2b-denso.png)

### 2. Services reorganised
Web **Weather**, **Flights** and **News** pages, each with a "show on the radio" Yes/No. Temperature unit now lives on the Weather page; **Finance** moved under News and lets you choose the currency and three cards (USD, EUR, GBP, BTC, ETH). Weather uses a **city** (with optional country code); Flights uses an airport **ICAO or IATA** code.

![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30D/docs/pv2ksm/screenshots/r30D-nav-preview.png)

### 3. Realistic weather sky
Procedural clouds, sun, moon, stars, rain, snow, fog and lightning on the Weather screen.

![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30D/docs/pv2ksm/screenshots/r30D-clima-novo-realista.png)

### 4. News with accents, layouts
News headlines use a smooth font with accented letters. NAnalog and Vintage layouts share the same frame.

![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30D/docs/pv2ksm/screenshots/r30D-noticias-acentos.png)
![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30D/docs/pv2ksm/screenshots/r30D-layouts-nanalog-vintage-antes-depois.png)

### Fixes
- Saving the web Config page no longer reboots the radio (larger web server task stack).
- Full PT/EN/ES review of every new text.
