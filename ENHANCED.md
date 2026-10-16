# Enhanced render back end — design

Street Rod 2 Enhanced keeps the faithful engine (`ENGINE.md`) running unchanged: game flow, garage, Bob's, the race
logic and physics, the original road renderer, the dashboard and every other screen, at the game's own pace.
`src/enhanced/` records what each race frame is built from and draws the road view (and the rear-view mirror)
again on the host, at output resolution, for every displayed frame. The result is laid over the scaled EGA
picture where the original's own picture would show the road view. This is the approach of Street Rod 1 Enhanced
(`../StreetRodEnhanced/ENHANCED.md`, "SRE" below), itself from Test Drive II/III Enhanced (TD2E, TD3E).

Goals: **stage 1** high resolution and smooth motion at the original draw distance (14 segments ahead, 7 behind);
**stage 2** a longer draw distance. Status: design only (S1.1); nothing implemented yet.

References: RR = `../StreetRod2/port/spec/race_render.md` (SR2 delta spec), RACE = `../StreetRod2/port/spec/race.md`,
RR1 = `../StreetRod/port/spec/race_render.md` (SR1). Search them, never dump them. Line numbers refer to commit
1b3cfce of this repo.

## 1. What SR2 gives us (and how it differs from SR1)

* **Five race kinds, one road mode word `DS:ADD6`** (RR §1.1), each a separate case for the back end:

  | ADD6 | Race | Start / finish seg | Renderer |
  |---|---|---|---|
  | 0 | town cruise and event trips | dest. / – | normal |
  | 1 | street drag race | 200 / 0x17C | normal |
  | 2, 4 | Grudge Night drag strip (time run / rounds) | 8 / 0x58 | normal on `L2/R2/C2` (index +0xC0), flat (`dy = 0`), barrier walls, start tree, starter; opponent not drawn in mode 2 |
  | 0x10 | Mulholland Drive | 0x172 / 0x3C0 | normal + hillsides and drop-offs (`BL/BR`), own sky, truss bridges, guard rails, rocks, cliff fall, outer band 200 instead of 400 |
  | 8 | Aqueduct | 8 / 0x156 | **separate renderer** (`race_aq.c`): concrete channel of polygons, roll rotation, `AQ/AQM` flags, SR2_SHP format-B objects, rotated opponent sprites, night sky |

* **The whole course is in memory** (far segment 3726, RR §1.4): `L R YAW C HGT` (0x3DE = 990 entries each, SR1 1120),
  new `BL/BR` (bank height: > 0 hill or wall beside the road, < 0 drop-off), `L2/R2/C2` (0x74 entries, drag strip),
  `AQ/AQM` (0x172 entries, Aqueduct and its reversed copy for the mirror), `LANE` (obstacle lane 1..16). They are
  built at race start (`race_track.c`, the only writer) and read-only during the race, so the back end reads them
  directly and needs no history ring. The arrays are contiguous in one segment: reading past an end silently returns
  the next array, so every enhanced read is clamped (§5).
* **Sub-steps: one segment = 64 sub-steps** (`DS_substep` A560, SR1 8). The continuous position is
  `p = seg + substep/64`. The original's row depth uses `q = substep/8` truncated (RR `road_project_front`):
  row 0 at z = 4, rows 1..13 at `z = 8k + 4 - q`, row 14 pinned at 116. So the original still shows only 8 depth
  positions per segment; the enhanced road uses `z(k) = 8k + 4 - substep/8` in float (no truncation), and keeps
  row 0 (z = 4) only as the near clip.
