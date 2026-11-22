# Street Rod 2 Enhanced

An enhanced native SDL3 port of *Street Rod 2: The Next Generation* (California Dreams, 1991), built on the
faithful [SDL3 port](https://github.com/kylofon/street-rod-2-sdl3): high-resolution races with smooth motion,
longer draw distance and haze, wide aspect ratios, gamepad support and a launcher. `--classic` plays exactly as the
faithful port. The original game files are not included.

## Graphics update scope

This fork is the Street Rod 2 home for one-for-one graphics updates, including cars, parts, props, backgrounds, and other existing visuals. Replacement assets and renderer support preserve the stock gameplay, roster, locations, saves, and input behavior. Keep each asset’s native slot, placement, mask, draw order, and animation timing. Verify a replacement in the running game before including it in a release.

The [Street Rod Enhanced fork](https://github.com/Ghenghis/streetrod-enhanced) is the matching graphics workstream for the first game. The separate StreetRod-HD integration project combines the accepted work from both forks and owns new locations, races, and gameplay.

Download: see Releases. Usage: [dist/README.md](dist/README.md).

Build (MSYS2 mingw64, SDL3, wxWidgets 3.2 for the launcher):

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build

MIT ([LICENSE](LICENSE)). *Street Rod 2* is © California Dreams.
