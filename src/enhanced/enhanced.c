/* Street Rod 2 Enhanced: options, hooks, compose, written mask and layers (ENHANCED.md §2, §2.1, §3.9), snapshots
 * and interpolation (§3.1).
 * S2.1: the host and coverage foundation. The output frame is the EGA picture scaled nearest-neighbour; the
 * enhanced view (later subtasks) is drawn by enh_compose where coverage allows.
 * S2.2: a snapshot per road_frame and the interpolated view at the display rate (SR_ENH_TRACE).
 * S2.3: the enhanced view of the town and the street drag race (enh_road.c, enh_raster.c) laid over the scaled EGA
 * picture where coverage allows; the compare tool (enh_compare.c). */
#include "enhanced.h"
#include "enh_internal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "host.h"
#include "game/race_int.h"
#include "platform/ega.h"
#include "platform/vga.h"

enh_options enh = {
    .res_scale = 4,
    .aa = 2,
    .motion_delay = 110,
    .modes = ENH_MODE_ALL,
};

/* ---------------------------------------------------------------- options */

static bool parse_int(const char *v, int lo, int hi, int *out)
{
    if (!v || !*v) return false;
    char *end;
    long n = strtol(v, &end, 10);
    if (*end || n < lo || n > hi) return false;
    *out = (int)n;
    return true;
}

static bool parse_modes(const char *v, unsigned *out)
{
    static const struct { const char *name; unsigned bit; } names[] = {
        { "town", ENH_MODE_TOWN }, { "drag", ENH_MODE_DRAG }, { "strip", ENH_MODE_STRIP },
        { "mulholland", ENH_MODE_MULHOLLAND }, { "aqueduct", ENH_MODE_AQUEDUCT },
        { "all", ENH_MODE_ALL }, { "none", 0 },
    };
    if (!v) return false;
    unsigned m = 0;
    char buf[128];
    snprintf(buf, sizeof buf, "%s", v);
    for (char *t = strtok(buf, ","); t; t = strtok(NULL, ",")) {
        size_t i;
        for (i = 0; i < sizeof names / sizeof names[0]; i++)
            if (!strcmp(t, names[i].name)) { m |= names[i].bit; break; }
        if (i == sizeof names / sizeof names[0]) return false;
    }
    *out = m;
    return true;
}

int enh_parse_arg(const char *arg, const char *val)
{
    if (!strcmp(arg, "--classic")) { enh.classic = true; return 1; }
    if (!strcmp(arg, "--res-scale")) return parse_int(val, 1, 8, &enh.res_scale) ? 2 : 0;
    if (!strcmp(arg, "--aa")) return parse_int(val, 1, 4, &enh.aa) ? 2 : 0;
    if (!strcmp(arg, "--motion-delay")) return parse_int(val, 0, 1000, &enh.motion_delay) ? 2 : 0;
    if (!strcmp(arg, "--enh-modes")) return parse_modes(val, &enh.modes) ? 2 : 0;
    return 0;
}

static FILE *trace;         /* SR_ENH_TRACE=file ("-" = stderr): snapshots and interpolated frames (§5) */
static struct { uint64_t n, sum, max; } rstat;  /* enhanced render time, per race (SR_ENH_TRACE) */
static int compare;         /* SR_ENH_COMPARE: 1 = deterministic clock, >= 2 also the compare tool (§5) */

void enh_init(void)
{
    if (enh.classic) return;
    while (enh.res_scale * enh.aa > 16 && enh.aa > 1) enh.aa--;       /* res_scale * aa <= 16 (§3.9) */
    const char *t = SDL_getenv("SR_ENH_TRACE");
    if (t && *t) {
        trace = strcmp(t, "-") ? fopen(t, "w") : stderr;
        if (trace)
            fprintf(trace, "# S frame t_ms ticks mode seg sub step cam_x heading opp_seg opp_frac opp_lat "
                           "front_s/v/drawn mirror_s/v/drawn page wall_hit\n"
                           "# C t_ms render_ms frame_a frame_b x w snap P rel_heading cam_x O opp_lat jerk us\n");
    }
    const char *c = SDL_getenv("SR_ENH_COMPARE");
    compare = c && *c ? atoi(c) : 0;
    if (c && *c && !compare && strcmp(c, "0")) compare = 1;            /* as host.c: any other value = the clock */
    if (compare >= 2) enh_compare_init(compare);
}

