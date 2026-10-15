# Street Rod 2 Enhanced — agent rules (keep this file short: it is loaded every session)

Enhanced C/SDL3 port of California Dreams *Street Rod 2: The Next Generation* (1991), built on the faithful port
`../StreetRod2` at v0.1.0 (b79b6e9). State and subtasks: `PLAN.md`. Engine rules: `ENGINE.md`.

Enhanced rules:
- Enhancements live in `src/enhanced/`. Every edit to engine code is marked `/* ENH: … */` and kept
  as small as a hook (`grep -rn "ENH:" src` lists them all).
- `--classic` turns every enhancement off and must behave exactly as the faithful release
  (`sr2-enhanced --classic --check` says ok and a headless snapshot run matches the faithful build frame for frame).
  Each enhancement is its own option; enhanced defaults are on.
- The simulation is unchanged unless the subtask says so: render, interpolate, add input or settings,
  do not change game rules.
- A bug in the engine itself: fix it in `../StreetRod2` first (its PLAN.md), then port the fix here.
- Any change to engine code must keep the headless smoke run OK (`docs/PORTING.md` "Headless"; the faithful repo has `tools/sr2_smoke.py`).

Token budget rules:
- Work on ONE subtask per session (PLAN.md id, e.g. `S1.2`). Read only `PLAN.md` "Status" + that
  subtask. When done: status cell (`done YYYY-MM-DD`, `U2 pending`), `python tools/plan_site.py`,
  commit. Longer notes go in the "Notes:" list under the table, never between rows.
  When you need something from the user, add a row to `## User to-do` at the top of PLAN.md.
- Never read large files whole: `Grep` first, then `Read` with offset/limit.
- Specs, RE notes, disassembly and format docs stay in `../StreetRod2` (`port/spec/*.md`, `port/RE_GUIDE.md`, `FORMATS.md`, `port/symbols.csv`, `tools/x86dis.py`; SR1 sources further up in `../StreetRod`):
  search them, never dump them, never re-derive what they say.
- Do not view images or screenshots unless asked. Visual checks are the user's job (U-tasks): write
  the PNG / run the build, then ask the user what they see.
- Wide searches → the `scout` agent (Haiku).
- Windows: build with `C:\msys64\mingw64\bin` first on PATH. PowerShell 5.1 corrupts UTF-8 files:
  edit with the Edit/Write tools or Python only.
- Game data is never committed.
- No test HTML pages; implement, then tell the user what to test.
- Build: `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe` with `C:\msys64\mingw64\bin` on PATH. `Game\` is a junction to `..\StreetRod2\Game`.
- Never edit `../StreetRod2` or `../StreetRod*` from here, except the engine-bug flow above (a separate session in that repo).
