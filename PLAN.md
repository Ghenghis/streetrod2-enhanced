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
| U1 | S2.3 + S2.5 + S2.6 (town and street drag race): run `build\sr2-enhanced.exe` next to `build\sr2-enhanced.exe --classic`; drive in town (cruise to Burger's) and race a street drag race. In Enhanced the road, kerbs, sidewalks, building walls and sky must be sharp (no stair steps) and move smoothly between the game's frames, at the same place as in classic; the sky turns slower when steering (1/8). **Objects** (trees, lamp posts, road signs, hydrants, location signs, billboards, banners): since your S2.5 feedback each is drawn from its largest picture only and scaled by true perspective, so look that they **grow smoothly and slowly** as you approach, with **no size steps** (no LOD changes), the same size as classic when near (rows where classic shows its biggest picture), smaller than classic's jumps would suggest in the middle distance, and that they now appear at the far end of the road (fading in) instead of popping in halfway; trees stand behind the buildings. **Mirror**: now drawn by Enhanced too: sharp road, walls, trees and banners in it, the same view as classic, moving smoothly, frame lines crisp; the opponent / police car and the starter in the mirror are still classic pixels (later subtask). Report anything misplaced, flickering, popping or too big | S2.3, S2.5, S2.6 | 15 min |
| U1 | S2.9 part 1 (draw distance x1.25, town and street drag race): run `build\sr2-enhanced.exe` (default 1.25) next to `build\sr2-enhanced.exe --draw-distance-scale 1` (the original distance) and `--classic`. Look at: the road, walls and kerbs now reach about 25 % further (17.5 instead of 14 segments) with **no popping at the far edge** (objects fade in over the last segment, nothing appears suddenly), **no road over the sky** (the far road ends just below the horizon panorama / the town's rooftop panorama, also when the car bounces; only crests may rise above it), objects stand on the road's far part and never float beyond its end; **town ends sane** (cruise to Burger's: the street ahead shows only town blocks, no race road); **drag race after the finish** (fixed after your playtest of f1d3127): past the finish line plain road with centre dashes and no trees / signs, now following the course's hill exactly as the original does, so an **opponent waiting past the finish stands on the road** (not in the air); and **after the race stops** (the last second before the result boxes, and behind the boxes) the picture stays the enhanced one: **no revert to the old low-res road / hill**; the boxes themselves are classic pixels. Also check the end of a town cruise (arrival) the same way. The mirror is unchanged | S2.9 | 10 min |
| U1 | S2.4 (Mulholland and the drag strip): run `build\sr2-enhanced.exe` next to `build\sr2-enhanced.exe --classic`. **Mulholland Drive race**: sharp road, white finish band, centre dashes; the **hillsides** (yellow banks) beside the road and the **drop-offs** where the road has a cliff edge, the **guard rails** (posts every other segment) and the **truss bridges** (bridge frames: posts, top bar, diagonals) must be at the same places as in classic and move smoothly, no stair steps; the sky/mountains turn slowly when steering (1/8) and shift a little with hills; bushes, rocks and obstacles (the 0x14 ones) grow smoothly, no size steps; while driving over a hill crest nothing flickers. **The mirror in Mulholland is still the classic picture** (later subtask). **Drag strip (Grudge Night, qualifier and rounds)**: flat road with the **barrier walls** (red / cyan stripes of 5 segments) along both sides moving smoothly, the wide start dashes, start tree (classic pixels for now: it may lag a few pixels behind the interpolated road) and starter, the opponent (classic pixels), mirror drawn by Enhanced with the walls. Report anything misplaced, missing, flickering or ugly | S2.4 | 15 min |
| U1 | S2.5 (opponent, police, start tree, starter): run `build\sr2-enhanced.exe` next to `build\sr2-enhanced.exe --classic`. **Drag race (street) and Mulholland**: the opponent car is now drawn from its own picture by Enhanced: sharp, and it must stay glued to the road (no sliding or bobbing against the road) as it pulls ahead, shrinking smoothly with distance (no size steps; classic shows its shrunk copies), and when it passes you it moves into the mirror (street drag race: sharp car in the mirror). Check that it is hidden behind nearer hills/objects as in classic, and that the crash / contact placement (bumping the opponent) looks like classic. **Street drag race mirror**: the starter figure stands at the start line in the mirror. **Drag strip (Grudge Night)**: the start tree is drawn by Enhanced (sharp, lamps light up in sequence) and grows as you approach; the opponent in the strip. Police cars (town cruise, speeding): same checks. Not changed on purpose: the starter sliding out at the bottom of the screen (screen overlay, classic pixels), the Mulholland mirror (S2.6 leftover). Report anything misplaced, flickering, too big or missing | S2.5 | 15 min |
| U1 | S2.6 (Mulholland mirror): run `build\sr2-enhanced.exe` next to `--classic`, Mulholland Drive race. Look in the rear-view mirror: sharp road, yellow hillsides / banks and drop-offs, verge strips, guard rail, truss bridges, bushes and rocks, sky strip above the horizon; the opponent car in the mirror (sharp, no size steps) and the starter figure at the start. Same places as in classic, nothing flickering | S2.6 | 10 min |
| U1 | S2.7 (Aqueduct): run `build\sr2-enhanced.exe` next to `build\sr2-enhanced.exe --classic` and race the Aqueduct (the night race in the concrete channel). Look at: the **channel** (grey walls and floor, black floor edges, white wall-top lines) sharp and moving smoothly, the far end now about 25 % further away; the **joint lines** every few segments, the **wall openings** (dark windows and doors in the walls), the **centre trough** (dark strip with a black outline, interrupted every 24 segments), the **low boxes** across the middle and the **two thin pillars**, the **floor shapes** (debris on the floor) growing smoothly with no size steps, and the white finish band; **steer up a wall**: the whole picture rolls smoothly with the climb (no 1-degree steps), everything stays glued together, the floor shapes and the opponent tilt with it, beyond the channel's far end the concrete continues (the original shows sky there while rolling); **bump a wall hard** (the bounce spins the view: it snaps as the original, no smearing); the **opponent car** sharp, tilted by its continuous bank angle when it rides up a wall (classic: 7 fixed angles), sizes like the other races; the **night sky**: small sharp stars and the sky shapes, scrolling at 1/8 of the original's speed when you steer (the same rule as the other races' skies: say if the Aqueduct should keep the original speed); the **mirror**: channel, stars, the starter at the start, the opponent behind you. Report anything misplaced, flickering, popping or missing | S2.7 | 15 min |
| U3 | Decisions (defaults apply if unanswered), see U3 below | plan | 5 min |
| U2 | The faithful playtests (U6 in `../StreetRod2/PLAN.md`) are still open. Bugs found there are fixed in the faithful repo first (S5.6) and ported here by hand | engine | - |

## Status

- **Done:** S0.1 (repo from the faithful port v0.1.0, created while the faithful playtests were still open),
  S1.1 (render back-end design: `ENHANCED.md`), S2.1 (host and coverage foundation; U1 check open),
  S2.2 (snapshots, interpolation, `SR_ENH_TRACE`, compare clock), S2.3 (Milestone A: enhanced town and street drag
  race view, `SR_ENH_COMPARE`; its U1 check open), the road-side objects part of S2.5 for modes 0 and 1 (pulled
  forward on the user's S2.3 playtest; since the S2.5 playtest the largest LOD only at true 1/z; the rest of S2.5 after
  S2.4), S2.5 complete (opponent, police, start tree, starter), S2.6 for modes 0 and 1 (enhanced mirror, town cruise checked; U1 check open), the modes 0 and 1 part of
  S2.9 (draw distance x1.25 = 17.5 segments, `--draw-distance-scale`, on the user's request; U1 check open), S2.4 (Mulholland,
  drag strip), S2.6 rest (Mulholland mirror), S2.7 (the Aqueduct; U1 check open).
- **Next:** every race kind is now drawn by the enhanced view (S2.7 Aqueduct done, U1 pending); remaining Phase 2: S2.9 (table ends per mode, far scenery fade, opponent at distance, haze, mirror distance), then S2.8 (performance pass, stage 1 acceptance). Your tasks: **User to-do** above (the U1 checks).
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
| S2.3 | **Milestone A** (§3.2, §3.4, §3.5): float road geometry, raster with AA, resolve through the live palette, sky, for the town and the street drag race (modes 0, 1) at draw distance 14, sprites as passthrough; `SR_ENH_COMPARE`; U1 check | O | L | U1 pending |
| S2.4 | Mulholland (banks `BL/BR`, hillsides, drop-offs, guard rails, trusses, own sky) and the drag strip (modes 2/4: `L2/R2/C2`, flat, barrier walls) (§3.2) | S | M | done 2026-10-29, U1 pending |
| S2.5 | Sprites (§3.6): SR2_SHP format-A shapes at continuous scale, signs, banners, opponent and police from native LIB3 pictures, start tree and starter; remove every passthrough | S | L | done 2026-10-29 (modes 0, 1 objects 2026-10-16; opponent, police, start tree, starter 2026-10-29), U1 pending |
| S2.6 | Mirror (§3.7) and the town cruise check (§3.8) | S | M | done 2026-10-30 (modes 0, 1 2026-10-16; Mulholland mirror and opponent / starter in every mirror 2026-10-30), U1 pending |
| S2.7 | Aqueduct, mode 8 (§3.3): channel geometry, walls and openings, boxes, pillars, format-B shapes, float roll, rotated opponent, night sky | O | L | done 2026-10-30, U1 pending |
| S2.8 | Performance pass (worker bands if over 8 ms), stage 1 acceptance (§3.10), U1/U2 playtest of all five race kinds | S | M | |
| S2.9 | Longer draw distance, stage 2 (§4): table ends per mode, far scenery fade, opponent at distance, haze, mirror distance (was E2) | O | L | modes 0, 1 at factor 1.25 done 2026-10-16 (U1 pending); rest open |
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
- S2.3: `enh_road.c` (float geometry: segments seg+1..seg+14, the last stretched to z = 116 as the original's row 14,
  cut into up to 8 slices; town walls, tall parts and their screen-space end faces, bands, dashes from the DS:7E52 table;
  sky and panorama from page 1 rows 0x66.., scroll 1/8, horizon from HGT(P - 96)), `enh_raster.c` (index samples at
  res_scale x aa, convex fill, linear-light resolve through the snapshot palette), compose overlays the view rows
  18..98 outside the mirror where the written mask allows. Passthrough brackets (`enh_layer_fg/bg`) around
  `front_objects` and the two front `opponent_draw` calls in `road_draw_front`; the mirror rectangle stays the
  original's by geometry until S2.6. Modes 0 and 1 only (`rs.draws_view`); other modes unchanged.
  `SR_ENH_COMPARE=2` (`enh_compare.c`): per race frame the snapshot at motion delay 0 with quantized depth, centre sample
  per pixel, passthrough pixels laid over, colour edges matched within +-1 px both ways; `=3` lists misses and the row
  tables, `SR_ENH_COMPARE_DIR` dumps. `python tools/enh_compare.py` (threshold 96 %): drag 99.8/99.9 % (h), 99.7/99.9 % (v),
  town 98.9/99.1 % (h), 97.1/98.6 % (v). The float rows equal the original's row values (rows 1..14); the town's misses
  are the original's 1/16-px slope drift on long near bands and +-1 row tops against the textured rooftop panorama.
  Render 4.5-5 ms per displayed frame at 1280x800 2x2 (headless). `classic_check.py --scaled` now runs `--enh-modes none`,
  `--clock` masks the view rows; both, `--classic --check` and the faithful `sr2_smoke.py` (15 paths) pass.
- S2.5, part 1 (modes 0 and 1, done 2026-10-16 before S2.4 on the user's request after the S2.3 playtest: scenery not
  smooth, trees in front of buildings). Cause of the trees: not timing (the same number of passthrough pixels lay over
  enhanced wall pixels at motion delay 0 and 110), but the original's order: `front_objects(row)` runs after the row's
  walls, so an object behind the building line (trees and posts at XOL/XOR and beyond, the far trees of flag 2 at
  (XFL + XMIN) / 2) paints over the wall of its own segment, which is nearer. `enh_sprite.c` re-makes `front_objects`
  from the track flags at the interpolated view (shapes incl. hydrants and banner text, location sign and billboard
  pictures from their sheets, banner frame), at the original's row values (continuous estimates of its integers);
  sizes follow the original's LOD / sign size at its row depths, interpolated between them (the next larger LOD mapped
  onto the interpolated box; the sheet's smaller signs are the mips). In the town the objects behind the building line
  are drawn before the segment's walls (`ENH_OBJ_OUT`), the rest after the band; the drag race and the compare view
  keep the original's order. `enh_objects_begin/end` replace the passthrough bracket (still a passthrough in the
  other modes); `enh_obj_orig` hooks in `scenery_draw_shape/_hydrant/_sign_picture/_banner` record the original's calls
  for `SR_ENH_COMPARE`: `tools/enh_compare.py` (now also the cruise) matches every call (kind, type, row depth, side,
  anchor +-1 px; drag 7897, drive home 1224, cruise 1597 calls: all matched both ways) and checks the continuous size
  at the original's row depths (within half a LOD step + 0.5 px: all); edges with the objects now enhanced: drag
  99.8/99.9 % (h), 99.7/99.8 % (v), town 98.7-98.8/99.1 % (h), 96.9-97.1/98.5-98.7 % (v). The live anchors (continuous)
  are within 1 px of the original's integers for 92 % of the calls, the rest within 1.5 px. Render 5.5-6.0 ms per
  displayed frame at 1280x800 2x2 (headless; S2.3 4.5-5). Open in S2.5: the opponent and police (passthrough), the
  objects of the other modes (Mulholland bushes, rocks, trusses, guard rails, drag strip start tree and starter) once
  S2.4 draws those modes.
- S2.5 part 2 and S2.6 (modes 0 and 1, 2026-10-16, on the user's S2.5 playtest: objects showed the LOD steps and grew
  too fast on approach; "keep only the highest level, also in the mirror"). The original's LODs 3..6 follow 1/z
  (tree: height x z = 2040..2230) but LODs 0..2 are only 45..75 % of it, so the interpolated sizes grew 3x over rows
  9..6. Now every object is drawn from its largest LOD / the sign sheet's largest size, scaled by zt / z (zt: that
  LOD's row depth, 24.5; hydrant 16.5; far objects zt / (z + 8 off)); size = the original's at the largest LOD's row,
  true 1/z elsewhere (ENHANCED.md §3.6). At row 10 that is twice the original's LOD 0, so the objects are now drawn out
  to the road's far end (row 14) and fade in over its last segment (`ras_alpha` screen door). Mirror (`enh_road.c`
  `mirror`): mirror projection, rows 1..7 (flags seg - r - 1), backdrop pictures from page 2 (`enh_sheet`, now
  reading the VRAM page from its segment), town walls, bands, dashes, banner and `mirror_objects` (same size rules,
  depth 4 x mirror z), frame lines, clipped to the mirror (`ras_clip`); coverage no longer excludes the mirror;
  opponent / police and starter in the mirror are passthrough brackets (not trivial: native sprites are S2.5's open
  part). Hooks: `enh_mirror_objects_begin/end`, two passthrough brackets. `tools/enh_compare.py`: object calls now
  also from the mirror (drag 12842, drive home 1951, cruise 2473: all matched both ways), sizes measured against the
  largest-LOD perspective expectation (all within 0.5 px), mirror edges apart (drag 99.1-99.7 %, town 95.6-98.8 %,
  threshold `--mirror-min` 95: the original's near mirror rows step up to 15 px per pixel row); view edges unchanged
  (drag 99.7-99.9 %, town 96.9-99.1 %). Render 7.2 ms per displayed frame in the cruise at 1280x800 2x2 (headless, the S2.5 build 6.1 ms back to back; the machine was loaded). Identity: classic_check plain, `--clock`, `--classic --check`, faithful smoke 15/15 ok; `--scaled 4` race_drag diverged (whole race, not the picture) in 3 of 7 runs under 100 % external CPU load, ok in the last 2 (the S2.5 build 6/6 ok in the same period): rerun when the machine is idle. Town cruise (§3.8): mode 0 runs the normal renderer, compared in
  the cruise run.
- S2.5 part 3 (2026-10-29: opponent, police, start tree, starter; every sprite passthrough except the aqueduct and the
  starter's screen slide is gone). `enh_props.c` reads the game's masked LIB3 pictures from DGROUP at render time and draws
  them with `ras_sprite` (ras_image with index 0xFF transparent). Car: the level the original chose (snapshot
  `opp_s[view]`: 0 side, 1 rear 3/4, 2..8 rear, 9..12 front) selects the native picture (levels 3..8 -> the level-2 picture,
  10..12 -> level 9, never the shrunk copies); anchor = opponent_draw's rule with continuous values (`car_front` /
  `car_mirror` in `enh_road.c`: z = |d / 8| + 4 with d = O - P, YAW / HGT at the car, the mirror's +10 on the heading
  term); size = min(1.3, 17 / z) x native (mirror 15 / z_m; k fitted to the original's shrunk levels at their depths).
  Crash / contact placement (res == 3, opp_contact) draws the original's rect (`enh_opp_place` records r.dx - r.sx, r.dy).
  Painter's order as the original: after the row's objects, before the barrier walls (`segment`), row 0 last; mirror after
  the row's objects. Start tree: largest sub-picture of the panel (lamps are painted into it by the game), 4 segments past
  the start line, size min(1.3, 21.7 / z). Starter in the mirror (mode 1): size 0, feet pinned at y 0x27 as the original.
  Hooks: `enh_prop_fg/bg(mirror)` replace the six `enh_layer_fg/bg` brackets (passthrough in the aqueduct, in the compare
  view and for the Mulholland mirror), `enh_opp_place`. `SR_ENH_TRACE` prints the pictures drawn per race. Mulholland /
  strip road-side objects were already enhanced (S2.4). Checks: `--classic --check` ok; `classic_check.py` plain and
  `--clock` ok; `enh_compare.py` equal to before (compare view keeps the original's car); headless demo runs of the Enhanced exe (drag, grudge, Mulholland: no crash; police not exercised, props drawn per
  `SR_ENH_TRACE`); `--scaled 4` race_drag flaky (whole-race divergence also at the previous commit, `--enh-modes none` runs no hook).
- classic_check wall-clock flake (2026-10-30): the street drag's green light is a tick deadline (`race_drive.c:275`)
  while the race steps every `pace_ticks(8)`, whose phase depends on load time, so green lands a frame early or late:
  two deterministic race variants (split at about 3.5 s), each exe (faithful included) lands in either; snapshots
  (first frame >= 250 ms after the last) also skip short-lived frames now and then. Fix: plain mode tries a path up to
  `--tries` (4) times, pools the faithful frames of all tries and reruns classic / scaled until their frames are in
  the pool (the try that passed is printed). `--clock` was never affected.
- S2.9 part 1 (modes 0 and 1, 2026-10-16, on the user's request "increase the draw in-distance by 25 % for the
  scenery and objects"): `--draw-distance-scale X` (1..4, default 1.25; 1 = the original's 14 segments; `--classic`
  ignores it), one factor for road, walls, objects, fade and table ends (`enh_road.c` `cam_of`, `road`): far end
  z = 4 + 112 x scale (144 = 17.5 segments), each segment up to the one holding the far end drawn with its own flags
  (the compare view keeps the original's stretch of its last row to the far end), objects out to the far end, fading
  in over its last segment (dithered `ras_alpha`, as before). Table ends (§4 item 2), live view only: past the street
  drag race's finish (segments >= track_end 0x17C, the road course in the table) plain road (only the course's
  centre-dash bit kept), no objects; YAW / HGT stay the table's (the f1d3127 hold of the finish values made the road
  flat while the original, whose opponent pixels show, places the waiting opponent on the course's hill: the car hung
  in the air; fixed 2026-10-16); the town is clamped at its last segment 0xB3 (never reached at 1.25: Burger's at
  seg 0x9B sees to 0xAC; up to scale 1.7); the town's road surface is clipped to rows >= 63 (the rooftop panorama's
  base, its fill there is the road colour) so the far road never covers the panorama (§4 item 6); the drag race's far
  road at z 144 lies 1.6 px below the horizon row (flat road reaches the horizon only beyond z ~ 340, scale 3).
  The mirror is unchanged (reads the table as the original). `tools/enh_compare.py` runs every check at scale 1
  (unchanged: drag 99.7-99.9 %, town 96.9-99.1 %, mirror 96.0-99.7 %, all object calls and sizes) and repeats each run
  at `--scale` 1.25, where the compare tool (`D` lines) renders seg + 1 .. seg + 14 alone and the whole view: near
  samples changed 0 of 260 M (drag) / 24 M (drive home) / 26 M (cruise), extension 8.7 M / 0.4 M / 0.5 M samples.
  Render: no measurable cost, 8.2-11.0 ms per displayed frame at 1280x800 2x2 headless at 1.25 against 9.7-10.8 ms at
  1 back to back (the machine was loaded: S2.6 measured 7.2 ms). Identity: `--classic --check`, classic_check plain
  and `--clock` ok; `--scaled 4` race_drag failed in 2 of 3 runs (once the classic run itself diverged from the
  faithful one: wall-clock timing under load; that run uses `--enh-modes none`), ok in the last; faithful smoke 15/15.
  Fix 2026-10-16 (user playtest of f1d3127: "in the last second it reverts to a hill drawn with the old low-res
  lines"): the revert was the 2 s after `race_run` ends: it copies the shown race page to page 0 and the back page,
  shows page 0 (result boxes follow), and `enh_race_end` had switched the overlay off, so the original's picture
  showed. Now `enh_race_end` starts a hold (`enhanced.c` `hold_start` / `hold_follow`): the last rendered raster
  (nothing re-rendered, the race data is gone) stays over the displayed page, live palette, where its pixels still
  equal the race picture's and were enhanced at the end; writes after the race show the original (boxes); the hold
  follows a page switch by content and ends when more than half of the view is drawn over, the display leaves the
  pages, or the next race begins. Engine hook: `ega_get_start` (ENH:). Headless (SR_ENH_TRACE `# hold` lines): drag
  race end hold on page 1, followed to page 0 with all 20355 enhanced view pixels, kept through screens 23 / 6 to the
  drive home; cruise arrival the same (21864). Checks: enh_compare.py unchanged (all ok); `--classic --check`, classic_check `--clock`, `--scaled 4` ok, plain race_drag
  once 1 frame short (end timing), ok on rerun; faithful smoke 15/15.
  Remaining for S2.9: `--draw-distance N` up to 120 (generalise the factor; fewer slices far away, horizon clip for
  far road), the other modes' table ends (Mulholland past 0x3DD, drag strip past 0x58, Aqueduct past 0x156) once
  S2.4 draws them, the town end beyond scale 1.7 (a closing view instead of the clamp), opponent and police at
  distance, haze, mirror distance, cost at N = 60 / 120.
- S2.4 (2026-10-29): the drag strip (modes 2, 4) and Mulholland (mode 0x10) are drawn by the enhanced view (`rs.draws_view`
  for every mode but the Aqueduct). Strip: `enh_flag` reads L2 / R2 / C2 at the segment, YAW / HGT / BL / BR / LANE at
  seg + 0xC0 (`enh_toff`, also in `enhanced.c` `trk`), flat, barrier walls (stripes of 5 segments, front and mirror: the mirror's
  stripe index runs with seg + row as the original's), wide start dashes, objects through the existing `front()` /
  `mirror()` (modes flags were already in it). Mulholland: `sky_mulholland` (page-1 picture from row 0x6E - dy, dy = height
  over HGT four segments back, clamped, scroll at 1/8; the compare view uses the original's whole-byte 2 px scroll),
  `hill_side` re-makes `front_hillsides` (both branches incl. the bridge-end polygons and the free-standing peaks when a
  bridge ends in rows 10..14 = `l16`) per slice with the hill tables DS:7E1E.. read at the continuous row (`tabv`),
  verge strips (colour 2), ground (9) only beside flat banks, guard rails (post on every other segment, rail wall with
  edge lines), trusses and truss ends (colour 10, `dash + 1` px thick), white finish band from segment 0x3AD, bush groups,
  rocks and obstacle 0x14 in `enh_sprite.c`, outer-band objects only on flat banks. Passthrough (original pixels, may lag
  the interpolated road): the start tree (`ENH:` hook in `front_objects`), the opponent / police / starter as before and
  the whole **Mulholland mirror** (`rs.mirror_native`: those pixels are never covered, also in the end-of-race hold). Compare tool now
  runs every mode (`enh_compare_frame` for all draws_view modes; `enh_road_debug_rows` also prints flags and banks): at
  `--draw-distance-scale 1` strip 99.2 % edge match, all object calls and sizes matched; Mulholland front 97.1 % (30 of
  969 frames below 92 %, worst around seg 409-411 and 429-431; its mirror object calls are unmatched by design, native
  mirror). Render per displayed frame at 1280x800 2x2, idle: strip 7-8 ms, Mulholland 17.4 ms mean (34 max; the big hill
  polygons: candidate for a cheaper fill). Identity: `--classic --check`, `classic_check.py` plain (once a wall-clock
  divergence, ok on rerun) and `--clock` ok. Open: Mulholland mirror (S2.6), start tree / opponent natively (S2.5), table ends
  past the course for Mulholland (0x3DD) and the strip (0x58) at the extended distance (S2.9), the strip's `top_band_row`
  culling nuances.
- S2.6 rest (2026-10-30): the Mulholland mirror is drawn by the enhanced view (`rs.mirror_native` removed): `mir_hill_side` (mirror_hillsides, colour 0x0E, edges at the mirror clip 240 / 319), verge strips and bank-flat ground in `mir_band`, `mir_extras` (trusses, truss end, guard rail with the front's rail phase), the 20-row sky strip of `mirror_draw_backdrop`, Mulholland object types in the `enh_sprite.c` mirror() (0x15 / 0x16, bush groups, obstacle 0x14, no `L/R & 0x20` shapes). The mirror car was already native in every mode; the starter in the mirror now also in mode 0x10. Checks: `enh_compare.py` 21 ok; Mulholland compare run: object calls matched 12084/12084 (front + mirror), mirror edges h ~95 % / v ~95-96 %; `--classic --check` ok.
- S2.7 (2026-10-30): the Aqueduct (mode 8) is drawn by the enhanced view (`enh_aq.c`; `rs.draws_view` for every mode). The
  channel is straight with one cross-section per frame (the deformation of `aq_climb_left/_right`, now snapshotted with the
  wall-top shifts at `enh_frame_begin` and interpolated), so walls, floor, trough, openings, boxes and pillars are planar
  quads projected exactly with `road_proj_x/_y` (dx = heading, dy = cur_height - pitch) at continuous depth, then rotated
  about the far floor point of the original's row 14 (middle / left / right end while climbing) by the interpolated roll
  (x 255/256 as sin256; the roll pair blends the short way round). Far end 4 + 112 x scale; past track_end only joints,
  trough gaps and openings, past the table its pattern. Concrete below the far floor line (the original's no-roll fill;
  while rolling the original shows sky there). Outlines 0.4 px (1 px in the compare view), stars 2 output px, sky scroll
  1/8 (asked in U1). Format-B shapes from their unrotated variant 3 rotated by the roll; floor shapes from group 0 at
  z 8.5 with true 1/z (groups 2..6 are ~620/z, 0..1 ~345/z: smaller than classic mid-distance, the S2.5 rule). Car: the
  native unrotated picture of its level rotated by the continuous bank (`enh_prop_car_aq`, `ras_sprite_rot`), starter in
  the mirror rotated with the roll. Mode 8: raw heading interpolated (no YAW). Mirror: aq_project_mirror at continuous
  depth 2..56, the original's flags (band [b, b+1] = AQ[b - 1]), its stars and backdrop. The original's y truncation
  towards zero is emulated (+1 above the eye: wall tops at eye-level matched the original's row tables exactly in
  `SR_ENH_COMPARE=3`, which now prints `aq row` lines). Compare (`SR_ENH_COMPARE=2`, scale 1, Aqueduct run): edges h
  97.5 / 95.9 %, v 95.7 / 93.9 %, mirror h 94.3 / 96.7 %, v 94.3 / 97.4 %; the misses are rolled frames (sub-pixel
  rotation, the concrete beyond the far end), the car's continuous size and the pinned rows 12..14. Render 8.6 ms mean,
  15.9 ms max per displayed frame (1280x800 2x2, headless). Checks: `--classic --check` ok; `classic_check.py` plain and
  `--clock` ok on title, race_drag and the new `race_aq` path; `enh_compare.py` unchanged (all ok); faithful smoke 15/15
  with this exe. Not exercised headless: the mirror car (the demo opponent never fell behind), a hard wall bounce.

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
