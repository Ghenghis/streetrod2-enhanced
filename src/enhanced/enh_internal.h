/* Street Rod 2 Enhanced: declarations shared by the files of src/enhanced/ (ENHANCED.md §2). */
#ifndef ENH_INTERNAL_H
#define ENH_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

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
} enh_view;

/* YAW / HGT at a continuous position P (sub-steps), linear between segments; indices clamped to the track (§1). */
double enh_track_yaw(double P);
double enh_track_hgt(double P);
/* The interpolated view at host time `now_ns`; false when no snapshot exists (no race frame yet). */
bool enh_interp(uint64_t now_ns, enh_view *out);

#endif
