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
| U3 | Decisions (defaults apply if unanswered), see U3 below | plan | 5 min |
| U2 | The faithful playtests (U6 in `../StreetRod2/PLAN.md`) are still open. Bugs found there are fixed in the faithful repo first (S5.6) and ported here by hand | engine | - |

## Status

- **Done:** S0.1 (repo from the faithful port v0.1.0, created while the faithful playtests were still open).
- **Next:** S1.1 (Opus). Your tasks: **User to-do** above.
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
| S1.1 | Design: how a high-resolution, smooth render back end hooks into the driver vector table, race road and `host.c` presentation (reuse `../StreetRodEnhanced/ENHANCED.md` stage 1); write `ENHANCED.md` | O | M | |

### Phase 2 - Render
| Id | Task | Model | Size | Status |
|---|---|---|---|---|
| S2.1 | Render back end: high resolution, smooth motion at the original draw distance (was E1) | O | L | |
| S2.2 | Longer draw distance (was E2; SRE stage 2) | O | M | |
| S2.3 | Widescreen / integer scaling options (was E5) | S | M | |

### Phase 3 - Input and settings
| Id | Task | Model | Size | Status |
|---|---|---|---|---|
| S3.1 | Gamepad and key bindings, A/Z gear keys option (was E4) | S | S | |
| S3.2 | Launcher from `../StreetRod2/sr2port/launcher/` (and `../StreetRodEnhanced/launcher/`), adapted: game folder, sound device, switches, window and Enhanced options; built by default, `-DSR2E_LAUNCHER=OFF` to skip (was E3) | S | M | |

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

## Decisions
(Settled answers from U3 go here, dated.)

## TODO (parked)