bool enh_hires(void) { return !enh.classic; }

/* ---------------------------------------------------------------- race state, layers */

#define PAGE_BYTES  (EGA_BYTES_PER_LINE * 200)      /* 8000: one bit per pixel, as a plane */
#define PAGE1_OFF   0x2000u                         /* A200:0000 */
#define PAGE2_OFF   0x4000u                         /* A400:0000 */

static struct {
    bool overlay;           /* a race of an enabled mode is running */
    bool bg;                /* inside a race frame, background layer */
    bool draws_view;        /* the enhanced view is drawn over the EGA picture (S2.3: road modes 0 and 1) */
    int shown_page;         /* page (1, 2) presented by road_page_flip with a frame, 0 = none */
    u8 written[2][PAGE_BYTES];  /* per race page: bit set = the original's pixel shows (§2.1) */
} rs;

/* ---------------------------------------------------------------- snapshots (§3.1) */

#define RING 8
static struct {
    enh_snap s[RING];
    unsigned n;             /* snapshots taken since race start; the newest is s[(n - 1) % RING] */
    unsigned race;          /* races begun (the trace restarts its jerk estimate) */
} ring;

static enh_snap *snap_at(unsigned age) { return &ring.s[(ring.n - 1 - age) % RING]; }    /* age 0 = newest */

static void snap_take(void)
{
    enh_snap *s = &ring.s[ring.n % RING];
    memset(s, 0, sizeof *s);
    s->t_ns = host_tick_ns();
    s->frame = ring.n;
    s->ticks = DSL(DS_ticks);
    s->mode = (s16)DSW(DS_road_mode);
    s->racing = DSB(DS_racing);
    s->start_seg = DSS(DS_start_seg);
    s->track_end = DSS(DS_track_end);
    s->fallen = DSS(DS_fallen);
    s->fall_dir = DSS(DS_fall_dir);
    s->seg = DSS(DS_seg);
    s->substep = DSS(DS_substep);
    s->step = DSS(DS_step_per_frame);
    s->cam_x = DSS(DS_cam_x);
    s->cam_y = DSS(DS_cam_y);
    s->heading = DSS(DS_heading);
    s->cur_height = DSS(DS_cur_height);
    s->bounce_y = DSS(DS_view_bounce_y);
    s->wheel = PHS(PLAYER, PH_WHEEL);
    s->opp_seg = DSS(DS_opp_seg);
    s->opp_frac = DSS(DS_opp_seg_frac64);
    s->opp_lat = DSS(DS_opp_lat);
    s->opp_lat_step = DSS(DS_lat_last_opp_step);
    s->opp_model = DSS(DS_opp_model);
    s->opp_sprite = DSS(DS_opp_sprite);
    s->opp_variant = DSS(DS_opp_variant);
    s->opp_s[0] = s->opp_s[1] = s->opp_v[0] = s->opp_v[1] = -1;
    s->police_state = DSS(DS_police_state);
    s->police_seg = DSS(DS_police_seg);
    s->start_light_pos = DSS(DS_start_light_pos);
    s->start_lights = DSS(DS_start_lights);
    s->light_latch = DSS(DS_light_latch);
    s->starter_seg = DSS(DS_starter_seg);
    s->wall_hit_shift = DSS(DS_wall_hit_shift);
    for (int i = 0; i < 17; i++) s->pal[i] = ega_get_palette_reg(i);
    ring.n++;
}