* **Projection** (`race_road.c:72-107`, RR `062c/0684`): no rotation, evaluable in float at any depth:
  * `sx = 160 - 4·(cx - (x + z·dx/4)) / z`, `dx = heading - YAW[seg]`; in mode 8 `cx` is clamped to 0x564..0xD5C.
  * `sy = (2·z·dy + 4·(y - cam_y)) / z + bounce + 60`, `dy = cur_height - HGT[seg]` (SR2 halves the hill term of SR1
    and adds `DS_view_bounce_y` 9A56, the Aqueduct wall bounce).
  * Mirror: `road_proj_x_mirror/y_mirror` (centre 0x118, no ×4); mirror rows z = 8 at row 0, then `q + 8k` (k ≤ 4),
    `8k` (k ≥ 5, an original step that pops); mirror row r reads the flags of `seg - r - 1`.
  * Row values that the enhanced geometry needs (RR §1.5, eleven row tables): road edges (from `C & 0xE0`: normal
    0x5DC/0xCE4, bridge 0x7D0/0xAF0, one-sided closures), road y (0x7D0), kerb y (0x79E), eye y (0x76C, now every
    row), outer edges (`0x5DC - w`, `0xCE4 + w`, w = 200 Mulholland / 400 else), bank tops (`0x7D0 - BL/BR`), far
    terrain x. Row-0 quirks (Mulholland `dx` clamp ±0x30, row-0 outer-edge formula) are kept only in the compare tool.
* **Frame order** (`road_frame`, `race_road.c:1434-1456`): project front → draw front → project mirror (overwrites
  rows 0..7 of the same tables) → draw mirror → `road_fillmode_end` → hood overlay → starter slide (racing, not drag
  strip) → A-pillar overlay → `road_page_flip` → `pace_ticks(8)`. Mode 8 uses `aq_project_*`/`aq_draw_*` instead.
* **Pacing.** `pace_ticks(8)` (`platform/gfx.c:211`) is a deadline metronome: 8 ticks of the 72.8 Hz race tick
  (PIT divisor 0x4000, `platform/timer.c:213`) ≈ 9.1 frames a second, a late frame shortens the next wait. With
  AdLib music the tick may be chained at 68.6 or 60 Hz (`ENGINE.md` Timing). So frame times are irregular: the
  snapshot carries the host time of its tick, not an assumed period.
* **What moves when** (verified in `race_drive.c:381-394`, `race_phys.c:827-868`, `platform/timer.c:96-115`):
  * per frame, in order: `road_step` (heading nudge from steering, `road_frame`, then lateral `cam_x` drift), then
    `race_logic` → `race_step` → `player_advance` (`substep += step_per_frame`, segment carry) and
    `opp_advance_steer` (opponent segment, fraction and lateral position);
  * every 12th tick, in the race ISR: `race_phys_step` (speed, rpm, gear, wheel, edge collision with bounce of
    `cam_x`/heading in Mulholland and the drag strip). Speed only changes `step_per_frame`, which is used once per frame.
  So the car positions change once per game frame; one snapshot per `road_frame` is enough (SRE needed a second
  per-physics-step snapshot; SR2 does not).
* **The original's draw code feeds the simulation**: `opponent_draw` (`race_opp.c:134`) writes the contact and
  alongside flags and the sprite hysteresis state; `front_objects` / `obstacle_hit_test` (`race_road.c:1139`) set
  the rock and obstacle latches (`DS_rock_hit` BB88, `DS_obstacle14_hit` ADD4) that `road_edge_collision` reads.
  **The faithful draw must keep running every frame, unchanged.** The enhanced view is an overlay, never a
  replacement of the original's draw calls.
* **Video**: VGA pages A000 (page 0: dashboard, shown below the split), A200, A400 (race pages, flipped by
  `road_page_flip`, `race_road.c:251`); split at row 0x63 (`race_drive.c:354`); view clip (0,18)-(319,101), mirror
  clip (240,22)-(319,42). Drawing goes through the 20-slot driver table `DS:A0AE` (`platform/drv_ega.c:49-107`):
  blit 0, masked blit 2, line 12, hspan 13 (colour from slot 17 set_colour), fill rows 14, shrink 15, rotate 16,
  **poly_fill 18** (fill and edge colour), plot_points 19. **Every VRAM write ends in `vga_wr` / `vmovedata`
  (`platform/vga.c:135`, `:169`)**: one choke point for the coverage mask.
