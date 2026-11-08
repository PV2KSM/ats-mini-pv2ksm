# ATS-Mini v2.45 — PV2KSM r30 (community mod)

**Unofficial community mod by PV2KSM (PY2/PV2KSM), free, no warranty.** Based on the open-source [ESP32-SI4732 / ATS-Mini](https://github.com/esp32-si4732/ats-mini) firmware v2.45 (MIT, © 2025 ESP32-SI4732 Radio — see `LICENSE` and `NOTICE.txt`). Not affiliated with or endorsed by the original project.

## ⚠️ Tested hardware — read first
| Variant | Status |
|---|---|
| **ATS-Mini, OSPI** (ESP32-S3, 8 MB octal PSRAM, Si4732) | ✅ **Tested** by PV2KSM on a real radio (firmware build "Oct 28 2026", Wi-Fi + web UI + radio screens below) |
| ATS-Mini, QSPI | ⚠️ Compiled only — **not tested on hardware** |
| LilyGO T-Embed (Si473x) | ⚠️ Compiled only — **not tested on hardware** |

Flash **only the file for your own variant** (the web updater refuses the wrong one). Please report results for QSPI / LilyGO in *Issues*.

What was checked on the OSPI radio: web UI pages (Control, Memory, Finance, Backup, Config, Update, Patches), the **Analog** layout (live mirror on the Control tab), and the Weather (now / days / hours / METAR / rain radar), Flights (list + detail) and Finance screens.
**r30 (languages, euro, translated web pages) was tested by PV2KSM on the OSPI radio: OK.** Not yet confirmed on real hardware: the **Cockpit** and **Avionics** layouts (only checked as PC renders), full-flash "merged" images, backup *import*.
Known: with a weak battery the radio can reboot while Wi-Fi is on (charge it or turn Wi-Fi off). A user reported ~170 mA draw with Wi-Fi enabled; the reboot depends on the hardware, worse with a weak 3V3 regulator.
Known: the flights service occasionally reported `TLS erro (-1)` on the Services screen (retries work).

## What's new
- **r30: languages** — English (default on a fresh install), Portuguese and Spanish on the Flights, Weather, Finance, Network, METAR and rain-radar screens and on the whole web interface. Change it in **Settings → Language** (radio) or **Config → Language** (browser). Full manuals in EN/PT/ES: [docs/pv2ksm](https://github.com/PV2KSM/ats-mini-pv2ksm/tree/pv2ksm-r30/docs/pv2ksm).
- **r30: Finance** now shows US dollar, **euro** and Bitcoin.
- **UI layouts** (Settings → UI Layout / Theme): **Cockpit** (airliner radio panel, ACTIVE/STANDBY), **Analog** (tuning scale with red pointer + analog S/SNR dials), **Avionics** (GA-radio look, red 7-segment LED).
- **Flights**: adsb.lol tracker with map background (CARTO/OSM, your own key) and nearby airports.
- **Weather**: now, next days, next hours, **METAR**, **rain radar** (Open-Meteo, aviationweather.gov, RainViewer). Fetched only while the Weather screen is open.
- **Finance**: USD, EUR and Bitcoin in your local currency (Coinbase / Frankfurter-ECB).
- **Web UI**: live radio screen on the *Control* tab, Backup export/import, Location (ICAO or lat/lon), Finance, Status with "you are tuned to" + RDS.
- Stability: network tasks on core 1, Wi-Fi power-save off, "Last reset" diagnostic.

## Screenshots
**On the radio (OSPI)**

| Weather now | Next days | Next hours |
|---|---|---|
| ![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/radio-weather-now.jpg) | ![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/radio-weather-days.jpg) | ![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/radio-weather-hours.jpg) |

| METAR | Rain radar | Finance |
|---|---|---|
| ![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/radio-metar.jpg) | ![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/radio-rain-radar.jpg) | ![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/radio-finance.jpg) |

| Flights | Flight detail |
|---|---|
| ![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/radio-flights.jpg) | ![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/radio-flight-detail.jpg) |

**Web interface**

| Control (live screen) | Memory | Config |
|---|---|---|
| <img src="https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/web-control.jpg" width="220"> | <img src="https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/web-memory.jpg" width="220"> | <img src="https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/web-config.jpg" width="220"> |

| Finance | Backup | Update | Patches |
|---|---|---|---|
| <img src="https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/web-finance.jpg" width="170"> | <img src="https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/web-backup.jpg" width="170"> | <img src="https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/web-update.jpg" width="170"> | <img src="https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30/docs/pv2ksm/screenshots/web-patches.jpg" width="170"> |

## Files
| File | Use |
|---|---|
| `*-ota.bin` | Browser update (Update → Manual upload) |
| `*.zip` | Variant package: OTA + full merged image (flash at 0x0) + bootloader/partitions + SHA256 |
| `…-source.zip` | Source of this version (also on branch `pv2ksm-r28`) |

## How to update
1. **Back up** first (web UI → Backup → export; leave "include Wi-Fi/keys" **off** if you will share the file).
2. Open the radio's IP in a browser → *Update* → *Manual upload* → pick the `*-ota.bin` of **your** variant.
3. Wait for the reboot; don't power off while writing.

## Limits & licences
- Open-Meteo is free for **non-commercial use only**; RainViewer free API is for personal/educational use; adsb.lol, NOAA, CARTO/OSM, Coinbase and Frankfurter keep their own terms. Don't use this firmware in a commercial product without checking them (`AVISOS-DE-TERCEIROS.md`).
- No Wi-Fi credentials or API keys are included in the binaries or source; your map key and Wi-Fi stay in your radio's memory.
- When reporting a problem, include the **Last reset** line from the Status page and any on-screen message.

Credits: ESP32-SI4732 / ATS-Mini (MIT) · B612 font (SIL OFL 1.1) · LovyanGFX · data: adsb.lol, OpenStreetMap/CARTO, Open-Meteo, NOAA, RainViewer, Coinbase, Frankfurter.

---

# Resumo em português
**Mod não oficial da comunidade, por PV2KSM — gratuito e sem garantia.** Base: ATS-Mini v2.45 (MIT).

**⚠️ Testado somente no ATS-Mini variante OSPI.** As variantes QSPI e LilyGO T-Embed foram **só compiladas, sem teste em hardware**. Grave apenas o arquivo da sua variante e conte o resultado nos *Issues*.

**Testado no rádio OSPI:** páginas da web, layout Analog, telas de Clima (agora/dias/horas/METAR/radar), Voos e Finanças. **Ainda não confirmados em hardware:** layouts Cockpit e Avionics, imagem `merged`, importação de backup.

**Novidades:** layouts Cockpit, Analog e Avionics; rastreador de voos com mapa; clima, METAR e radar de chuva; dólar e bitcoin; tela do rádio ao vivo na aba *Control*; backup; estabilidade.

**Como atualizar:** faça backup → abra o IP do rádio → *Update* → *Manual upload* → escolha o `*-ota.bin` da sua variante.
