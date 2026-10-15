# Street Rod 2 Enhanced — SDL3

An enhanced native port of California Dreams's *Street Rod 2: The Next Generation* (1991), built on the faithful
[SDL3 port](https://github.com/kylofon/street-rod-2-sdl3), in the making. Not an emulator. The original game files are not included.

Status and plan: [PLAN.md](PLAN.md) (as a page: `python tools/plan_site.py`, then open
`site/index.html`). Agent rules: [CLAUDE.md](CLAUDE.md). Engine rules: [ENGINE.md](ENGINE.md).

## Features

* `--classic`: the original picture and behaviour, as the faithful port.

## Layout

* `src/` — the faithful engine (`ENGINE.md`; changes marked `ENH:`).
* `src/enhanced/` — the enhancements.
* `docs/PORTING.md` — the faithful port's porting rules.
* `tools/` — build, packaging and check scripts.

## License

MIT ([LICENSE](LICENSE)). *Street Rod 2: The Next Generation* is © California Dreams; its files are not included.