/* The values the frame's projection computed (Aqueduct roll and channel, horizon band row). */
static void snap_projected(void)
{
    if (!ring.n) return;
    enh_snap *s = snap_at(0);
    s->roll = DSS(DS_rot_enable);
    s->rot_cos = DSS(DS_rot_cos);
    s->rot_sin = DSS(DS_rot_sin);
    s->rot_cx = DSS(DS_rot_cx);
    s->rot_cy = DSS(DS_rot_cy);
    s->aq_pitch = DSS(DS_aq_pitch);
    s->aq_dy = DSS(DS_aq_dy);
    s->floor_h = DSS(DS_aq_saved_floor_h);
    s->top_h = DSS(DS_aq_saved_top_h);
    s->xl = DSS(DS_aq_saved_xl);
    s->xr = DSS(DS_aq_saved_xr);
    s->wall_l_shift = DSS(DS_aq_wall_l_shift);
    s->wall_r_shift = DSS(DS_aq_wall_r_shift);
    s->opp_bank = DSS(DS_opp_bank);
    s->top_band_row = DSS(DS_top_band_row);
}

static void trace_snap(const enh_snap *s)
{
    fprintf(trace, "S %u %.3f %u %d %d %d %d %d %d %d %d %d %d/%d/%d %d/%d/%d %d %d\n", s->frame, s->t_ns / 1e6,
            s->ticks, s->mode, s->seg, s->substep, s->step, s->cam_x, s->heading, s->opp_seg, s->opp_frac, s->opp_lat,
            s->opp_s[0], s->opp_v[0], s->opp_drawn[0], s->opp_s[1], s->opp_v[1], s->opp_drawn[1], s->page,
            s->wall_hit_shift);
}

/* ---------------------------------------------------------------- track reads (clamped, §1) */

static s16 trk(u16 arr, int i) { return TRKS(arr, i < 0 ? 0 : i >= TRK_N ? TRK_N - 1 : i); }

static double trk_at(u16 arr, double P)
{
    double f = floor(P / 64.0);
    int i = (int)f;
    s16 v0 = trk(arr, i);
    return v0 + (P / 64.0 - f) * (s16)(trk(arr, i + 1) - v0);      /* wrap-safe step */
}

double enh_track_yaw(double P) { return trk_at(TRK_YAW, P); }
double enh_track_hgt(double P) { return trk_at(TRK_HGT, P); }

/* ---------------------------------------------------------------- interpolation (§3.1) */

#define SNAP_SEGS     2           /* a jump of >= 2 segments */
#define SNAP_HEADING  16
#define SNAP_CAM_X    0x100       /* edge bounce */

static double pos(s16 seg, s16 sub) { return seg * 64.0 + sub; }

/* heading - YAW(P): the heading relative to the road at the car's continuous position */
static double rel_heading(const enh_snap *s)
{
    s16 y0 = trk(TRK_YAW, s->seg);
    return (s16)(s->heading - y0) - (enh_track_yaw(pos(s->seg, s->substep)) - y0);
}

/* A pair that must not be blended: mode change, fall-off and wall-bounce frames, jumps (§3.1). */
static bool discontinuous(const enh_snap *a, const enh_snap *b)
{
    if (a == b) return false;
    return a->mode != b->mode || b->fall_dir || b->fallen != a->fallen || b->wall_hit_shift
        || fabs(pos(b->seg, b->substep) - pos(a->seg, a->substep)) >= SNAP_SEGS * 64
        || abs((s16)(b->heading - a->heading)) >= SNAP_HEADING || abs((s16)(b->cam_x - a->cam_x)) >= SNAP_CAM_X;
}

enum { V_P, V_REL, V_CAMX, V_CAMY, V_HGT, V_BOUNCE, V_WHEEL, V_O, V_LAT, V_ROLL, V_RCX, V_RCY, V_PITCH, V_AQDY, V_N };

static void values(const enh_snap *s, double *v)
{
    v[V_P] = pos(s->seg, s->substep);
    v[V_REL] = rel_heading(s);
    v[V_CAMX] = s->cam_x;
    v[V_CAMY] = s->cam_y;
    v[V_HGT] = s->cur_height;
    v[V_BOUNCE] = s->bounce_y;
    v[V_WHEEL] = s->wheel;
    v[V_O] = pos(s->opp_seg, s->opp_frac);
    v[V_LAT] = s->opp_lat;
    v[V_ROLL] = s->roll;
    v[V_RCX] = s->rot_cx;
    v[V_RCY] = s->rot_cy;
    v[V_PITCH] = s->aq_pitch;
    v[V_AQDY] = s->aq_dy;
}

