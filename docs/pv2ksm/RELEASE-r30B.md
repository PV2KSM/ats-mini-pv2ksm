# ATS-Mini v2.45 — PV2KSM r30B (community mod) — new look, Scan screens, OpenStreetMap map

**Unofficial community mod by PV2KSM (PY2/PV2KSM), free, no warranty.** Based on the open-source [ESP32-SI4732 / ATS-Mini](https://github.com/esp32-si4732/ats-mini) firmware v2.45 (MIT, © 2025 ESP32-SI4732 Radio — see `LICENSE` and `NOTICE.txt`). Not affiliated with or endorsed by the original project.

> **r30B = r30A + fixes and polish.** The radio tunes, connects and translates as before. What is new is **how it looks and how you control it**: icon menus, new main-screen layouts, a dedicated **Scan screen for every layout**, a new web **Control** tab, the **rain radar in the new look** and the **OpenStreetMap** map out of the box (no key needed). **The original look is still the default** — you opt in. If you already run r30A, this release fixes the issues reported on it (see below).

## ⚠️ Tested hardware — read first
| Variant | Status |
|---|---|
| **ATS-Mini, OSPI** (ESP32-S3, 8 MB octal PSRAM, Si4732) | ⏳ **TO FILL IN by the author after testing on the radio** |
| ATS-Mini, QSPI | ⚠️ Compiled only — **not tested on hardware** |
| LilyGO T-Embed (Si473x) | ⚠️ Compiled only — **not tested on hardware** |

Flash **only the file for your own variant** (the web updater refuses the wrong one). Please report results for QSPI / LilyGO in *Issues*.
The pictures below are **renders made on a PC with the same drawing code as the firmware** (not photos of the radio). Real photos may differ slightly in colour.
The CPU stays at the original **80 MHz**; the new graphics were optimised for it (icons are cached, lighter anti-aliasing on small icons, calmer animation rate). How smooth it feels on your radio is the main thing to report.

## What's new (and what each thing does)

### Menu style — *Visual → Menu style*
| Option | What it does |
|---|---|
| **Original** (default) | The r30 look, unchanged: side list menus, original screens, original weather. |
| **Carousel** | Turns the **new look** on. Full-screen menus: one big round icon in the middle, neighbours on each side, name below. |
| **Grid** | Turns the **new look** on. Full-screen menus as a grid of 8 icon tiles per page, with the selected name in the top bar. |

With Carousel or Grid every menu and editor uses the same style, the status message (e.g. Wi-Fi/update) is a centred box, and **Finance, Network and Weather** are restyled (colours follow your choices below).

### New **Visual** menu (main menu, before Settings)
Everything about appearance in one place:
| Entry | What it does |
|---|---|
| **UI Layout** | Choose the main screen. The 5 original layouts plus 9 new ones (below). The screen is **previewed live behind the selector**. |
| **Theme** | The original colour themes (as in r30). Live preview behind the selector. |
| **Accent** | Main colour of the new screens: Amber, Green, Cyan, Blue, Magenta, Red, White. |
| **Text color** | 12 choices for numbers and letters (Accent, White, Cream, Amber, Orange, Yellow, Green, Mint, Cyan, Sky, Pink, Red). |
| **Background** | Graphite, Black, Navy, Wine, Forest. |
| **Menu color** | Colour of the menus: same as the accent or its own. |
| **Menu style** | Original / Carousel / Grid (see above). |

*Theme, UI Layout and the colour entries moved from **Settings** to **Visual**.*

### New UI layouts (use Accent / Text color / Background)
| Layout | What it is |
|---|---|
| **Neo Analog** | Cream tuning scale with a red pointer and two analog dials (signal and SNR). |
| **Neo S-Meter** | Big frequency, tuning scale and segmented signal / SNR bars, with Volume, Step, Bandwidth and AGC cards. |
| **Vintage** | Wood-panel receiver look with glass dial and tuning-eye. |
| **Clock** | Large digital clock, date, frequency and the weather at a glance. |
| **Memories** | The current band as a line with your saved memories as pins. |
| **Minimal** | One huge frequency and a thin signal line. |
| **VFD** | Green 7-segment vacuum-fluorescent display. |
| **LCD** | Classic 7-segment LCD. |
| **Neon** | Cyberpunk: cyan/magenta panels, chromatic-offset digits. |

### Language
The names and titles in the Carousel and Grid menus now follow **Settings → Language** (Portuguese, English, Spanish). Texts are ASCII (no accents), like the other translated screens. The web page selectors are translated too. Icons are drawings, so they are the same in every language.

### Web interface
*Config → UI*: new selectors **Accent color, Text color, Background, Menu color, Menu style** (the layout list shows the new layouts).

### What changed since r30A
- **Scan screen for every layout, each with its own model.** Because the UI layouts are so different from each other, the Scan screen is designed per layout instead of one generic graph. While scanning, the screen shows a large graph with the frequency, **RSSI**, **SNR**, the scale and the strongest peak. Each layout has its own look (background and colours): Neo Analog, Neo S-Meter, Vintage, Clock, Memories (station name), Minimal, VFD and LCD (segment bars), Neon, plus **Cockpit, Analog and Avionics**. The original *Default* and *S-Meter* layouts keep the classic Scan graph. *(Design sketches below; the colours on the radio may differ slightly.)*
- **Frequency digit cursor** (underline on the selected digit) in all the new layouts, and **MUTED / SQ** badges that no longer overlap other items.
- **Station names** no longer overlap (long names scroll).
- **Status messages** (e.g. "Loading EiBi Schedule") now sit on a box with a background in every layout.
- **Menus over the original layouts:** the side bar no longer overlaps the menu icons and the info box is no longer drawn in the middle of the screen (Carousel/Grid with Default, S-Meter, Cockpit, Analog and Avionics).
- **Web interface:**
  - **Control** tab redesigned: the radio screen stays fixed at the top, then tuning, **encoder buttons (Rotate ◀ ▶, Click, Short press)** as asked by G8PTN, volume, signal, go-to-frequency, band and mode. Texts in PT/EN/ES.
  - New texts are translated into Portuguese and Spanish.
- **Rain radar** is back as the last Weather page in the new look too (click the encoder to zoom).
- **Map:** **OpenStreetMap** is now the preconfigured map source (no key needed) and CARTO was removed. Attribution is shown on the map and in *About*. Existing CARTO settings are migrated.
- **Online update disabled** in this mod ("Mod: use Manual upload.") so that it can never install the original firmware over the mod.

### Known issue
- **Bluetooth "Ad hoc" mode can make the web interface stop responding** on some radios (reported by G8PTN). The cause is **not found yet**. Workarounds: use USB, or the Ad-hoc serial port, or switch Bluetooth off. If it happens to you, please send the **heap numbers from the Status page** with Bluetooth on and off.

### Fixes (issue #3, thanks G8PTN)
- Web UI **Location** page now shows the just-saved *Show map* / *Map style* / tile URL right after Save.
- **Flights** screen without a map now follows the *Background* setting instead of always black.

### Not in this release
No CPU-speed option (the radio stays at 80 MHz). Japanese/Chinese are not included yet.

## Screenshots (PC renders)
**Main-screen layouts**

![layouts](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30A/docs/pv2ksm/screenshots/r30A-layouts.png)

**Menus: Carousel, Grid, Visual menu, languages**

![menus](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30A/docs/pv2ksm/screenshots/r30A-menus.png)

**Colour options (Accent / Text color / Background)**

![colors](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30A/docs/pv2ksm/screenshots/r30A-colors.png)

**Weather (new look)**

![weather](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30A/docs/pv2ksm/screenshots/r30A-weather.png)

**Scan screens: one model per layout (design sketches made on a PC, not photos)** — first image: Neo Analog, Neo S-Meter, Vintage, Clock, Memories, Minimal, VFD, LCD, Neon; second part: Cockpit, Analog, Avionics

![scan](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30B/docs/pv2ksm/screenshots/r30B-scan.png)

**Web Control tab (design sketch)**

![control](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30B/docs/pv2ksm/screenshots/r30B-control.png)

**Rain radar (photo of the radio, Original look; r30B adds the same page to the new look)**

![rain radar](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30B/docs/pv2ksm/screenshots/radio-rain-radar.jpg)

**Finance and Network (new look)**

![finance and network](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30A/docs/pv2ksm/screenshots/r30A-finance-network.png)

## Files
| File | Use |
|---|---|
| `ats-mini-v2.45-ospi-r30B-ota.bin` | ATS-Mini OSPI — browser update (Update → Manual upload) |
| `ats-mini-v2.45-qspi-r30B-ota.bin` | ATS-Mini QSPI (compiled only) |
| `ats-mini-v2.45-lilygo-t-embed-r30B-ota.bin` | LilyGO T-Embed (compiled only) |
| `SHA256SUMS.txt` | Checksums |

## How to update
1. **Back up** first (web UI → Backup → export; leave "include Wi-Fi/keys" **off** if you will share the file).
> ⚠️ **Do not use the radio's online "Update → Install" for this mod.** It checks the *original* project's releases (not this one), so it says "Already up to date" and, if the original project publishes a newer version, it would install the original firmware over the mod. Always use **Manual upload**.

2. Open the radio's IP in a browser → *Update* → *Manual upload* → pick the `*-ota.bin` of **your** variant.
3. Wait for the reboot; don't power off while writing.
4. To try the new look: **Menu → Visual → Menu style → Carousel or Grid**. To go back: choose **Original**.

## Limits & licences
- **OpenStreetMap** tiles are a free community service with a usage policy (identifiable User-Agent, attribution, no heavy use). Access may be blocked without notice. You can set another tile source in *Location*.
- Open-Meteo is free for **non-commercial use only**; RainViewer free API is for personal/educational use; adsb.lol, NOAA, OpenStreetMap, Coinbase and Frankfurter keep their own terms (`AVISOS-DE-TERCEIROS.md`).
- No Wi-Fi credentials or API keys are included in the binaries or source.
- When reporting a problem, include the **Last reset** line from the Status page and any on-screen message.

Credits: ESP32-SI4732 / ATS-Mini (MIT) · B612 font (SIL OFL 1.1) · LovyanGFX · data: adsb.lol, OpenStreetMap, Open-Meteo, NOAA, RainViewer, Coinbase, Frankfurter.

---

# Resumo em português

**r30B = r30A + correções e acabamento.** O rádio sintoniza, conecta e traduz como antes. O que muda é **o visual e o controle**: menus de ícones, layouts novos, uma **tela de Scan própria para cada layout**, a nova aba web **Control**, o **radar de chuva no visual novo** e o mapa **OpenStreetMap** já configurado (sem chave). **O visual original continua sendo o padrão** — você escolhe se quer o novo.

**⚠️ Status do teste:** OSPI — *(preencher após testar)*. QSPI e LilyGO T-Embed: **só compiladas, sem teste em hardware**. Grave só o arquivo da sua variante. As imagens são **desenhos/renderizações feitas no PC** (não são fotos do rádio). A CPU continua em **80 MHz**.

**Novidades desde a r30A**
- **Scan para todos os layouts novos:** gráfico grande com frequência, **RSSI**, **SNR**, escala e o pico mais forte, com fundo e cores próprios em cada layout (Neo Analog, Neo S-Meter, Vintage, Clock, Memories, Minimal, VFD, LCD, Neon e também **Cockpit, Analog e Avionics**). Os layouts *Default* e *S-Meter* mantêm o gráfico clássico.
- **Cursor sublinhado no dígito** da frequência nos layouts novos; avisos **MUTED/SQ** sem sobreposição; nomes de estação longos rolam; mensagens de status com fundo em todos os layouts.
- **Menus sobre os layouts originais:** a barra lateral não cobre mais os ícones e a caixa de informação não aparece mais no meio da tela.
- **Página web:** aba **Control** reorganizada (tela do rádio fixa no topo, botões do **encoder**: girar ◀ ▶, clique e pressão curta, como pediu o G8PTN), textos novos em PT/EN/ES.
- **Radar de chuva** de volta como última página do Clima no visual novo (clique do encoder alterna o zoom).
- **Mapa:** **OpenStreetMap** como padrão (sem chave); CARTO removido; atribuição na tela e em *About*; configurações antigas do CARTO são migradas.
- **Atualização online desativada** neste mod (use **Manual upload**), para nunca instalar o firmware original por cima.
- **Problema conhecido:** o modo Bluetooth "Ad hoc" pode fazer a página web parar de responder em alguns rádios (relato do G8PTN). **A causa ainda não foi encontrada.** Alternativas: USB, porta serial Ad hoc ou desligar o Bluetooth. Se acontecer com você, envie os números de *heap* da página Status com o Bluetooth ligado e desligado.
- **Correções (issue #3):** a página web *Location* mostra na hora os valores salvos; a tela *Flights* sem mapa segue a configuração *Background*.
- **Fora desta versão:** opção de velocidade da CPU; japonês e chinês.

**Como atualizar:** ⚠️ **não use o *Update → Install* online do rádio** com este mod. Use sempre o **Manual upload**: faça backup → abra o IP do rádio → *Update* → *Manual upload* → escolha o `*-ota.bin` da sua variante. Para usar o visual novo: **Menu → Visual → Menu style → Carrossel ou Grade**; para voltar: **Original**.
