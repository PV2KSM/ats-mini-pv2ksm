# ATS-Mini v2.45 — PV2KSM r30C (community mod)

**Unofficial community mod by PV2KSM, free, no warranty.** Based on the open-source [ESP32-SI4732 / ATS-Mini](https://github.com/esp32-si4732/ats-mini) firmware v2.45 (MIT, see `LICENSE` and `NOTICE.txt`). Not affiliated with or endorsed by the original project.

## Tested hardware — read first
| Variant | Status |
|---|---|
| **ATS-Mini, OSPI** | Tested by PV2KSM on a real radio (News, QR code, layouts, languages) |
| ATS-Mini, QSPI | Compiled only — **not tested on hardware** |
| LilyGO T-Embed (Si473x) | Compiled only — **not tested on hardware** |

Flash **only the file for your own variant**. Please report QSPI / LilyGO results in *Issues*.
Not confirmed on hardware: every layout of the issue #5 fixes (checked as PC renders and on the OSPI radio), News with custom feeds or editions other than Brazil, and the English/Spanish versions of the new texts.

## What's new in r30C

### 1. News screen
Menu → **News**. Headlines from **Google News** for the country you choose (or from your own RSS/Atom feed), shown in a TV-style screen with an animated search while the headlines load.

| Searching | Headline | QR code |
|---|---|---|
| ![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30C/docs/pv2ksm/screenshots/news-search.png) | ![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30C/docs/pv2ksm/screenshots/news-item.png) | ![](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30C/docs/pv2ksm/screenshots/news-qr.png) |

*Images are PC renders of the screen design (the real radio uses your colour theme).*

- **Rotate** the encoder to move between headlines.
- **Short press** (hold the button briefly, about half a second) opens a **QR code window**; a quick click closes it. A very long press turns the display off, as always.
- **Nothing runs in the background**: headlines are fetched only while the News screen is open, and the QR code is generated only when you open it.
- Choose the country/edition (or paste your own feed address) on the web **News** page or in the radio menu.

### 2. Reading the article on your phone — the QR code can be hard to scan
Google News links are long, and a long address makes a **dense QR code** on a small 320×170 screen. Tips:
- **Use the phone camera in macro / close-up mode** and hold it a little closer. This worked on an iPhone 17 Pro and a Motorola Edge 50 Fusion. On an entry-level phone (Moto E13) it also worked, but took longer.
- If it still does not read, **open the article from the web page instead**: the radio's web interface has a **News** page that lists the current headlines with **clickable links** (and the length of each address). Tap a headline to open the article. This always works, whatever the phone.
- Addresses up to 1000 characters are supported; the web list shows each length.

### 3. Fixes from issue #5 (thanks, Dave)
- Station and EiBi names **scroll inside their field** instead of overlapping other information (Cockpit, Cessna, Analog and the new layouts).
- The **SQL / MUTE chips** and the Cockpit **STBY** label no longer collide with the Wi-Fi/battery banner or with each other.
- The weather summary on the web **Status** page follows the page language.

### 4. Language review (PT / EN / ES)
The original menu, option values, pop-ups, status lines, About screen, RDS program types, the Cockpit weather card, dates and the web page messages are now available in Portuguese, English and Spanish. Change it in **Settings → Language** (radio) or **Config → Language** (browser).

## Notes
- Google News feeds use an unofficial URL format; the publishers' terms apply. No key needed.
- Weather: Open-Meteo (non-commercial). Rain radar: RainViewer (personal/educational).
- Free firmware: no donations, no sponsorship.

## Files
| File | Use |
|---|---|
| `ats-mini-v2.45-ospi-r30C-ota.bin` | ATS-Mini OSPI — browser update (Update → Manual upload) |
| `ats-mini-v2.45-qspi-r30C-ota.bin` | ATS-Mini QSPI (not hardware-tested) |
| `ats-mini-v2.45-lilygo-t-embed-r30C-ota.bin` | LilyGO T-Embed (not hardware-tested) |