/* Position of time t on the pair (a, b): 0 at a, 1 at b, extrapolated up to `xmax` past a. */
static double pair_x(const enh_snap *a, const enh_snap *b, uint64_t t, double xmax)
{
    if (a == b || b->t_ns <= a->t_ns) return 1.0;
    double x = ((double)t - (double)a->t_ns) / (double)(b->t_ns - a->t_ns);
    return x < 0 ? 0 : x > xmax ? xmax : x;
}

static void pair_at(const enh_snap *a, const enh_snap *b, double x, double *out)
{
    double va[V_N], vb[V_N];
    values(a, va);
    values(b, vb);
    for (int k = 0; k < V_N; k++) out[k] = va[k] + (vb[k] - va[k]) * x;
    /* the opponent jumps on its own (aq_wall_bounce pulls it away 2 segments a frame): no blend then */
    if (fabs(vb[V_O] - va[V_O]) >= SNAP_SEGS * 64) out[V_O] = vb[V_O];
}

/* Render time t = now - motion delay. The pair (a, b) around t (or the newest pair) is blended linearly; while t is
 * inside it, the line of the previous pair (z, a), extrapolated, fades into it with a smoothstep, so the speed stays
 * continuous when a new snapshot arrives: between snapshots everything moves at the speed of the last pair (TD3E,
 * SRE). A discontinuous pair shows b unblended. */
bool enh_interp(uint64_t now_ns, enh_view *o)
{
    if (!ring.n) return false;
    uint64_t delay = (uint64_t)enh.motion_delay * 1000000u;
    uint64_t t = now_ns > delay ? now_ns - delay : 0;
    unsigned avail = ring.n < RING ? ring.n : RING, i = 0;
    while (i + 1 < avail && snap_at(i + 1)->t_ns > t) i++;  /* b = the oldest snapshot after t, else the newest */
    const enh_snap *b = snap_at(i);
    const enh_snap *a = i + 1 < avail ? snap_at(i + 1) : b;
    const enh_snap *z = i + 2 < avail ? snap_at(i + 2) : a;
    double v[V_N];
    memset(o, 0, sizeof *o);
    o->a = a;
    o->b = b;
    o->render_ns = t;
    o->w = 1.0;
    o->snap = a == b || discontinuous(a, b);
    if (o->snap) {
        o->x = 1.0;
        values(b, v);
    } else {
        o->x = pair_x(a, b, t, 1.5);             /* a late snapshot: half a period past b at most */
        pair_at(a, b, o->x, v);
        if (z != a && !discontinuous(z, a) && o->x < 1.0) {
            double vz[V_N];
            pair_at(z, a, pair_x(z, a, t, 2.5), vz);     /* the previous line through this pair */
            o->w = o->x * o->x * (3 - 2 * o->x);
            for (int k = 0; k < V_N; k++) v[k] = vz[k] + (v[k] - vz[k]) * o->w;
            /* a hard stop: the previous pair's line overshoots b; positions along the road never run backwards */
            double va[V_N], vb[V_N];
            values(a, va);
            values(b, vb);
            for (int k = V_P; k <= V_O; k += V_O - V_P)
                v[k] = fmax(fmin(va[k], vb[k]), fmin(fmax(va[k], vb[k]), v[k]));
        }
    }
    o->P = v[V_P];
    o->rel_heading = v[V_REL];
    o->heading = enh_track_yaw(o->P) + o->rel_heading;
    o->cam_x = v[V_CAMX];
    o->cam_y = v[V_CAMY];
    o->cur_height = v[V_HGT];
    o->bounce_y = v[V_BOUNCE];
    o->wheel = v[V_WHEEL];
    o->O = v[V_O];
    o->opp_lat = v[V_LAT];
    o->roll = v[V_ROLL];
    o->rot_cx = v[V_RCX];
    o->rot_cy = v[V_RCY];
    o->aq_pitch = v[V_PITCH];
    o->aq_dy = v[V_AQDY];
    return true;
}

/* One line per interpolated (composed) frame; jerk = change of the camera speed along the road (sub-steps per
 * second) from the previous composed frame, us = time spent in enh_interp. */
