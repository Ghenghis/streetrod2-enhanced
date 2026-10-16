# Street Rod 2 Enhanced — plan

Built on the faithful port (`../StreetRod2`, release v0.1.0, commit b79b6e9). The faithful engine
runs unchanged (`ENGINE.md`); enhancements live in `src/enhanced/` and hook into the engine at marked
`ENH:` points. `--classic` turns every enhancement off.

Token efficiency is a first-class goal: small subtasks, one session each, the cheapest model that can
do the job, and the user doing what is cheap for a human (looking at pictures, playing).
Tracker: `python tools/plan_site.py` → `site/index.html` (this plan plus token usage per subtask).

## User to-do

What the agents need from you now, most urgent first. Agents add a row when they hand you a task and
remove it when it is done.

| Task | What to do | For | Time |
|---|---|---|---|
| U1 | S2.1: run `build\sr2-enhanced.exe` (Enhanced defaults, 1280x800 frame) next to `build\sr2-enhanced.exe --classic`: menus, garage, one race. The picture must look the same in both, the mouse pointer must follow the mouse in menus and the garage, Alt+Enter full screen fine | S2.1 | 10 min |
| U3 | Decisions (defaults apply if unanswered), see U3 below | plan | 5 min |
| U2 | The faithful playtests (U6 in `../StreetRod2/PLAN.md`) are still open. Bugs found there are fixed in the faithful repo first (S5.6) and ported here by hand | engine | - |

## Status

- **Done:** S0.1 (repo from the faithful port v0.1.0, created while the faithful playtests were still open),
  S1.1 (render back-end design: `ENHANCED.md`), S2.1 (host and coverage foundation; U1 check open),
  S2.2 (snapshots, interpolation, `SR_ENH_TRACE`, compare clock).
- **Next:** S2.3 (Opus, Milestone A: the first enhanced picture). Your tasks: **User to-do** above.
- Escalation rule: a subtask that fails twice on Sonnet → new Opus session with a 5-line note
  (symptom, file, what was tried). Never carry an old transcript over.

## Model guide

| Model (app model picker) | Effort | Use for | Relative cost |
|---|---|---|---|
| **Haiku** | low–medium | Packaging, launcher wiring of existing options, README/CHANGELOG/PLAN bookkeeping, regeneration | 1× |
| **Sonnet** | medium | Default: features on a finished design (back ends, panel/HUD, input, settings), bug fixes with a known cause | ~3× |
| **Opus** | high | Architecture (render back end, display lists, frame interpolation), projection/clipping/timing changes, cross-module bugs, escalations | ~5× |

## How to run a subtask (user, every time)

1. Code tab → **New session** in `C:\Coding\StreetRod2Enhanced` (never continue an old session for a new subtask).
2. Pick the model from the subtask table; set the effort.
3. Send exactly: `Do S1.1 from PLAN.md.` (with the subtask id).
4. When the agent says it is done: glance at the commit, do any U-check it asks for, close the session.

## Subtasks

Model: H = Haiku, S = Sonnet, O = Opus. Status goes in the row's last cell (`done YYYY-MM-DD`,
`U2 pending`); notes go in the "Notes:" list under each table, never between rows.

### Phase 1 - Design
| Id | Task | Model | Size | Status |
|---|---|---|---|---|
| S1.1 | Design: how a high-resolution, smooth render back end hooks into the driver vector table, race road and `host.c` presentation (reuse `../StreetRodEnhanced/ENHANCED.md` stage 1); write `ENHANCED.md` | O | M | done 2026-10-15 |

### Phase 2 - Render
Design: `ENHANCED.md` (section numbers below). Stage 1 = S2.1-S2.8, stage 2 = S2.9.

| Id | Task | Model | Size | Status |
|---|---|---|---|---|
| S2.1 | Host and coverage (§2, §2.1, §6 step 1): output frame up to 2560×1600 with logical 320×200 kept for presentation and mouse, `enh_compose` in `ega.c`, written mask in `vga_wr`/`vmovedata`, background/foreground layers, race begin/end and frame hooks, options `--res-scale` `--aa` `--motion-delay` `--enh-modes`; picture = scaled EGA (no visible change); `--classic` identical | O | M | U1 pending |
| S2.2 | Snapshots and interpolation (§3.1): snapshot ring at `road_frame`, `enh_opp_drawn`, `enh_frame_presented`, relative-heading blend, snaps; `SR_ENH_TRACE` and the deterministic compare clock (§5). Nothing drawn yet | O | M | done 2026-10-16 |
| S2.3 | **Milestone A** (§3.2, §3.4, §3.5): float road geometry, raster with AA, resolve through the live palette, sky, for the town and the street drag race (modes 0, 1) at draw distance 14, sprites as passthrough; `SR_ENH_COMPARE`; U1 check | O | L | |
| S2.4 | Mulholland (banks `BL/BR`, hillsides, drop-offs, guard rails, trusses, own sky) and the drag strip (modes 2/4: `L2/R2/C2`, flat, barrier walls) (§3.2) | S | M | |
| S2.5 | Sprites (§3.6): SR2_SHP format-A shapes at continuous scale, signs, banners, opponent and police from native LIB3 pictures, start tree and starter; remove every passthrough | S | L | |
| S2.6 | Mirror (§3.7) and the town cruise check (§3.8) | S | M | |
| S2.7 | Aqueduct, mode 8 (§3.3): channel geometry, walls and openings, boxes, pillars, format-B shapes, float roll, rotated opponent, night sky | O | L | |
| S2.8 | Performance pass (worker bands if over 8 ms), stage 1 acceptance (§3.10), U1/U2 playtest of all five race kinds | S | M | |
| S2.9 | Longer draw distance, stage 2 (§4): table ends per mode, far scenery fade, opponent at distance, haze, mirror distance (was E2) | O | L | |
| S2.10 | Widescreen / integer scaling options (was E5) | S | M | |

