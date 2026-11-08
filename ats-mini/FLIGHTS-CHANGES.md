
## r5
- Flights radar basemap: Web Mercator PNG tiles (default CARTO dark) downloaded over HTTPS, resampled into a 152x152 circular image in PSRAM (Map.cpp). Tile URL template, on/off and key are set on /location and stored in NVS (key never in source).
- Web UI restyled: light/dark theme, pill navigation, card tables (PageCommon.h, PageLocation.h, PageControl.h).

## r6
- Flights: full-screen radar over the basemap; encoder rotation selects aircraft (wraps), short click opens/closes a detail card (type, registration, altitude, speed, heading, vertical rate, distance), long press exits.
- Map: default tile URL now rastertiles/dark_all with ?key=; whole-screen basemap (320x170), connection reuse.

## r7
- Flights: no radar circle; the whole screen is the map (config range = centre to top/bottom edge, width ~1.9x), scale bar, aircraft over the full screen.
- Web Status page: "Voce esta sintonizado em" card (live frequency/band/station/signal) + animated weather card for the configured location (fetched by the browser from Open-Meteo).

## r8
- New UI layout "Cockpit" (Settings > UI) and colour theme "Cockpit" (Settings > Theme): FMC-style B612 Mono font (FontB612.h, SIL OFL), animated weather card on the left (fetched in the background while the layout is shown), big frequency with digit selector, STEP/BW/AGC/VOL cells, segmented S-meter with peak hold, SNR bar and band-position line. CockpitUI.h is shared with a PC preview.
