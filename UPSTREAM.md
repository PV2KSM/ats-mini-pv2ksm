# Upstream base

This mod is based on **ESP32-SI4732 / ATS-Mini v2.45** (<https://github.com/esp32-si4732/ats-mini>, MIT license, commit `a45a27d4ca0d9ce4230469248a4025819393fbac`, tag `v2.45`).

`upstream-base/v2.45/` keeps an unmodified copy of that firmware (`ats-mini/`, `changelog/`, `LICENSE`). It is for comparison only and is not part of the build.

## Where the mod differs from the base
- 18 of the 68 shared source files were changed by the mod: `About.cpp Common.h Draw.cpp Draw.h Menu.cpp Menu.h Network.cpp Ota.cpp PageCommon.h PageConfig.h PageMemory.h PageStatus.h Station.cpp Storage.cpp Themes.cpp Utils.cpp Web.cpp ats-mini.ino`.
- The other shared files are identical to the base.
- The mod adds about 47 new source files (Flights, Weather, Finance, News, Map, Radar, layouts, translations, extra web pages).

## Taking a new upstream release
```
tools/upstream-diff.sh v2.46
```
The script downloads that release, compares it with the base and prints:
1. files upstream changed that the mod did **not** touch (can be copied as they are);
2. files upstream changed that the mod **also** changed (merge by hand);
3. files added or removed upstream.

Then: apply the changes, build, test on the radio, and replace `upstream-base/` with the new release.