static void trace_view(uint64_t now, const enh_view *v, double us)
{
    static uint64_t t1, t2;
    static double p1, p2;
    static unsigned race;
    double jerk = 0;
    if (race != ring.race) { race = ring.race; t1 = t2 = 0; }
    if (t1 && t2 && now > t1 && t1 > t2)
        jerk = (v->P - p1) / ((double)(now - t1) / 1e9) - (p1 - p2) / ((double)(t1 - t2) / 1e9);
    fprintf(trace, "C %.3f %.3f %u %u %.4f %.4f %d %.3f %.3f %.1f %.3f %.1f %.1f %.1f\n", now / 1e6,
            v->render_ns / 1e6, v->a->frame, v->b->frame, v->x, v->w, v->snap, v->P, v->rel_heading, v->cam_x, v->O,
            v->opp_lat, jerk, us);
    t2 = t1; p2 = p1;
    t1 = now; p1 = v->P;
}

static enh_view view;               /* the interpolated view of the last compose (drawn from S2.3 on) */

static unsigned mode_bit(int road_mode)
{
    switch (road_mode) {
    case 0: return ENH_MODE_TOWN;
    case 1: return ENH_MODE_DRAG;
    case 2: case 4: return ENH_MODE_STRIP;
    case 8: return ENH_MODE_AQUEDUCT;
    case 0x10: return ENH_MODE_MULHOLLAND;
    default: return 0;
    }
}

void enh_race_begin(int road_mode)
{
    if (enh.classic) return;
    rs.overlay = (enh.modes & mode_bit(road_mode)) != 0;
    rs.draws_view = rs.overlay && (road_mode == 0 || road_mode == 1);     /* S2.3: town, street drag race */
    rs.bg = false;
    rs.shown_page = 0;
    memset(rs.written, 0xFF, sizeof rs.written);
    enh.track = rs.overlay;
    ring.n = 0;
    ring.race++;
    if (trace && rs.overlay) fprintf(trace, "# race mode %d\n", road_mode);
}

void enh_race_end(void)
{
    if (rs.overlay) enh_compare_race_end();
    if (trace && rstat.n)
        fprintf(trace, "# render %llu frames, mean %.3f ms, max %.3f ms (%dx%d, aa %d)\n", (unsigned long long)rstat.n,
                rstat.sum / 1e6 / rstat.n, rstat.max / 1e6, 320 * enh.res_scale, 200 * enh.res_scale, enh.aa);
    memset(&rstat, 0, sizeof rstat);
    if (trace) fflush(trace);
    rs.overlay = false;
    rs.draws_view = false;
    rs.bg = false;
    rs.shown_page = 0;
    enh.track = false;
}

void enh_frame_begin(void)
{
    if (!rs.overlay) return;
    rs.bg = true;
    if (trace && ring.n) trace_snap(snap_at(0));           /* the previous frame, now complete */
    if (!ring.n && rs.draws_view) enh_sky_capture();        /* road_load_graphics has stored the sky in page 1 */
    snap_take();
}

void enh_layer_fg(void)
{
    if (rs.bg) snap_projected();
    rs.bg = false;
}

void enh_opp_drawn(int view_id, int s, int v, bool drawn)
{
    if (!rs.overlay || !ring.n || view_id < 0 || view_id > 1) return;
    enh_snap *n = snap_at(0);
    n->opp_s[view_id] = (s8)s;
    n->opp_v[view_id] = (s8)v;
    n->opp_drawn[view_id] = drawn;
}
void enh_layer_bg(void) { if (rs.overlay) rs.bg = true; }

void enh_frame_presented(int page)
{
    if (!rs.overlay) return;
    rs.shown_page = (page == 1 || page == 2) ? page : 0;
    if (ring.n) snap_at(0)->page = (s8)rs.shown_page;
    if (compare >= 2 && rs.draws_view && ring.n) enh_compare_frame(snap_at(0), rs.shown_page);
}

/* ---------------------------------------------------------------- written mask (§2.1) */

