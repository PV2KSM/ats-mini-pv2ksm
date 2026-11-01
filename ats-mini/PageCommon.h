#ifndef PAGE_COMMON_H
#define PAGE_COMMON_H

#include "PageTemplate.h"

static const char pageStart[] PROGMEM = R"HTML(<!DOCTYPE HTML>
<HTML>
<HEAD>
  <META CHARSET="UTF-8">
  <META NAME="viewport" CONTENT="width=device-width, initial-scale=1.0">
  <TITLE>{{title}}</TITLE>
  <SCRIPT>window.LANG='{{lang}}'</SCRIPT>
  <SCRIPT SRC="/i18n.js"></SCRIPT>
  <META NAME="color-scheme" CONTENT="light dark">
  <STYLE>
    :root {
      --bg: #f2f5fa; --card: #ffffff; --fg: #172033; --mut: #667085; --line: #e3e8f0;
      --acc: #2563eb; --acc2: #1d4ed8; --head: #e8efff; --inp: #ffffff; --shadow: 0 6px 24px rgba(20, 40, 90, .08);
    }
    @media (prefers-color-scheme: dark) {
      :root {
        --bg: #0b1220; --card: #121b2e; --fg: #e6edf7; --mut: #93a4c3; --line: #223152;
        --acc: #5b9dff; --acc2: #7db2ff; --head: #17233d; --inp: #0b1220; --shadow: 0 6px 24px rgba(0, 0, 0, .35);
      }
    }
    * { box-sizing: border-box; }
    BODY { margin: 0; padding: 0 0 24px; background: var(--bg); color: var(--fg);
           font-family: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif; line-height: 1.45; }
    H1 { text-align: center; font-size: 1.45em; font-weight: 700; margin: 0; padding: 22px 12px 6px; letter-spacing: .01em; }
    A { color: var(--acc); text-decoration: none; }
    A:hover { text-decoration: underline; }
    .NAV { display: flex; flex-wrap: wrap; justify-content: center; gap: 6px; padding: 10px 12px 6px; max-width: 820px; margin: 0 auto; }
    .NAV A, .NAV SPAN { padding: .38em .95em; border-radius: 999px; font-size: .9em; border: 1px solid var(--line); background: var(--card); color: var(--fg); }
    .NAV A:hover { border-color: var(--acc); color: var(--acc); text-decoration: none; }
    .NAV SPAN { background: var(--acc); border-color: var(--acc); color: #fff; font-weight: 600; }
    TABLE { width: calc(100% - 24px); max-width: 768px; margin: 14px auto; border: 0; border-collapse: separate; border-spacing: 0;
            background: var(--card); border-radius: 16px; box-shadow: var(--shadow); overflow: hidden; }
    TH, TD { padding: .65em .85em; border-top: 1px solid var(--line); }
    TR:first-child > TH, TR:first-child > TD { border-top: 0; }
    .HEADING { background: var(--head); color: var(--acc); column-span: all; text-align: left; font-size: .82em; font-weight: 700;
               text-transform: uppercase; letter-spacing: .08em; }
    TD.LABEL { text-align: right; color: var(--mut); width: 34%; }
    SMALL { color: var(--mut); }
    INPUT[type=text], INPUT[type=password], INPUT[type=number], SELECT, TEXTAREA {
      width: 100%; padding: .6em .75em; border-radius: 10px; border: 1px solid var(--line); background: var(--inp); color: var(--fg); font: inherit; }
    INPUT:focus, SELECT:focus, TEXTAREA:focus { outline: 2px solid var(--acc); outline-offset: 1px; }
    INPUT[type=submit], BUTTON { padding: .7em 1.6em; min-width: 44%; border: 0; border-radius: 12px; background: var(--acc); color: #fff;
                                 font: inherit; font-weight: 600; cursor: pointer; }
    INPUT[type=submit]:hover, BUTTON:hover { background: var(--acc2); }
    .CENTER { text-align: center; }
    .HEAP { color: var(--mut); font-size: .75em; text-align: center; margin: 1.5em 0.5em; }
    @media (max-width: 560px) {
      TD.LABEL { text-align: left; width: auto; display: block; padding-bottom: 0; border-top: 1px solid var(--line); }
      TR:first-child > TD.LABEL { border-top: 0; }
    }
  </STYLE>
</HEAD>
<BODY>
)HTML";

static const char pageEnd[] PROGMEM = R"HTML(
<FOOTER CLASS="HEAP">
  Internal heap: {{heap_used}} KiB used / {{heap_free}} KiB free &middot;
  PSRAM: {{psram_used}} KiB used / {{psram_free}} KiB free (at page render)
</FOOTER>
<FOOTER CLASS="HEAP">
  Data: map &copy; OpenStreetMap contributors &middot; weather Open-Meteo.com (CC BY 4.0) &middot;
  rain radar RainViewer &middot; flights adsb.lol &middot; METAR NOAA Aviation Weather Center &middot;
  prices Coinbase and Frankfurter (ECB) &middot; airports OurAirports &middot; font B612 (Airbus, SIL OFL) &middot;
  added features by PV2KSM
</FOOTER>
</BODY>
</HTML>
)HTML";

static void webNavigation(String &out, const char *activePage)
{
  static const struct { const char *name; const char *path; } pages[] =
  {
    {"Status", "/"},
    {"Control", "/control"},
    {"Memory", "/memory"},
    {"Location", "/location"},
    {"Finance", "/finance"},
    {"Backup", "/backup"},
    {"Config", "/config"},
    {"Wi-Fi", "/wifi"},
    {"Update", "/update"},
    {"Patches", "/patches"},
  };
  out += R"HTML(<NAV CLASS="NAV">)HTML";
  for(size_t i = 0; i < sizeof(pages) / sizeof(pages[0]); i++)
  {
    if(!strcmp(activePage, pages[i].path)) pageAppend(out, R"HTML(<SPAN>{{label}}</SPAN>)HTML", {{"label", pages[i].name}});
    else pageAppend(out, R"HTML(<A HREF="{{path}}">{{label}}</A>)HTML", {{"path", pages[i].path}, {"label", pages[i].name}});
  }
  out += "</NAV>";
}

#endif // PAGE_COMMON_H