Notes:
- S1.1 renumbered the old S2.2 (draw distance) to S2.9 and the old S2.3 (widescreen) to S2.10.
- S2.1: `enh_frame_presented` is already in (with the other frame hooks); `enh_opp_drawn` is left to S2.2. The enhanced
  compose presents at the display rate only once a subtask draws the view (`rs.draws_view`, S2.3); until then it changes
  with the EGA picture, so snapshot runs stay comparable. `python tools/classic_check.py [--scaled 4]` checks `--classic`
  (and the scaled Enhanced frames) against the faithful release frame for frame on a title run and a demo drag race.
- S2.2: `enh_frame_begin` snapshots DGROUP into a ring of 8 (`enh_internal.h` `enh_snap`; the Aqueduct and horizon values
  the projection computes are added at `enh_layer_fg`), `enh_opp_drawn` (two hooks in `opponent_draw`) records the sprite
  level per view. `enh_interp(now)` gives the view at now - `--motion-delay`: linear in the pair around it, the previous
  pair's line faded in with a smoothstep (speed continuous), relative heading, positions clamped to the pair (no
  backward step at a hard stop), snaps as §3.1 (wall bounce = `DS_wall_hit_shift` set; `DS_aq_bounce_flag` stays 1 for
  the race). Evaluated every 8 ms in `enh_compose` while the overlay is on; S2.3 draws `view`.
  `SR_ENH_TRACE=file|-`: `S` line per snapshot, `C` line per evaluated frame. `SR_ENH_COMPARE=1`: host clock +1 ms per
  `host_pump`, no sleeps (`host_clock_ns`, `host_tick_ns`); `python tools/classic_check.py --clock` checks that two classic
  runs and an Enhanced run give the same frame sequence. Demo drag race: 9.1 snapshots/s (109.9 ms), lag 110 ms, steady
  |jerk| <= 6 sub-steps/s at ~550 sub-steps/s (the hard stop and the cam_x snap at the finish excepted), < 30 us per call.

### Phase 3 - Input and settings
| Id | Task | Model | Size | Status |
|---|---|---|---|---|
| S3.1 | Gamepad and key bindings, A/Z gear keys option (was E4) | S | S | |
| S3.2 | Launcher from `../StreetRod2/sr2port/launcher/` (and `../StreetRodEnhanced/launcher/`), adapted: game folder, sound device, switches, window and a "Picture" group (Graphics enhanced/original, Resolution, Anti-aliasing, Motion, Draw distance, Haze; `ENHANCED.md` §3.9); built by default, `-DSR2E_LAUNCHER=OFF` to skip (was E3) | S | M | |

### Phase 4 - Release
| Id | Task | Model | Size | Status |
|---|---|---|---|---|
| S4.1 | Packaging (`tools/package_release.py` from the faithful port, adapted to `sr2-enhanced` + launcher), README, CHANGELOG, release v0.1.0 | H | M | |

### Phase 0 — Repository
| Id | Task | Model | Size | Status |
|---|---|---|---|---|
| S0.1 | Repo from the faithful port at v0.1.0: engine, build, `src/enhanced/`, `--classic`, PLAN/CLAUDE/ENGINE/README | O | M | done 2026-10-15 |

## User tasks

### U1 — Visual checks (when an agent asks)
Run the build or open the PNG it names next to `--classic`, and reply in one line: `OK`, or what is
wrong and where. Agents do not look at images themselves.

### U2 — Playtests and bug reports
```
Build: <commit hash>   Options: <command line or launcher settings>
Steps: <what you did, from launch>
Classic does: <expected>
Enhanced does: <what happened; crash message if any>
Files: work\bugs\<n>.png
```
Several bugs in one message; one session then fixes them in a batch.

### U3 — Decisions (answer when convenient; defaults apply otherwise)
1. **Defaults**: every enhancement on by default, `--classic` for the original. Default: yes.
1. **GitHub**: create `street-rod-2-enhanced` when you want it; default: local only until the first release.
1. **Picture defaults** (`ENHANCED.md` §8): 1280×800 with 2×2 anti-aliasing, all five race kinds enhanced (Aqueduct last), draw distance 60 and haze 30 in stage 2. Default: yes.

## Decisions
(Settled answers from U3 go here, dated.)
- 2026-10-15 (user): picture defaults kept as proposed (1280x800, 2x2 AA, all five race kinds with the Aqueduct last, draw distance 60, haze 30). S2.1 runs on Opus. Sky scroll slowed to 1/8 of the original rate (`SKY_SCROLL`, as Street Rod Enhanced), not the faithful rate first.

## TODO (parked)
