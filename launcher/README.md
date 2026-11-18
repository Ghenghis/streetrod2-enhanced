# Street Rod 2 Enhanced (the launcher)

`Street Rod 2 Enhanced.exe` starts `sr2-enhanced`, the enhanced SDL3 port of Street Rod 2: The Next Generation
(California Dreams, 1991), with the options chosen in its window. It is the faithful port's launcher
(`../StreetRod2/sr2port/launcher`, same design, see its README) with the Enhanced options added. Built with
[wxWidgets](https://www.wxwidgets.org/) 3.2.

## The window

* **Game files**: **Folder** (`--game-dir`, default `Game` beside the launcher; needs `SR.EXE`, `LIB1`, `LIB2`,
  `LIB3`, `SR2_DATA`) and **Program** (`sr2-enhanced.exe`, default beside the launcher). Play stays greyed out until
  both are there.
* **Graphics and sound**: the original's video menu (EGA / VGA) and sound options menu (AdLib / PC speaker), passed as
  `e` / `v` and `adlib` / `speaker` so the game skips both text menus.
* **Options**: **Window size** (`--scale` 1..6), **Start in full screen** (`--fullscreen`), and the game's switches
  **Demo**, **No mouse**, **No upper memory** (`demo`, `nomouse`, `nouemem`).
* **Picture** (Enhanced, ENHANCED.md 3.9): **Picture** Enhanced / Original (`--classic`: every enhancement off, the
  other Enhanced controls greyed out), **Resolution** (`--res-scale` 1..8), **Anti-aliasing** (`--aa`, only the
  values with `res-scale x aa <= 16`), **Motion** smooth / the game's frames (`--motion-delay 0`), **Draw distance**
  (`--draw-distance` 14..120), **Haze** (`--haze` 0..100), **Aspect** 4:3 / 16:10 / 16:9 / fill the window
  (`--aspect`), **Whole-number scaling** (`--integer-scale`).
* **Game settings > Keys and gamepad**: the game's keys, and the Enhanced controls: **Gear keys** (`--no-gear-keys`
  when off), **Shift up / down** keys (`--key-shift-up/-down`, A / Z by default), **Gamepad shoulders, triggers and
  Start** (`--no-pad-extras` when off).
* **File > Preferences**: always on top. **About**: version, author and links.

Command line: `--game-dir D --scale N [--fullscreen] {e|v} {adlib|speaker} [nomouse] [demo] [nouemem]`, then either
`--classic` or `--res-scale N --aa N [--motion-delay 0] --draw-distance N --haze N [--aspect A] [--integer-scale]
[--no-gear-keys | --key-shift-up K --key-shift-down K] [--no-pad-extras]` (keys only when not A / Z).
`"Street Rod 2 Enhanced.exe" --print-command` prints the line the saved settings would run and exits.

Everything is remembered in `%APPDATA%\Street Rod 2 Enhanced\settings.ini` (`~/.config/street-rod-2-enhanced` on
Linux). Delete the file to go back to the defaults.

## Building

Built by the game's build beside `sr2-enhanced.exe` when wxWidgets 3.2 is found (MSYS2:
`mingw-w64-x86_64-wxwidgets3.2-msw`; Debian / Ubuntu: `libwxgtk3.2-dev`); `-DSR2E_LAUNCHER=ON` requires it, `OFF`
leaves it out. Standalone: `cmake -S launcher -B launcher/build -G Ninja`. On Windows the build copies the DLLs it
needs beside it (`../tools/copy_dlls.cmake`).
