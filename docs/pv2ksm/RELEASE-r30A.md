# ATS-Mini v2.45 — PV2KSM r30A (community mod) — new look, same firmware

**Unofficial community mod by PV2KSM (PY2/PV2KSM), free, no warranty.** Based on the open-source [ESP32-SI4732 / ATS-Mini](https://github.com/esp32-si4732/ats-mini) firmware v2.45 (MIT, © 2025 ESP32-SI4732 Radio — see `LICENSE` and `NOTICE.txt`). Not affiliated with or endorsed by the original project.

> **r30A is a graphics update on top of r30.** Nothing changes in how the radio tunes, in the Wi-Fi/Flights/Weather/Finance data or in the languages. What is new is **how it looks**: icon menus, new main-screen layouts, colour options, restyled screens and text that follows your language. **The original look is still the default** — you opt in.

## ⚠️ Tested hardware — read first
| Variant | Status |
|---|---|
| **ATS-Mini, OSPI** (ESP32-S3, 8 MB octal PSRAM, Si4732) | ⏳ **TO FILL IN after testing on the radio** |
| ATS-Mini, QSPI | ⚠️ Compiled only — **not tested on hardware** |
| LilyGO T-Embed (Si473x) | ⚠️ Compiled only — **not tested on hardware** |

Flash **only the file for your own variant** (the web updater refuses the wrong one). Please report results for QSPI / LilyGO in *Issues*.
The pictures below are **renders made on a PC with the same drawing code as the firmware** (not photos of the radio). Real photos may differ slightly in colour.
The CPU stays at the original **80 MHz**; the new graphics were optimised for it (icons are cached, lighter anti-aliasing on small icons, calmer animation rate). How smooth it feels on your radio is the main thing to report.

## What's new (and what each thing does)

### Menu style — *Visual → Menu style*
| Option | What it does |
|---|---|
| **Original** (default) | The r30 look, unchanged: side list menus, original screens, original weather (with the rain-radar page). |
| **Carousel** | Turns the **new look** on. Full-screen menus: one big round icon in the middle, neighbours on each side, name below. |
| **Grid** | Turns the **new look** on. Full-screen menus as a grid of 8 icon tiles per page, with the selected name in the top bar. |

With Carousel or Grid every menu and editor uses the same style, the status message (e.g. Wi-Fi/update) is a centred box, and **Finance, Network and Weather** are restyled (colours follow your choices below). In the new look the **rain-radar page is not shown** (it stays in the Original look).

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

**Finance and Network (new look)**

![finance and network](https://raw.githubusercontent.com/PV2KSM/ats-mini-pv2ksm/pv2ksm-r30A/docs/pv2ksm/screenshots/r30A-finance-network.png)

## Files
| File | Use |
|---|---|
| `ats-mini-v2.45-ospi-r30A-ota.bin` | ATS-Mini OSPI — browser update (Update → Manual upload) |
| `ats-mini-v2.45-qspi-r30A-ota.bin` | ATS-Mini QSPI (compiled only) |
| `ats-mini-v2.45-lilygo-t-embed-r30A-ota.bin` | LilyGO T-Embed (compiled only) |
| `SHA256SUMS.txt` | Checksums |

## How to update
1. **Back up** first (web UI → Backup → export; leave "include Wi-Fi/keys" **off** if you will share the file).
> ⚠️ **Do not use the radio's online "Update → Install" for this mod.** It checks the *original* project's releases (not this one), so it says "Already up to date" and, if the original project publishes a newer version, it would install the original firmware over the mod. Always use **Manual upload**.

2. Open the radio's IP in a browser → *Update* → *Manual upload* → pick the `*-ota.bin` of **your** variant.
3. Wait for the reboot; don't power off while writing.
4. To try the new look: **Menu → Visual → Menu style → Carousel or Grid**. To go back: choose **Original**.

## Limits & licences
- Open-Meteo is free for **non-commercial use only**; RainViewer free API is for personal/educational use; adsb.lol, NOAA, CARTO/OSM, Coinbase and Frankfurter keep their own terms (`AVISOS-DE-TERCEIROS.md`).
- No Wi-Fi credentials or API keys are included in the binaries or source.
- When reporting a problem, include the **Last reset** line from the Status page and any on-screen message.

Credits: ESP32-SI4732 / ATS-Mini (MIT) · B612 font (SIL OFL 1.1) · LovyanGFX · data: adsb.lol, OpenStreetMap/CARTO, Open-Meteo, NOAA, RainViewer, Coinbase, Frankfurter.

---

# Resumo em português

**r30A = atualização gráfica da r30.** O rádio sintoniza, conecta e traduz como antes; o que muda é **o visual**: menus de ícones, novos layouts da tela principal, opções de cor, telas redesenhadas e textos que seguem o idioma. **O visual original continua sendo o padrão** — você escolhe se quer o novo.

**⚠️ Status do teste:** OSPI — *(preencher após testar)*. QSPI e LilyGO T-Embed: **só compiladas, sem teste em hardware**. Grave só o arquivo da sua variante. As imagens são **renderizações feitas no PC com o mesmo código de desenho do firmware** (não são fotos do rádio). A CPU continua em **80 MHz** e os gráficos foram otimizados para isso.

**O que cada coisa faz**
- **Menu → Visual → Menu style:** *Original* (padrão, visual da r30), *Carrossel* (menus em tela cheia com um ícone grande no centro) ou *Grade* (8 ícones por página). Carrossel/Grade ligam o **visual novo**: editores no mesmo estilo, aviso centralizado, Finanças/Rede/Clima redesenhados (sem a página do radar de chuva, que continua no visual Original).
- **Novo menu Visual** (antes de Configurações): **UI Layout** (escolhe a tela principal, com **prévia ao fundo**), **Theme** (temas de cor originais), **Accent** (cor principal), **Text color** (12 cores de texto), **Background** (5 fundos), **Menu color** e **Menu style**. Tema, UI Layout e cores saíram de *Configurações*.
- **9 layouts novos:** Neo Analog, Neo S-Meter, Vintage, Clock, Memories (memórias como marcadores), Minimal, VFD, LCD e Neon.
- **Idioma:** nomes e títulos dos menus Carrossel/Grade seguem *Configurações → Idioma* (PT/EN/ES, sem acentos). Os seletores da página web também foram traduzidos.
- **Página web (Config):** novos seletores de cor de destaque, cor do texto, fundo, cor do menu e estilo do menu.
- **Correções (issue #3, obrigado G8PTN):** a página web *Location* agora mostra na hora os valores de *Show map* / *Map style* salvos; a tela *Flights* sem mapa segue a configuração *Background* em vez de ficar sempre preta.
- **Fora desta versão:** opção de velocidade da CPU; japonês e chinês.

**Como atualizar:** ⚠️ **não use o *Update → Install* online do rádio** com este mod: ele consulta as releases do projeto *original* (diz "Already up to date" e, se o original lançar versão nova, instalaria o firmware original por cima do mod). Use sempre o **Manual upload**. Faça backup → abra o IP do rádio → *Update* → *Manual upload* → escolha o `*-ota.bin` da sua variante. Para usar o visual novo: **Menu → Visual → Menu style → Carrossel ou Grade**; para voltar: **Original**.
