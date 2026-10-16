/* Street Rod 2 Enhanced: declarations shared by the files of src/enhanced/ (ENHANCED.md §2). */
#ifndef ENH_INTERNAL_H
#define ENH_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "types.h"

/* One snapshot per road_frame (ENHANCED.md §3.1), read from DGROUP at enh_frame_begin; the Aqueduct group and the
 * row of the horizon band are computed by the frame's projection and read at enh_layer_fg. */
typedef struct {
    /* time */
    uint64_t t_ns;              /* host clock of the tick at the snapshot (host_tick_ns) */
    u32 frame;                  /* road_frame counter since race start */
    u32 ticks;                  /* DS_ticks */
    /* mode */
    s16 mode, racing, start_seg, track_end, fallen, fall_dir;
    /* player */
    s16 seg, substep, step, cam_x, cam_y, heading, cur_height, bounce_y, wheel;
    /* opponent (also the police car once police_spawn ran) */
    s16 opp_seg, opp_frac, opp_lat, opp_lat_step, opp_model, opp_sprite, opp_variant;
    s8 opp_s[2], opp_v[2];      /* per view (0 front, 1 mirror): sprite level / variant chosen, -1 = no call */
    bool opp_drawn[2];          /* the original blitted it */
    /* police, start */
    s16 police_state, police_seg;
    s16 start_light_pos, start_lights, light_latch, starter_seg;
    /* Aqueduct (at enh_layer_fg) */
    s16 roll, rot_cos, rot_sin, rot_cx, rot_cy, aq_pitch, aq_dy;
    s16 floor_h, top_h, xl, xr, wall_l_shift, wall_r_shift, opp_bank, wall_hit_shift;
    /* palette: 16 attribute registers + overscan */
    u8 pal[17];
    /* compare */
    s16 top_band_row;           /* at enh_layer_fg */
    s8 page;                    /* race page (1, 2) that presented this frame, 0 = not yet */
} enh_snap;

/* The interpolated view at a render time. Continuous values are blended; everything discrete (mode, sprite levels,
 * palette, start lights) is the newer snapshot's (`b`). */
typedef struct {
    const enh_snap *a, *b;      /* the pair around the render time (a == b with one snapshot) */
    uint64_t render_ns;         /* host clock time shown: now - motion delay */
    double x;                   /* position of the render time in the pair, 0..1.5 (> 1 = extrapolated) */
    double w;                   /* smoothstep weight of the pair against the previous pair's line (1 = pair only) */
    bool snap;                  /* the pair is discontinuous: b is shown unblended */
    double P;                   /* player position in sub-steps: seg * 64 + substep */
    double rel_heading;         /* heading - YAW(P) */
    double heading;             /* YAW(P) + rel_heading (raw YAW units, not wrapped) */
    double cam_x, cam_y, cur_height, bounce_y, wheel;
    double O, opp_lat;          /* opponent position in sub-steps, lateral position */
    double roll, rot_cx, rot_cy, aq_pitch, aq_dy;
    bool compare;               /* the compare view (SR_ENH_COMPARE >= 2): the original's constant-per-row details */
} enh_view;

/* YAW / HGT at a continuous position P (sub-steps), linear between segments; indices clamped to the track (§1). */
double enh_track_yaw(double P);
double enh_track_hgt(double P);
/* The interpolated view at host time `now_ns`; false when no snapshot exists (no race frame yet). */
bool enh_interp(uint64_t now_ns, enh_view *out);

/* ---- raster (enh_raster.c, §3.4): palette-index samples of the view rows at S = res_scale * aa per logical pixel */
#define ENH_VIEW_Y0 18              /* the original's view clip: rows 18 .. 101 */
#define ENH_VIEW_Y1 102
#define ENH_SPLIT_Y 0x63            /* race split (race_drive.c): rows >= 99 show the dashboard page */
bool ras_begin(int S);
int ras_S(void);
u8 *ras_row(int j);                 /* sample row j (0 = top of logical row ENH_VIEW_Y0), 320 * S samples */
void ras_poly(const double *xy, int n, u8 colour);                  /* convex polygon, logical coordinates */
void ras_rect(double x0, double y0, double x1, double y1, u8 colour);
void ras_resolve(const u8 pal[17], const u8 *cover, u32 *out, int res_scale, int aa);
void ras_image(const u8 *px, int w, int h, double x0, double y0, double x1, double y1, double ymax);
void ras_downscale(u8 *idx);       /* 320 x (ENH_VIEW_Y1 - ENH_VIEW_Y0) indices, most frequent per pixel */
u32 enh_pal_rgb(u8 attribute_reg);

/* ---- road and sky (enh_road.c, §3.2, §3.5) */
void enh_sky_capture(void);         /* page 1 rows 0x66.. (sky, panorama), at the first frame of a race */
bool enh_road_render(const enh_view *v);    /* false: not drawn in this mode (the original's picture stays) */
void enh_road_debug_rows(const enh_view *v, FILE *f);    /* compare aid (SR_ENH_COMPARE=3) */

/* ---- road-side objects (enh_sprite.c, §3.6): front_objects of one boundary at the view */
typedef struct {
    int s, k;                   /* segment, the original's row (s - floor(P / 64)) */
    double z, rc;               /* depth of the boundary; continuous row for sizes (= k when comparing) */
    /* the original's row values at boundary s (its integers when comparing, else continuous estimates of them) */
    double xl, xr, xol, xor_, xfl, xfr, y, ye, y16;    /* y16: 4 * ye - 3 * y (banner top) */
    double dash;                /* dash width of the row (banner posts) */
    bool flat_l, flat_r;        /* Yq == YT1q, Yq == YT2q (no bank at the row in front) */
} enh_bound;
enum { ENH_OBJ_ALL, ENH_OBJ_OUT, ENH_OBJ_IN };      /* all; behind the building line; between road and building line */
void enh_objects(const enh_bound *b, int mode, bool compare, int pass);

/* ---- compare (enh_compare.c, §5): SR_ENH_COMPARE >= 2 */
/* object records: kind 0 shape, 1 hydrant, 2 sign picture, 3 banner; orig = recorded from the original's calls */
void enh_compare_obj(bool orig, int kind, int type, int depth, int side, double x, double y, double x2);
void enh_compare_size(double err, double tol);
void enh_compare_objs_clear(void);      /* a new frame: the original's records start again */
void enh_compare_init(int level);
void enh_compare_frame(const enh_snap *s, int page);
void enh_compare_race_end(void);
bool enh_mirror_px(int x, int y);   /* (x, y) belongs to the mirror (the original's until S2.6) */

#endif