u32 enh_vram_peek(u16 off)
{
    return (u32)ega_plane(0)[off] | (u32)ega_plane(1)[off] << 8 | (u32)ega_plane(2)[off] << 16
         | (u32)ega_plane(3)[off] << 24;
}

void enh_vram_write(u16 off, u32 before)
{
    u8 *mask;
    if (off >= PAGE1_OFF && off < PAGE1_OFF + PAGE_BYTES) mask = &rs.written[0][off - PAGE1_OFF];
    else if (off >= PAGE2_OFF && off < PAGE2_OFF + PAGE_BYTES) mask = &rs.written[1][off - PAGE2_OFF];
    else return;
    u32 d = before ^ enh_vram_peek(off);
    u8 changed = (u8)(d | d >> 8 | d >> 16 | d >> 24);         /* pixels whose colour changed */
    if (rs.bg) {
        /* background: the enhanced view owns every pixel the write touched */
        u8 touched = (vga.map_mask & 0x0F) ? ((vga.mode & 3) == 1 ? 0xFF : vga.bit_mask) : 0;
        *mask &= (u8)~(touched | changed);
    } else {
        *mask |= changed;                                       /* foreground: last writer wins per pixel */
    }
}

bool enh_written(int page, int x, int y)
{
    if (page < 1 || page > 2 || x < 0 || x >= 320 || y < 0 || y >= 200) return true;
    return rs.written[page - 1][y * EGA_BYTES_PER_LINE + x / 8] >> (7 - x % 8) & 1;
}

/* ---------------------------------------------------------------- compose */

static u32 pic[320 * 200];
static u8 cover[(ENH_SPLIT_Y - ENH_VIEW_Y0) * 320];

/* The logical pixels of the view that show the enhanced picture (§2.1): above the split, outside the mirror (the
 * original's until S2.6), not last written by the foreground on the shown page. */
static void cover_build(int page)
{
    for (int y = ENH_VIEW_Y0; y < ENH_SPLIT_Y; y++)
        for (int x = 0; x < 320; x++)
            cover[(y - ENH_VIEW_Y0) * 320 + x] = !enh_mirror_px(x, y) && !enh_written(page, x, y);
}

bool enh_compose(bool (*ega_compose)(u32 *xrgb), u32 *out)
{
    bool changed = ega_compose(pic);
    /* The view at the display rate. Without a drawn view the host calls compose on every pump (nothing presented):
     * evaluate at most every 8 ms, the host's present gate, so the trace shows the display cadence. */
    static uint64_t last_eval;
    uint64_t now = host_clock_ns();
    bool live = rs.overlay && rs.draws_view && ring.n && rs.shown_page;
    if (rs.overlay && ring.n && (rs.draws_view || now - last_eval >= 8000000u)) {
        uint64_t c0 = SDL_GetTicksNS();
        last_eval = now;
        enh_interp(now, &view);
        if (trace) trace_view(now, &view, (double)(SDL_GetTicksNS() - c0) / 1e3);
    }
    /* While the enhanced view is drawn it changes on every call: the host then presents at the display rate (§2).
     * With the compare clock it presents only when the EGA picture changes, so snapshot runs of --classic and of the
     * Enhanced graphics present at the same moments (tools/classic_check.py --clock). */
    if (!changed && !(live && !compare)) return false;
    int n = enh.res_scale, w = 320 * n;
    for (int y = 0; y < 200; y++) {
        u32 *row = out + (size_t)y * n * w;
        const u32 *src = pic + y * 320;
        for (int x = 0; x < 320; x++)
            for (int k = 0; k < n; k++) *row++ = src[x];
        row -= w;
        for (int k = 1; k < n; k++) memcpy(row + (size_t)k * w, row, (size_t)w * sizeof *row);
    }
    if (live) {
        uint64_t c0 = SDL_GetTicksNS();
        if (enh_road_render(&view)) {
            cover_build(rs.shown_page);
            ras_resolve(view.b->pal, cover, out, n, enh.aa);
            uint64_t dt = SDL_GetTicksNS() - c0;                /* render time per displayed frame (§3.10) */
            rstat.n++;
            rstat.sum += dt;
            if (dt > rstat.max) rstat.max = dt;
        }
    }
    return true;
}
