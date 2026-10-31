# r30A (2026-10-31) = r30 + new look as an option (no CPU-speed option)

Source: proj_r30a (copy of r30 `proj`). Build: `bash mkr30a.sh <name> <opi|enabled> [-DLILYGO_SI473X]`; outputs in outr30a/.
- Settings > **Menu style**: Original (default, r30 look unchanged) / Carousel / Grid. Carousel and Grid turn on the new look (icon menus, restyled editors, centred toast, Finance/Network restyle, weather without radar page + new graphics).
- UI Layout list: original 5 + Neo Analog, Neo S-Meter, Vintage, Clock, Memories, Minimal, VFD, LCD, Neon (always available; use Accent/Text/Background colours).
- Settings: Accent, Text color, Background, Menu color, Menu style (affect new look / new layouts).
- Web Config: Accent color, Text color, Background, Menu color, Menu style (PT/EN/ES via gen_webi18n.py).
- CPU stays at the original 80 MHz. Performance: icon cache (12 entries, PSRAM), 2x supersample for small icons, weather tick 120 ms, main screen tick 100 ms.
- Not included from v0: CPU speed, Waterfall/Ring/Radar/clock-screen layouts.
- Untested on hardware: everything (compiled only). Priority checks: frame rate at 80 MHz, Menu style switch, web selectors, weather original vs new.

## Update 2026-10-31 (afternoon)
- Main menu has a **Visual** entry: UI Layout, Theme, Accent, Text color, Background, Menu color, Menu style (removed from Settings).
- UI Layout / Theme / colours edit with the screen visible behind the side card (live preview), also in the new look.
- Carousel/Grid menu names and titles follow the language (Settings > Language): PT/EN/ES, ASCII only (screen fonts have no accents).
