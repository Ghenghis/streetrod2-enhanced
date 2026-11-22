# Street Rod 2 Enhanced

An enhanced native SDL3 port of *Street Rod 2: The Next Generation* (California Dreams, 1991), built on the
faithful [SDL3 port](https://github.com/kylofon/street-rod-2-sdl3): high-resolution races with smooth motion,
longer draw distance and haze, wide aspect ratios, gamepad support and a launcher. `--classic` plays exactly as the
faithful port. The original game files are not included.

Download: see Releases. Usage: [dist/README.md](dist/README.md).

Enable the registered art in the parked garage with `--body-art art/garage-bodies`
from the repository root. Add `--body-art-restored` for fresh paint on supported
cars. Pass an absolute art directory when launching elsewhere.
See [garage body art](art/garage-bodies/README.md).

Build (MSYS2 mingw64, SDL3, wxWidgets 3.2 for the launcher):

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build

MIT ([LICENSE](LICENSE)). *Street Rod 2* is © California Dreams.