* **Pictures**: LIB3 pictures (`pic_get`, id ≥ 2000 → LIB3); the opponent sprite sets per class from SR2_DATA block 8
  (side, rear 3/4, rear, front; 5 classes + police, RR §1.7), shrunk by the block-7 masks to 13 levels (0..12);
  Mode 8 rotates them to 7 angles (`race_load_graphics_rot`, `race_aq.c:871`). Road-side objects are **SR2_SHP vector
  span shapes** (RR §1.8: 7 LODs of `{dx, dy, w}` spans, LOD = clamp(9 - depth, 0, 6); format A drawn by
  `shape_draw`, `race_gfx.c:408`; format B groups by `shp_b_draw`, `:429`), not SR1's hline shapes.

## 2. Architecture

**Approach.** Re-render from world state (SRE), not from the driver calls. Hooking the driver slots alone (poly_fill,
hspan, blit) would give crisp edges but keep the original's integer 320×200 coordinates, its 8 depth steps per
segment and its 9 frames a second: no smooth motion and no draw distance. The driver slots are used only to tell
layers apart (coverage).

**Files in `src/enhanced/`**

| File | Contents |
|---|---|
| `enhanced.c` / `.h` | options, hooks, snapshot ring, interpolation, compose, coverage mask, layers |
| `enh_road.c` | float road geometry for the normal renderer (modes 0, 1, 2/4, 0x10): strips, banks, hillsides, walls |
| `enh_aq.c` | float Aqueduct geometry (mode 8): channel, walls, openings, boxes, pillars, roll |
| `enh_raster.c` | sample buffer, polygon fill with coverage AA, resolve through the live palette, worker bands |
| `enh_sprite.c` | LIB3 picture decode, SR2_SHP shapes at continuous scale, sprite scaling / rotation, mips |
| `enh_internal.h` | shared declarations |

**Hooks** (`ENH:` in the engine; each a single call, a no-op when `enh.classic` or when the overlay is off)

| Hook | Site (commit 1b3cfce) | Purpose |
|---|---|---|
| `enh_parse_arg()` | `main.c:58` (exists) | options |
| `enh_init()` | `main.c:75`, after `host_init` | validate options, allocate the output frame, register the compose hook |
| `enh_race_begin()` | `race_drive.c:340`, after `shp_load_set()` | decode LIB3 sprites and SR2_SHP sets at native size, note the road mode, overlay on, snapshot ring reset |
| `enh_race_end()` | `race_drive.c:396`, before `arena_reset()` (all exits of the loop pass here) | overlay off, free per-race data |
| `enh_frame_begin()` | `race_road.c:1436`, first line of `road_frame` | snapshot of the frame's state (§3.1) with its tick time; start of the **background layer** |
| `enh_layer_fg()` | `race_road.c:1450`, before `road_fillmode_end()` | end of the background layer: hood, starter slide, A-pillar and everything after are **foreground** |
| `enh_objects_begin/end`, `enh_mirror_objects_begin/end`, `enh_obj_orig` | around `front_objects` and the mirror's banner + `mirror_objects` (`race_road.c`), in `scenery_draw_*` | the enhanced view owns the objects' pixels (S2.5, S2.6), else passthrough; compare records |
| `enh_opp_drawn(view, s, v, drawn)` | end of `opponent_draw`, `race_opp.c:134-…` | which sprite level / variant the original chose (front or mirror call) |
| `enh_frame_presented()` | `road_page_flip`, `race_road.c:258`, **before** `ega_show_page` | which page now carries this snapshot (the host may present during the retrace wait inside `ega_show_page`) |
| `enh_vram_write(off, before, after)` | `vga_wr` / `vmovedata`, `platform/vga.c:135`, `:169` | per-pixel written mask (§2.1) |
| `enh_compose()` | `platform/ega.c:19` compose | scale the EGA picture to the output frame, then draw the enhanced view and mirror where coverage allows |
| fall-off / bounce snaps | `road_fall_off_edge`, `race_road.c:1597` (calls `road_frame` 8×), `aq_wall_bounce` | nothing extra: each `road_frame` snapshots; the discontinuity test snaps |

**Host changes** (`host.h:24-25`, `host.c:101-110`, `:543`, `:741`)

* Raise `HOST_FRAME_MAX_W/H` to 2560 × 1600 (320 × 8 by 200 × 8). `ega.c` registers the frame at
  `320·res_scale × 200·res_scale` when Enhanced graphics are on; `--classic` registers 320 × 200 exactly as now.
