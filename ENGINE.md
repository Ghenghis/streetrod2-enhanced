# Engine rules — Street Rod 2 (from the faithful port v0.1.0)

The engine in `src/` is the faithful port's. This file condenses its porting rules so that Enhanced
sessions do not need the faithful repo's docs for everyday work. Deep detail stays in
`../StreetRod2` (searched, never dumped): `port/spec/*.md`, `port/RE_GUIDE.md`, `FORMATS.md`, `port/symbols.csv`.

## Architecture
Faithful reimplementation of SR.EXE (1991), VGA 16-colour path, as a delta port of the Street Rod (SR1) engine.
The unpacked EXE image sits in real-mode memory (`src/mem.c`): file segment SSSS -> `SEG = 0x1000 + SSSS`,
**DGROUP = file 3E08** (`DS:xxxx` = image `0x3E080 + xxxx`). Game code in `src/game/`, platform and video in
`src/platform/`, sound in `src/sound/`, host (SDL3 window, timer, keys, mouse, gamepad, speaker) in `src/host.c`.
`src/main.c`: `--game-dir`, `--scale`, `--fullscreen`, `--check`, `--run-ms`, `--classic`, then the original's switches.

## Memory model
Original state stays in DGROUP and is accessed through generated names: `DS_x` globals and `FN_x` functions in
`src/symbols.h`, generated from `port/symbols.csv` by `../StreetRod2/tools/gen_symbols.py` (run it there and copy
the header; do not hand-edit). Far code pointers stored in game data go through `codeptr_register(FN_x, fn)`.

## Timing
PIT-rate tick and retrace clock from `host.c`. While AdLib music plays the music ISR owns INT 8 and chains the
game tick at 68.6 Hz (tempo 120) or 60 Hz (tempo 60), not 72.8 Hz (spec `sound.md` section 9). The simulation runs in
that tick; display-rate work must not change it.

## Video
EGA/VGA planar model: `src/platform/ega.c` (4 planes, pages at A000:0000/2000/4000/6000, palette, scan-out),
`vga.c` register model. Drawing goes through a 20-slot driver vector table at `DS:A0AE` (`drv_*` wrappers):
blit, masked blit, text, line, hspan, fill_rows, poly_fill (slot 18), road helpers (slots 13, 14, 16, 19).
Pictures: `LIB1/2/3` libraries (`src/platform/lib.c`, 28-byte directory records, `pic_unpack`). The 320x200 screen is
shown at 320x240 aspect, scaled by `--scale N`.

## Input / sound
XT scan codes from the host, mouse and gamepad via `host.c`. Sound devices: AdLib (OPL2 through Nuked-OPL3:
`src/sound/adlib.c`, `mus.c`, `adfx.c`, bank `MSR.SND`, songs `*.MUS`) and PC speaker (`spk.c`). Effects go
through the 25-slot table at `DS:BB8C`.

## Verification
`sr2-enhanced --game-dir Game --check` loads and verifies SR.EXE. Headless: `SDL_VIDEO_DRIVER=dummy
SDL_AUDIO_DRIVER=dummy SR_SNAPSHOT_DIR=dir sr2-enhanced --run-ms 600` writes `snapNNNN.bmp` (also `SR_KEYS`,
`SR_SNAPSHOT_MS`, `SR_DEBUG_RACE`, `SR_DEBUG_END`; see `host.h`). The faithful repo has `tools/sr2_smoke.py` (15 checks).
`--classic` must give the same frames as the faithful build. Enhanced aids: `SR_ENH_TRACE=file` (snapshots and the
interpolated view), `SR_ENH_COMPARE=1` (deterministic host clock; `=2` also the edge compare of original vs enhanced view, `=3` its
details), `tools/classic_check.py [--scaled N] [--clock]`, `tools/enh_compare.py` (geometry acceptance, §3.10 item 3).

## Where to hook (ENH: points)
Race road drawing (`src/game/race_road.c`, race_render), the driver vector slots (blit / line / poly_fill) for a
higher-resolution back end, `host.c` frame presentation (scaling, interpolation), the key map (`host.c`,
`platform/kbd.c`), `main.c` argument parsing (`enh_parse_arg`). Full porting rules: `docs/PORTING.md`.