* Separate the **logical** size (320 × 200, used for the 4:3 presentation and for the mouse mapping at
  `host.c:741-742`) from the pixel size of the frame texture. Text mode (640 × 200) stays as it is.
* While the overlay is on, compose reports a change on every call, so the host presents at the display rate
  (VSync; the 8 ms gate at `host.c:543` stays).

### 2.1 Coverage by layer

A pixel of the view window shows the enhanced picture only if all of these hold:

* the shown page is a race page presented by `road_page_flip` with a snapshot (`enh_frame_presented`);
* the pixel is in the view rectangle and above the split (rows 18 .. min(101, split − 1)), the mirror included
  (since S2.6 the enhanced view draws the mirror in modes 0 and 1; before, its rectangle stayed the original's);
* its **written bit** is clear. Per page (A200, A400) one bit per pixel:
  * background-layer writes (between `enh_frame_begin` and `enh_layer_fg`) **clear** the bits they touch: the
    enhanced view owns those pixels;
  * foreground-layer writes, and every write outside a race frame (message boxes, "pulled over", results, the
    start sequence's text), **set** the bits of the pixels whose value changed (masked blits rewrite whole bytes;
    comparing before/after per pixel avoids punching byte-wide holes, SRE);
  * last writer wins per pixel, so painter's order is kept.

The hood (`road_draw_hood_overlay`, `race_road.c:960`), the starter slide and the A-pillar (LIB3 #2003) are native
pictures in the foreground: shown as the original's pixels, scaled nearest-neighbour. They look the same as a scaled
redraw and need no special case. The dashboard (page 0 below the split) is never covered.

**Passthrough for unported elements.** An element drawn by the original inside the background layer that the
enhanced renderer does not draw yet is bracketed with `enh_layer_fg()` / `enh_layer_bg()` at its call site: its
pixels then show as the original's. Each sprite family becomes enhanced by removing its bracket. Interim only: the
passthrough pixels are the current frame's while the enhanced road is interpolated one frame behind (§3.1).

## 3. Stage 1 — high resolution, smooth motion, original draw distance

### 3.1 Snapshots and interpolation

Snapshot per `road_frame` (ring of 8), all read from DGROUP at `enh_frame_begin`:

| Group | Fields |
|---|---|
| time | host time (ns) of the tick `DS_ticks` at the snapshot; frame counter |
| mode | `DS_road_mode` (ADD6), `DS_racing` (B7AE), `DS_start_seg`, `DS_track_end`, `DS_fallen`, `DS_fall_dir` |
| player | `DS_seg`, `DS_substep` (64ths), `DS_step_per_frame`, `DS_cam_x`, `DS_cam_y`, `DS_heading`, `DS_cur_height`, `DS_view_bounce_y`, wheel (`PH_WHEEL`) |
| opponent | `DS_opp_seg`, `DS_opp_seg_frac64`, `DS_opp_lat`, the lateral offset used by `opponent_draw`, sprite level / variant / drawn per view (from `enh_opp_drawn`), `DS_opp_model` class |
| police | police state, segment, lateral position, light phase (police sprite set `DS_police_sprites`) |
| start | `DS_start_light_pos` (A2B4), start lights / tree lamp state, starter position |
| Aqueduct | `DS_rot_cos`, `DS_rot_sin`, `DS_rot_cx`, `DS_rot_cy` (roll), `DS_aq_pitch`, `DS_aq_dy`, saved floor / wall-top heights and wall shifts used by `aq_project_*` |
| palette | the 16 attribute registers + overscan (the opponent's colours are set per race, `opp_palette_set`) |
| compare | `DS_top_band_row`, the shown page |

Interpolation:

* Render time = now − `motion_delay` (default: one game frame, 110 ms). Blend the two snapshots around it with
  smoothstep on the fraction (TD3E); between game frames everything moves at the speed of the last pair.
* Continuous positions: `P = seg·64 + substep` for the player and `O = opp_seg·64 + frac` for the opponent,
  interpolated linearly in sub-steps. YAW, HGT and BL/BR are interpolated linearly between segments at any `P`.
* Heading: interpolate only the **relative** heading `heading − YAW(P)` and add `YAW` at the interpolated position
  (SRE lesson: delaying the whole heading swings the view into bends). `cam_x` linear.
* Snap (no blend) on: race start, a mode change, `road_fall_off_edge` frames, an Aqueduct wall bounce, a jump of ≥ 2
  segments, a heading step ≥ 16, a `cam_x` step ≥ 0x100 (edge bounce).
* The sprite level / variant chosen by the original is taken from the newer snapshot (hysteresis is the original's).

### 3.2 Road geometry (normal renderer)

* For each segment from `P` forward to `seg + 14` (stage 1), compute the world edges: road (from `C & 0xE0`),
  kerb, outer band, bank tops (`BL/BR`), far terrain; centre dashes; finish band; crossings; town walls.
* Subdivide each segment into 4–8 slices with interpolated YAW / HGT / banks; project in float with the formulas of
  §1 at the output scale; emit polygons far to near in the original's painter's order (sky, then per row: Mulholland
  hillsides, bands, objects, opponent, drag-strip barriers).
* Mode specifics: modes 2/4 read `L2/R2/C2` at `seg + 0xC0` and use `dy = 0`; Mulholland uses w = 200, hillsides
  (`front_hillsides`, `race_road.c:1085`, whose per-row peak tables at DS 7E1E.. are interpolated by depth), the cliff
  drop-offs (BL/BR < 0) and guard rails.
* `top_band_row` culling (crests) becomes a per-sample depth test: far road behind a crest is hidden by painter's
  order. The original's value is kept in the snapshot for the compare tool.

### 3.3 Aqueduct (mode 8)

Same snapshots, own geometry (`enh_aq.c`) from `aq_project_front/mirror` (`race_aq.c:163`, `:198`) evaluated in
float per slice: floor, trough, wall tops, wall openings (`AQ` bits 0x04/0x100, 0x08/0x200), boxes (0x10), pillars
(0x80), joint lines (0x01), white band (0x400), format-B objects (0x20/0x40). The roll (`rotate_about_centre`,
`race_gfx.c:61`) is applied in float to all projected points with the interpolated roll angle. Night sky and stars
redrawn at output resolution. The rotated opponent comes from the native picture rotated by the continuous roll
angle, not from the 7 pre-rotated sets.

### 3.4 Raster, anti-aliasing, colour

* Samples are palette indices at `res_scale × aa` (default 1280 × 800 with 2 × 2); resolve in linear light
  through the snapshot's palette registers.
* Extended colours (linear-light mixes of palette entries, following palette changes) are allowed for AA and, in
  stage 2, haze.
* The edge colour of `poly_fill` (slot 18, e.g. concrete 5 with edge 2 in the Aqueduct) becomes a 1-output-pixel
  outline, so the look survives at high resolution.
* Rendering on worker bands only if one core misses the budget (§6).

### 3.5 Sky and horizon

`road_draw_sky` / `road_draw_sky_mulholland` (`race_road.c:441`, `:488`) at native size scaled; the scroll follows
the interpolated heading without the original's rounding to 8 pixels (`sky_scroll`, `race_road.c:428`). SRE found
the sky racing across the screen when steering and slowed it to 1/8; use `SKY_SCROLL` = 1/8 of the original rate from the start
(user decision 2026-10-15). Horizon y from `road_proj_y(0xF0, …)` in float.

### 3.6 Sprites

* **SR2_SHP format-A shapes** (road-side objects, hydrants, bush groups, banner text): draw LOD 0 (the most detailed)
  scaled by `1/z`, matched so the size equals the original LOD at the original row depths (TD3E fit). A span
  `{dx, dy, w}` becomes a rectangle at output scale.
* **Signs, banners, trusses, guard rails, rocks, the start tree and the starter**: from their LIB3 pictures or as
  polygons, continuous scale.
* **Opponent and police**: the native LIB3 picture of the level the original chose (side / rear 3/4 / rear /
  front, block 8), scaled to the size of the original's shrunk level at the interpolated depth (at most 1.3 × native,
  SRE). Mode 2 draws no opponent (as the original).
* Mip levels for downscaling from the most frequent pattern, not by averaging indices (TD2E `build_mips`).
* Order of porting = order of passthrough removal: shapes → signs and banners → opponent and police → start props.
* Done for modes 0 and 1 (S2.5 part 1, `enh_sprite.c`): `front_objects` re-made from the track flags at the view; in
  the town the objects behind the building line (XOL / XOR and beyond) are drawn before their segment's walls, which
  the original paints them over (the original draws a row's objects after its walls). Hooks: `enh_objects_begin/end`
  around `front_objects` (a passthrough bracket in the modes not done yet), `enh_obj_orig` in `scenery_draw_*`
  (compare records).
* **Largest source only, true perspective** (user feedback 2026-10-16: the first version interpolated between the
  original's LOD sizes, so objects showed the steps and ballooned on approach). Every object is drawn from its largest
  source: shapes from the largest LOD l = min(n - 1, clamp) of their set, which the original uses at row depth 9 - l
  (boundary z = 8 (9 - l) + 4 - q, on average zt = 8 (9 - l) + 0.5: 24.5 for 7-LOD sets, 16.5 for the hydrant), scaled
  by zt / z about the anchor; a far object the original draws `off` rows smaller (flag 2 trees, L / R 0x200) by
  zt / (z + 8 off); signs and billboards from the sheet's largest size (64 x 43, row depth 3) by 24.5 / z. So the size
  equals the original's at the largest LOD's row and is the true 1/z everywhere else (larger than the original nearer,
  where it keeps LOD 6, and farther, where its LODs 0..2 are only 45..75 % of 1/z: the cause of the fast growth).
  Where the original draws nothing (a depth whose LOD the set lacks) nothing is drawn. Pop-in: the original's row 10
  would show a 1/z object at twice its LOD 0 size; the enhanced view draws the objects out to the road's far end (row
  14, z <= 116) and fades them in over the last segment (screen-door coverage `ras_alpha`, resolved by the AA average).
  Compare (`SR_ENH_COMPARE=2`): the compare view still draws the original's LOD and rows < 11 (edge match and call
  match: kind, type, depth, side, anchor +-1 px); the size check measures the spans the live view would emit at the
  original's row depth against the largest LOD's height x zt / z (within 0.5 px).

### 3.7 Mirror

The same renderer with the mirror projection, 7 rows behind, clipped to (240,22)-(319,42); background from the
original's mirror backdrop (`mirror_draw_backdrop`, `race_road.c:969`) scaled; frame lines redrawn at output
resolution. Intentional deviations (recorded, visible in the compare tool): continuous mirror depth instead of the
k ≥ 5 step, banks read behind the car (the original reads `BL/BR` forward in the mirror, RR `0ce4`).

Done for modes 0 and 1 (S2.6, `enh_road.c` `mirror`): projection `x = 280 + (YAW − heading) / 4 − (cam_x − x) / z`,
`y = 36 + (HGT − height) + (y − cam_y) / z` with z = (P − Pw) / 8 ≥ 8 (row 0); row r between boundaries seg − r
and seg − r + 1 with the flags of segment seg − r − 1 (the original's flag pointers); backdrop pictures from page 2's
store (frame 2005 rows 20..44 and its mount rows 18..19, the 2-row horizon strip of 2063 above the far row), the town
walls (C & 1 only; tall part half height left, full right, as the original), bands, dashes, the banner and
`mirror_objects` with the rules of §3.6 (size depth = 4 × the mirror's z), frame lines. The objects behind the
building line go before the row's walls as in the front view. The opponent / police car and the starter stay the
original's pixels (passthrough brackets in `road_draw_mirror` and `mirror_objects`). Hooks:
`enh_mirror_objects_begin/end` around the banner and `mirror_objects`. Compare: the mirror rectangle is edge-matched
apart (`M` lines) with the original's depths and banks; the original's nearest mirror rows step by up to 15 px per
pixel row (spans at the row's top), so the town reaches 95.6-98.8 % there (threshold 95 %).

### 3.8 Town cruise

Mode 0 uses the normal renderer and data: enhanced in stage 1, switchable separately (`--enh-modes`).

### 3.9 Options

| Option | Default | Notes |
|---|---|---|
| `--classic` | off | everything off, faithful picture and frames |
| `--res-scale 1..8` | 4 | output 320·N × 200·N |
| `--aa 1..4` | 2 | samples per axis; `res_scale × aa ≤ 16` |
| `--motion-delay MS` | one game frame (110) | 0 = latest snapshot, no interpolation (for alignment checks) |
| `--enh-modes LIST` | `town,drag,strip,mulholland,aqueduct` | modes drawn enhanced; the others stay original |
| `--draw-distance N` | 60 (stage 2) | 14..120 segments |
| `--draw-distance-scale X` | 1.25 | 1..4: the draw distance as a multiple of the original's 14 segments (S2.9 part 1, modes 0, 1: 17.5 segments); 1 = the original's; picture only; S2.9 generalises it into `--draw-distance` |
| `--haze 0..100` | 30 (stage 2) | |

Launcher (S3.2): a "Picture" group with Graphics (enhanced / original), Resolution, Anti-aliasing, Motion, and in
stage 2 Draw distance and Haze.

### 3.10 Stage 1 acceptance

1. `--classic`: `sr2-enhanced --classic --check` ok; a headless `SR_SNAPSHOT_DIR` run is frame-for-frame identical to
   the faithful build (`../StreetRod2/tools/sr2_smoke.py` checks pass); `grep -rn "ENH:" src` lists only the hooks of
   §2.
2. Simulation unchanged: with the compare clock (§5) a scripted race (`SR_KEYS`) gives the same `SR_DEBUG_RACE` log and
   the same EGA pages with and without Enhanced graphics, in each mode.
3. Geometry: with `SR_ENH_COMPARE` at `--motion-delay 0`, the enhanced view box-filtered to 320 × 200 matches the
   original's frame: road edge rows within ±1 px, objects and cars at the same row and size ±1 px, in modes 0, 1, 2/4,
   0x10 and 8 (straights, crests, S-bends, bridges, Mulholland banks, Aqueduct roll).
4. Smoothness: `SR_ENH_TRACE` shows no backward step of the camera between snaps, and per-display-frame jerk below a
   threshold set in S2.2.
5. Coverage: start sequence, message boxes, "pulled over", finish, crash and results screens, the cliff fall and race
   exit show the original's pixels; no enhanced pixels outside a race.
6. Cost: render time per displayed frame at 1280 × 800, 2 × 2 measured and reported; target ≤ 8 ms on one core.
7. U1 visual check by the user at each milestone.

## 4. Stage 2 — longer draw distance

1. **`--draw-distance N`** (default 60, 14..120): road to `seg + N`, fewer slices further away; one slice when a
   segment is under one sample tall. `top_band_row` is replaced by a horizon clip; crests occlude in painter's order.
2. **Table ends** (never read past an array; the arrays are contiguous, §1):
   * normal road: last entry 0x3DD. Mulholland finish 0x3C0 + 60 passes it: beyond 0x3DD synthesise flat straight
     road with the last YAW / HGT, no banks, no objects;
   * street drag race: past 0x17C the table holds the rest of a road course: plain flat road, no objects;
   * drag strip: `L2/R2/C2` have 0x74 entries (index `seg + 0xC0` maps into the main arrays); past the finish 0x58
     draw the strip's flat road and barriers only;
   * Aqueduct: `AQ` has 0x172 entries, finish 0x156: continue the plain channel;
   * town (below 200): never show race-course segments; clamp the town view at the last town segment
     (check the exact town end in `track_build_all`, `race_track.c:286`);
   * mirror: never read below segment 0 (drag strip starts at 8).
3. **Scenery at distance**: the original draws objects only in rows 1–10 (`race_road.c:1284`); extend to N with the
   same continuous scale, fade in over the last 10 % with a dithered alpha, drop sprites under one sample.
4. **Opponent and police at distance**: the original shows them up to 14 segments; draw from the continuous position
   up to N with the rear picture scaled down. The game logic is unchanged (police spawn, siren at `opp_seg + 15`,
   pull-over, abort distance 7, RACE §0).
5. **Haze** `--haze 0..100`: blend towards the horizon colour with smoothstep from about 20 segments to N, using
   extended colours. Mulholland drop-offs and the Aqueduct night sky need their own horizon colour.
6. **Horizon consistency**: far road never draws above the sky panorama except on crests.
7. **Mirror distance**: 7 rows in the original, raised to `min(N/2, 30)`, clamped at segment 0.
8. **Cost**: measure at N = 60 and 120; target < 4 ms a frame with worker bands.

Stage 2 acceptance: in every mode with N ∈ {14, 60, 120}: no popping at the far edge, no road over the sky, no
reads past a table (assert in debug builds), sane road past each finish, the opponent visible far ahead and matching
the original's picture from 14 segments in.

## 5. Test tooling

* Existing: `SR_KEYS`, `SR_SNAPSHOT_DIR`, `SR_SNAPSHOT_MS`, `SR_DEBUG_RACE`, `SR_TRACE` (`host.h`), the faithful
  `tools/sr2_smoke.py`.
* `SR_ENH_COMPARE=1|2`: a deterministic host clock (fixed ticks per pump, so classic and enhanced runs are
  tick-identical) and side-by-side dumps of the original and the box-filtered enhanced view of the same frame, plus a
  text dump of the row tables vs the float rows.
* `SR_ENH_TRACE`: per displayed frame the interpolation fraction, delay, camera jerk and render time.
* `SR_ENH_RACE=mode[,seg]`: start straight into a race of a given mode (developer aid, like the faithful `SR_DEBUG_*`).

## 6. Order of work and milestones

1. Host and coverage: output frame size, logical vs pixel size, compose hook, written mask and layers, overlay on/off,
   options. The picture is the scaled EGA picture (no visible change). `--classic` identity checked.
2. Snapshots and interpolation with `SR_ENH_TRACE` and the compare clock. Nothing drawn yet.
3. **Milestone A — first enhanced picture:** road geometry, raster, resolve and sky for the town and the street drag
   race (modes 0 and 1) at draw distance 14, every sprite as passthrough, plus `SR_ENH_COMPARE`. Accepted when the
   compare is within ±1 px and the user sees a smooth, sharp road (U1).
4. Mulholland (banks, hillsides, drop-offs, guard rails, trusses) and the drag strip (flat, barriers).
5. Sprites: SR2_SHP shapes, signs and banners, opponent and police, start props. No passthrough left.
6. Mirror.
7. Aqueduct.
8. Performance pass (worker bands if over budget). **Stage 1 done.**
9. Draw distance, table ends, far scenery, opponent at distance, haze, mirror distance. **Stage 2 done.**

## 7. Risks and lessons carried over

* **Draw code with side effects** (§1): never skip or reorder the original's draw; the enhanced renderer only reads.
* **Game speed**: the pace stays 8 ticks a frame; smoothness comes only from interpolation.
* **Delayed heading**: delay only the relative heading (§3.1).
* **Shimmer from coverage timing**: coverage refers to the page actually presented (`enh_frame_presented` before
  `ega_show_page`, since the host presents inside its retrace wait), not to the back page being drawn.
* **Object size jumps** where the original switches LODs or sprite levels: the continuous scale must pass through the
  original sizes at the original depths.
* **Passthrough lag**: interim passthrough sprites are one frame ahead of the interpolated road (§2.1); acceptable until
  step 5, check alignment with `--motion-delay 0`.
* **Aqueduct roll**: the original rotates integer points about a pivot with 8.8 sin/cos; a float roll with the
  interpolated angle may differ by a pixel at the walls. The compare tool tolerates ±1 px.
* **Irregular frame times** (deadline pacing, AdLib-chained tick): interpolation uses measured snapshot times.

## 8. Decisions (confirmed by the user 2026-10-15; sky scroll 1/8)

1. Defaults 1280 × 800 with 2 × 2 AA; stage 2: draw distance 60, haze 30.
2. All five race kinds enhanced (`--enh-modes` all), the Aqueduct last.
3. Extended colours allowed from stage 1.
4. Past a finish line: plain road (or plain channel) with no objects.
