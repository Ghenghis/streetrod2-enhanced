/* Street Rod 2 Enhanced: float road geometry and sky of the normal renderer (ENHANCED.md §3.2, §3.5).
 * S2.3 (Milestone A): the town cruise and the street drag race (road modes 0, 1), since S2.9's first part at
 * --draw-distance-scale times the original draw distance (14 segments; default 1.25 = 17.5, §4). The road is projected in float with the formulas of road_proj_x / road_proj_y (race_road.c) at the
 * interpolated view, each segment cut into slices so that YAW / HGT / edges vary continuously, and emitted far to
 * near in the original's painter's order (road_draw_front: town walls, then the band of each row, then the row's
 * road-side objects, enh_sprite.c, S2.5). The opponent is the original's pixels (passthrough brackets in
 * road_draw_front and road_draw_mirror). The mirror (S2.6, §3.7) is the same renderer with road_proj_x_mirror /
 * road_proj_y_mirror (1/4 of the front's scale, centre 280, 36), 7 rows behind, clipped to (240, 22)-(319, 42), over
 * the original's backdrop pictures; frame lines redrawn.
 *
 * World: a position Pw along the road in sub-steps (64 per segment), lateral x and height y in road units (road
 * 0x5DC..0xCE4, surface y 2000, eye 0x76C). Depth z = (Pw - P) / 8 + 4, so segment seg + k lies at the original's row
 * depth 8k + 4 - substep / 8 (without the original's truncation of substep / 8; the compare quantizes P). Row 0
 * (z = 4) is the near clip; the far end is z = 4 + 112 x scale (116 at 1: the original pins row 14 there). Segment s carries the flags
 * L/R/C[s] for the band from boundary s - 1 to boundary s, and the edges, YAW and HGT of boundary s. */
#include "enh_internal.h"
#include "enhanced.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/race_int.h"
#include "platform/ega.h"

/* The original truncates its divisions (cdiv16): x towards the screen centre, y down to the row above; its spans
 * include both end pixels and a band covers the rows from its far row to the row above its near row. Projected values
 * are moved by XOFF / YOFF into the continuous pixel coordinates of the raster (a sample is inside when its centre is),
 * so that the edges land on the original's pixels. */
#define XOFF    0.5
#define YOFF  (-0.4)           /* -0.5, less a margin: an exact row value (the eye level 60) stays on its row */
#define SKY_RATE (1.0 / 8.0)    /* sky scroll: 1/8 of the original's 2 px (panorama 4 px) per heading unit (§3.5) */
#define FAR_Z   116.0           /* the original's row 14: the far end at --draw-distance-scale 1 */
#define TOWN_END 0xB3           /* the last town segment (track_build_all: town 0 .. 0xB3, race road from 0xB4) */
#define TOWN_HORIZON 63.0       /* the town's fill below the rooftop panorama (sky): the road surface stays below it */
#define SURFACE 2000.0
#define EYE     1900.0          /* 0x76C */

typedef struct {
    double P;                   /* player position, sub-steps */
    double cx, cy, hgt, heading, bounce;
    int mode, seg0;             /* seg0 = floor(P / 64) */
    bool racing, compare;
    int start_seg;
    double w;                   /* outer band width: 400 (200 Mulholland) */
    bool mirror;                /* the mirror's projection (proj_x / proj_y) */
    double far_z;               /* the front's far end: 4 + 112 x --draw-distance-scale (z = 116 at 1) */
    int rows;                   /* enh_view.rows (compare aid) */
    /* stage 2 table ends (ENHANCED.md §4 item 2), live view only: segments from `synth` on are plain road (no objects),
     * YAW / HGT held beyond position `hold` */
    int synth;
    double hold;
} cam;

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static u16 flag(u16 arr, int s) { return TRKW(arr, clampi(s, 0, TRK_N - 1)); }

/* The front view's track: past the street drag race's finish the table holds the road course (curves, hills, its
 * objects); the live view shows plain flat road there instead: from segment `synth` (track_end) on only the centre
 * dashes (C & 8, the course's dash pattern) are kept, and beyond the finish boundary (or the car, once past it) YAW / HGT
 * hold their value, so the road runs straight on at the car's heading. The compare view and the mirror read the table
 * as the original. */
static u16 cflag(const cam *c, u16 arr, int s) { return s < c->synth ? flag(arr, s) : arr == TRK_C ? flag(TRK_C, s) & 8 : 0; }
static double yaw_at(const cam *c, double Pw) { return enh_track_yaw(fmin(Pw, c->hold)); }
static double hgt_at(const cam *c, double Pw) { return enh_track_hgt(fmin(Pw, c->hold)); }

/* ---------------------------------------------------------------- mirror projection (road_project_mirror) */

#define MIR_X0   0xF0           /* clip_set_mirror: x 240..319, rows 22..42 */
#define MIR_Y0   0x16
#define MIR_Y1   0x2B
#define MIR_NEAR 8.0            /* row 0 of road_project_mirror */

/* The mirror's depth of Pw (behind the car): continuous (P - Pw) / 8, at least 8 (the original's row 0); comparing
 * (Pw a boundary) the original's: row 0: 8, rows 1..4: 8k + q, rows 5..7: 8k (the k >= 5 step, §3.7). */
static double mir_z(const cam *c, double Pw)
{
    if (!c->compare) return fmax((c->P - Pw) / 8.0, MIR_NEAR);
    int k = c->seg0 - (int)lround(Pw / 64.0), q = (int)fmod(c->P, 64.0) >> 3;
    return k <= 0 ? 8.0 : k <= 4 ? 8.0 * k + q : 8.0 * k;
}

static double mir_dx(const cam *c, double Pw) { return enh_track_yaw(Pw) - c->heading; }
static double mir_dy(const cam *c, double Pw) { return (c->mode & 6) ? 0.0 : enh_track_hgt(Pw) - c->hgt; }

static double proj_x(const cam *c, double Pw, double xw)
{
    if (c->mirror) return 280.0 + mir_dx(c, Pw) / 4.0 - (c->cx - xw) / mir_z(c, Pw) + XOFF;
    double z = (Pw - c->P) / 8.0 + 4.0;
    double dx = c->heading - yaw_at(c, Pw);
    return 160.0 + dx - 4.0 * (c->cx - xw) / z + XOFF;
}

static double proj_y(const cam *c, double Pw, double yw)
{
    if (c->mirror) return 36.0 + mir_dy(c, Pw) + (yw - c->cy) / mir_z(c, Pw) + YOFF;
    double z = (Pw - c->P) / 8.0 + 4.0;
    double dy = (c->mode & 6) ? 0.0 : c->hgt - hgt_at(c, Pw);
    return 2.0 * dy + 4.0 * (yw - c->cy) / z + c->bounce + 60.0 + YOFF;
}

/* road edges of boundary s (edges_of in race_road.c: C & 0xE0) */
static void edges_at(const cam *c, int s, double *xl, double *xr)
{
    u16 cm = cflag(c, TRK_C, s) & 0xE0;
    if (cm == 0)        { *xl = 0x5DC; *xr = 0xCE4; }
    else if (cm & 0x80) { *xl = 0x7D0; *xr = 0xAF0; }
    else if (cm & 0x20) { *xl = 0xCE4; *xr = 0xCE4; }      /* one-sided closures (zero width) */
    else                { *xl = 0x5DC; *xr = 0x5DC; }
}

/* The projected values of one cross-section at Pw. */
typedef struct { double Pw, xl, xr, xol, xor_, y, ye; } sect;

static sect section(const cam *c, double Pw)
{
    sect p;
    double s1 = ceil(Pw / 64.0), f = (Pw - (s1 - 1) * 64.0) / 64.0, l0, r0, l1, r1;
    edges_at(c, (int)s1 - 1, &l0, &r0);
    edges_at(c, (int)s1, &l1, &r1);
    p.Pw = Pw;
    p.xl = proj_x(c, Pw, l0 + (l1 - l0) * f);
    p.xr = proj_x(c, Pw, r0 + (r1 - r0) * f);
    p.xol = proj_x(c, Pw, 0x5DC - c->w);
    p.xor_ = proj_x(c, Pw, 0xCE4 + c->w);
    p.y = proj_y(c, Pw, SURFACE);
    p.ye = proj_y(c, Pw, EYE);
    return p;
}

static void quad(u8 col, double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3)
{
    double p[8] = { x0, y0, x1, y1, x2, y2, x3, y3 };
    ras_poly(p, 4, col);
}

/* a band from x = a to x = b between the far section f and the near section n (road plane) */
static void band_quad(u8 col, double af, double bf, double an, double bn, const sect *f, const sect *n)
{
    quad(col, af, f->y, bf, f->y, bn, n->y, an, n->y);
}

/* ---------------------------------------------------------------- town walls (town_walls_front, racing off) */

static void town_walls(const cam *c, int s, const sect *p, int n, bool left)
{
    u16 F = cflag(c, left ? TRK_L : TRK_R, s), Fm1 = cflag(c, left ? TRK_L : TRK_R, s - 1);
    u16 C = cflag(c, TRK_C, s), Cm1 = cflag(c, TRK_C, s - 1);
    if (C & 4) return;                                          /* intersection */
    int k = (F & 8) ? 1 : 2;
    double top = EYE - (SURFACE - EYE) / k;                     /* the tall part */
    for (int i = 0; i < n; i++) {
        const sect *a = &p[i], *b = &p[i + 1];                  /* near, far */
        double xa = left ? a->xol : a->xor_, xb = left ? b->xol : b->xor_;
        /* poly_fill includes the bottom row (the bands' spans do not): bottom edges one row lower */
        quad((F & 0x10) ? 2 : 3, xa, a->y + 1, xb, b->y + 1, xb, b->ye, xa, a->ye);
        if (F & 4)
            quad(4, xa, a->ye + 1, xb, b->ye + 1, xb, proj_y(c, b->Pw, top), xa, proj_y(c, a->Pw, top));
    }
    const sect *nr = &p[0], *fr = &p[n];
    double xn = left ? nr->xol : nr->xor_, xf = left ? fr->xol : fr->xor_;
    if (Cm1 & 4) {                                              /* the end face towards the intersection */
        double x0 = left ? -1.0 : xn, x1 = left ? xn : 321.0;
        ras_rect(x0, nr->ye, x1, nr->y - 1.0, 2);
        ras_rect(x0, nr->y - 1.0, x1, nr->y, 4);
    }
    if (!(F & 4) || (Fm1 & 4)) return;
    /* the end face of the tall part (a screen-space rectangle as wide as the wall, as the original) */
    double h = (nr->y - nr->ye) / k, wd = fabs(xn - xf);
    if (left) ras_rect(xn - wd, nr->ye - h, xn, nr->ye, 2);
    else      ras_rect(xn, nr->ye - h, xn + wd, nr->ye, 2);
}

/* ---------------------------------------------------------------- road bands (road_draw_band_front) */

/* dash width in pixels (table DS:7E52, index = row): constant per band when comparing, else continuous in depth */
static double dash_width(const cam *c, int kband, double Pw)
{
    if (c->compare) return DSS((u16)(DS_dash_width + 2 * clampi(kband, 0, 24)));
    double kf = (Pw - c->P) / 64.0 + 0.5;
    int k0 = clampi((int)floor(kf), 0, 24), k1 = clampi(k0 + 1, 0, 24);
    double f = fmin(fmax(kf - floor(kf), 0.0), 1.0);
    s16 w0 = DSS((u16)(DS_dash_width + 2 * k0)), w1 = DSS((u16)(DS_dash_width + 2 * k1));
    return w0 + (w1 - w0) * f;
}

static void band(const cam *c, int s, int kband, const sect *p, int n)
{
    u16 C = cflag(c, TRK_C, s), L = cflag(c, TRK_L, s), R = cflag(c, TRK_R, s);
    bool cross = C & 1;
    if (cross && (C & 4)) return;                               /* intersection: nothing drawn */
    bool wide = (c->mode & 6) && s < c->start_seg + 5;
    for (int i = 0; i < n; i++) {
        const sect *a = &p[i], *b = &p[i + 1];                  /* near, far */
        if (b->y >= a->y) continue;                             /* faces away (behind a crest) */
        if (!cross) {
            band_quad(9, -1.0, b->xl, -1.0, a->xl, b, a);
            band_quad(9, b->xr, 321.0, a->xr, 321.0, b, a);
            band_quad((C & 0x10) ? 0x0F : 1, b->xl, b->xr, a->xl, a->xr, b, a);
        } else {
            if (!(L & 0x80)) band_quad(4, b->xol, b->xl, a->xol, a->xl, b, a);
            else {
                double mb = (b->xol + b->xl) / 2, ma = (a->xol + a->xl) / 2;
                band_quad(4, b->xol, mb, a->xol, ma, b, a);
                band_quad(9, mb, b->xl, ma, a->xl, b, a);
            }
            if (!(R & 0x80)) band_quad(4, b->xr, b->xor_, a->xr, a->xor_, b, a);
            else {
                double mb = (b->xor_ + b->xr) / 2, ma = (a->xor_ + a->xr) / 2;
                band_quad(4, mb, b->xor_, ma, a->xor_, b, a);
                band_quad(9, b->xr, mb, a->xr, ma, b, a);
            }
        }
        if ((C & 8) && b->xl < 0x13F) {
            double wb = dash_width(c, kband, b->Pw), wa = dash_width(c, kband, a->Pw);
            if (wide) { wb *= 8; wa *= 8; }
            double cb = (b->xl + b->xr) / 2, ca = (a->xl + a->xr) / 2;
            band_quad(wide ? 2 : 4, cb - (wb + 1) / 2, cb + (wb + 1) / 2, ca - (wa + 1) / 2, ca + (wa + 1) / 2, b, a);
        }
    }
}

/* ---------------------------------------------------------------- road-side objects (enh_sprite.c) */

/* The original's integer row values at boundary Pw: road_proj_x / _y themselves when comparing (the snapshot's state is
 * still in DGROUP then); otherwise the exact projection moved by half a pixel the way the original truncates (towards
 * the screen centre in x, towards the eye level in y), blended linearly over the 2 px around the centre so that an
 * object crossing it does not jump. */
static double est_x(const cam *c, double Pw, double xw)
{
    double z = (Pw - c->P) / 8.0 + 4.0, dx = c->heading - yaw_at(c, Pw);
    if (c->compare) return road_proj_x((s16)lround(z), (s16)lround(xw), (s16)(int)lround(dx));
    double u = 4.0 * (c->cx - xw) / z - dx;
    return 160.0 - u + 0.5 * fmin(fmax(u / 2.0, -1.0), 1.0);
}

static double est_y(const cam *c, double Pw, double yw)
{
    double z = (Pw - c->P) / 8.0 + 4.0, dy = (c->mode & 6) ? 0.0 : c->hgt - hgt_at(c, Pw);
    if (c->compare) return road_proj_y((s16)lround(z), (s16)lround(yw), (s16)(int)lround(dy));
    double v = 2.0 * dy + 4.0 * (yw - c->cy) / z;
    return v - 0.5 * fmin(fmax(v / 2.0, -1.0), 1.0) + c->bounce + 60.0;
}

/* front_objects of segment s (row k = s - seg0; the original's rows < 11, live out to the road's far end) at its far
 * boundary, the objects of `pass`; none on the plain road past a table end */
static void objects(const cam *c, int s, int pass)
{
    int k = s - c->seg0;
    if (k < 1 || (c->compare && k > 10) || s >= c->synth) return;      /* the original: rows < 11 */
    double Pw = s * 64.0, l, r;
    enh_bound b = { .s = s, .k = k, .depth = k, .depth_banner = k };
    b.z = (Pw - c->P) / 8.0 + 4.0;
    b.fade = c->compare ? 1.0 : fmin(fmax((c->far_z - b.z) / 8.0, 0.0), 1.0);   /* fade in over the last segment */
    if (b.fade <= 0.0) return;
    edges_at(c, s, &l, &r);
    b.xl = est_x(c, Pw, l);
    b.xr = est_x(c, Pw, r);
    b.xol = est_x(c, Pw, 0x5DC - c->w);
    b.xor_ = est_x(c, Pw, 0xCE4 + c->w);
    b.xfl = est_x(c, Pw, (c->w - 0x177) * -4);
    b.xfr = est_x(c, Pw, (c->w + 0x339) * 4);
    b.y = est_y(c, Pw, SURFACE);
    b.ye = est_y(c, Pw, EYE);
    b.y16 = c->compare ? 4 * b.ye - 3 * b.y : est_y(c, Pw, 1600.0);
    double Pq = fmax((s - 1) * 64.0, c->P), yq = est_y(c, Pq, SURFACE);    /* row k - 1 (row 0 at z = 4) */
    int sq = clampi(s - 1, 0, TRK_N - 1);
    double ybl = est_y(c, Pq, SURFACE - TRKS(TRK_BL, sq)), ybr = est_y(c, Pq, SURFACE - TRKS(TRK_BR, sq));
    b.flat_l = c->compare ? ybl == yq : fabs(ybl - yq) < 0.5;
    b.flat_r = c->compare ? ybr == yq : fabs(ybr - yq) < 0.5;
    b.dash = dash_width(c, k, Pw);
    enh_objects(&b, c->mode, c->compare, pass);
}

/* One segment s (band kband of the original) between Pa (near) and Pb (far), in slices. */
static void segment(const cam *c, int s, int kband, double Pa, double Pb)
{
    sect p[9];
    double ya = proj_y(c, Pa, SURFACE), yb = proj_y(c, Pb, SURFACE);
    int n = clampi((int)ceil(fabs(ya - yb) / 2.0), 1, 8);
    for (int i = 0; i <= n; i++) p[i] = section(c, Pa + (Pb - Pa) * i / n);
    /* the objects behind the building line go behind the segment's walls (enh_sprite.c); comparing: the original's
     * order, all objects after the band */
    bool split = !c->racing && !c->compare;
    if (split) objects(c, s, ENH_OBJ_OUT);
    if (!c->racing) {
        town_walls(c, s, p, n, true);
        town_walls(c, s, p, n, false);
    }
    /* the town's far road stays below the rooftop panorama's base (its fill there is the road colour, §4 item 6) */
    if (split) ras_clip(0, TOWN_HORIZON, 320, ENH_VIEW_Y1);
    band(c, s, kband, p, n);
    if (split) ras_clip(0, ENH_VIEW_Y0, 320, ENH_VIEW_Y1);
    objects(c, s, split ? ENH_OBJ_IN : ENH_OBJ_ALL);
}

static void road(const cam *c)
{
    /* out to z = far_z (116 = the original's row 14 at --draw-distance-scale 1, 144 at 1.25: 17.5 segments). Comparing,
     * the last segment seg + ceil(N) is stretched or cut to the far end, as the original pins its row 14 at z = 116;
     * live, each segment up to the one holding the far end is drawn with its own flags. The town stops at its last
     * segment (race-road segments never show in the town; its last stop, Burger's at seg 0x9B, sees to 0xAC). */
    if (c->rows < 0) return;
    double Pnear = c->P, Pfar = c->P + (c->far_z - 4.0) * 8.0;
    if (!c->racing) Pfar = fmin(Pfar, TOWN_END * 64.0);
    int s_end = c->compare ? c->seg0 + (int)ceil((c->far_z - 4.0) / 8.0 - 1e-9) : (int)ceil(Pfar / 64.0);
    int s_near = c->seg0 + 1, s_far = c->rows > 0 && s_end > c->seg0 + c->rows ? c->seg0 + c->rows : s_end;
    for (int s = s_far; s >= s_near; s--) {
        double Pa = fmax((s - 1) * 64.0, Pnear), Pb = s == s_end ? Pfar : fmin(s * 64.0, Pfar);
        if (Pb > Pa) segment(c, s, s - c->seg0, Pa, Pb);
    }
}

/* ---------------------------------------------------------------- sky (road_draw_sky) */

#define SKY_Y0 0x66             /* page 1 rows 0x66..199: sky picture and horizon panorama (road_load_graphics) */
static u8 sky_src[200 - SKY_Y0][320];

void enh_sky_capture(void)
{
    const u8 *pl[4] = { ega_plane(0), ega_plane(1), ega_plane(2), ega_plane(3) };
    for (int y = SKY_Y0; y < 200; y++)
        for (int x = 0; x < 320; x++) {
            u16 o = (u16)(0x2000 + y * EGA_BYTES_PER_LINE + x / 8);
            int b = 7 - x % 8;
            sky_src[y - SKY_Y0][x] = (u8)((pl[0][o] >> b & 1) | (pl[1][o] >> b & 1) << 1 | (pl[2][o] >> b & 1) << 2
                                        | (pl[3][o] >> b & 1) << 3);
        }
}

/* One sample row from page-1 row `src` shifted right by `shift` logical pixels (wrapping at 320). */
static void sky_row(u8 *dst, int S, int src, double shift)
{
    const u8 *s = sky_src[clampi(src, SKY_Y0, 199) - SKY_Y0];
    double sh = fmod(shift, 320.0);
    if (sh < 0) sh += 320.0;
    for (int i = 0; i < 320 * S; i++) {
        int x = (int)floor((i + 0.5) / S - sh);
        if (x < 0) x += 320;
        dst[i] = s[x];
    }
}

static void sky(const cam *c)
{
    int S = ras_S(), rows = (ENH_VIEW_Y1 - ENH_VIEW_Y0) * S;
    double hy = 0;
    if (c->racing) {
        /* the original holds HGT[seg - 1] for the whole segment: compare that, else the value at the hold's middle */
        double h = c->compare ? TRKS(TRK_HGT, clampi(c->seg0 - 1, 0, TRK_N - 1)) : enh_track_hgt(c->P - 96.0);
        double dy = (c->mode & 6) ? 0.0 : c->hgt - h;
        hy = 2.0 * dy + 400.0 / 240.0 + c->bounce + 60.0 - 0.5;    /* the original's rows start at the integer hy */
        hy = fmin(fmax(hy, 0x12), 0x52);
    }
    int last_layer = -1, last_src = -1;
    for (int j = 0; j < rows; j++) {
        double y = ENH_VIEW_Y0 + (j + 0.5) / S;
        int layer, src = 0;
        double shift = 0;
        u8 fill = 0;
        if (!c->racing) {                                       /* town: fixed sky and rooftops */
            if (y < 47)      { layer = 0; src = (int)floor(y) + 84; }
            else if (y < 63) { layer = 1; src = (int)floor(y) + 116; }
            else             { layer = 2; fill = 1; }
        } else if (y < hy - 12) {
            layer = 0; src = (int)floor(y + 0xA6 - hy); shift = 2.0 * SKY_RATE * c->heading;
        } else if (y < hy + 8) {
            layer = 1; src = (int)floor(y + 0xAF - hy); shift = 4.0 * SKY_RATE * c->heading;
        } else { layer = 2; fill = 9; }
        u8 *d = ras_row(j);
        if (layer == last_layer && src == last_src) memcpy(d, ras_row(j - 1), (size_t)320 * S);
        else if (layer == 2) memset(d, fill, (size_t)320 * S);
        else sky_row(d, S, src, shift);
        last_layer = layer;
        last_src = src;
    }
}

/* ---------------------------------------------------------------- mirror (road_draw_mirror, §3.7) */

/* Mirror row r (1..7) lies between boundaries seg - r (far, the row's values) and seg - r + 1 (near) and carries the
 * flags of segment seg - r - 1 (the original's flag pointers start at seg - 8 for row 7). */

/* dash width of mirror row r (table DS:7E52 from index 13 + r): constant per row when comparing, else continuous */
static double mir_dash(const cam *c, int r, double Pw)
{
    if (c->compare) return DSS((u16)(DS_dash_width + 2 * clampi(13 + r, 0, 24)));
    double kf = 12.5 + (c->P - Pw) / 64.0;
    int k0 = clampi((int)floor(kf), 0, 24), k1 = clampi(k0 + 1, 0, 24);
    double f = fmin(fmax(kf - floor(kf), 0.0), 1.0);
    s16 w0 = DSS((u16)(DS_dash_width + 2 * k0)), w1 = DSS((u16)(DS_dash_width + 2 * k1));
    return w0 + (w1 - w0) * f;
}

/* town_walls_mirror (town blocks only, C & 1): the tall part is half the wall's height on the left and as high on the
 * right (the original's k = 2 - (left == 0)); the end faces look at segment s + 1, nearer the car */
static void mir_walls(const cam *c, int s, const sect *p, int n, bool left)
{
    u16 C = flag(TRK_C, s);
    if (!(C & 1) || (C & 4)) return;
    u16 F = flag(left ? TRK_L : TRK_R, s), Fn = flag(left ? TRK_L : TRK_R, s + 1), Cn = flag(TRK_C, s + 1);
    int k = left ? 2 : 1;
    double top = EYE - (SURFACE - EYE) / k;
    for (int i = 0; i < n; i++) {
        const sect *a = &p[i], *b = &p[i + 1];                  /* near, far */
        double xa = left ? a->xol : a->xor_, xb = left ? b->xol : b->xor_;
        quad((F & 0x10) ? 2 : 3, xa, a->y + 1, xb, b->y + 1, xb, b->ye, xa, a->ye);
        if (F & 4)
            quad(4, xa, a->ye + 1, xb, b->ye + 1, xb, proj_y(c, b->Pw, top), xa, proj_y(c, a->Pw, top));
    }
    const sect *nr = &p[0], *fr = &p[n];
    double xn = left ? nr->xol : nr->xor_, xf = left ? fr->xol : fr->xor_;
    if (Cn & 4) {
        double x0 = left ? MIR_X0 - 1.0 : xn, x1 = left ? xn : 321.0;
        ras_rect(x0, nr->ye, x1, nr->y - 1.0, 2);
        ras_rect(x0, nr->y - 1.0, x1, nr->y, 4);
    }
    if (!(F & 4) || (Fn & 4)) return;
    double h = (nr->y - nr->ye) / k, wd = fabs(xn - xf);
    if (left) ras_rect(xn - wd, nr->ye - h, xn, nr->ye, 2);
    else      ras_rect(xn, nr->ye - h, xn + wd, nr->ye, 2);
}

/* road_draw_band_mirror (modes 0, 1): grass, road (white finish band), or the town's sidewalks; centre dashes */
static void mir_band(const cam *c, int s, int r, const sect *p, int n)
{
    u16 C = flag(TRK_C, s), L = flag(TRK_L, s), R = flag(TRK_R, s);
    bool cross = C & 1;
    if (cross && (C & 4)) return;
    bool white = (C & 0x10) && c->seg0 >= 0x169;
    for (int i = 0; i < n; i++) {
        const sect *a = &p[i], *b = &p[i + 1];                  /* near, far */
        if (b->y > a->y) continue;                              /* faces away */
        if (!cross) {
            band_quad(9, MIR_X0 - 1.0, b->xl, MIR_X0 - 1.0, a->xl, b, a);
            band_quad(9, b->xr, 321.0, a->xr, 321.0, b, a);
            band_quad(white ? 0x0F : 1, b->xl, b->xr, a->xl, a->xr, b, a);
        } else {
            if (!(L & 0x80)) band_quad(4, b->xol, b->xl, a->xol, a->xl, b, a);
            else {
                double mb = (b->xol + b->xl) / 2, ma = (a->xol + a->xl) / 2;
                band_quad(4, b->xol, mb, a->xol, ma, b, a);
                band_quad(9, mb, b->xl, ma, a->xl, b, a);
            }
            if (!(R & 0x80)) band_quad(4, b->xr, b->xor_, a->xr, a->xor_, b, a);
            else {
                double mb = (b->xor_ + b->xr) / 2, ma = (a->xor_ + a->xr) / 2;
                band_quad(4, mb, b->xor_, ma, a->xor_, b, a);
                band_quad(9, b->xr, mb, a->xr, ma, b, a);
            }
        }
        if (C & 8) {
            double wb = mir_dash(c, r, b->Pw), wa = mir_dash(c, r, a->Pw);
            double cb = (b->xl + b->xr) / 2, ca = (a->xl + a->xr) / 2;
            band_quad(4, cb - (wb + 1) / 2, cb + (wb + 1) / 2, ca - (wa + 1) / 2, ca + (wa + 1) / 2, b, a);
        }
    }
}

/* The original's integer row values of the mirror at boundary Pw (road_proj_x_mirror / _y_mirror themselves when
 * comparing), else the continuous projection moved by half a pixel the way the original truncates. */
static double mir_est_x(const cam *c, double Pw, double xw)
{
    double z = mir_z(c, Pw), dx = mir_dx(c, Pw);
    if (c->compare) return road_proj_x_mirror((s16)lround(z), (s16)lround(xw), (s16)(int)lround(dx));
    double u = (c->cx - xw) / z - dx / 4.0;
    return 280.0 - u + 0.5 * fmin(fmax(u / 2.0, -1.0), 1.0);
}

static double mir_est_y(const cam *c, double Pw, double yw)
{
    double z = mir_z(c, Pw), dy = mir_dy(c, Pw);
    if (c->compare) return road_proj_y_mirror((s16)lround(z), (s16)lround(yw), (s16)(int)lround(dy));
    double v = dy + (yw - c->cy) / z;
    return v - 0.5 * fmin(fmax(v / 2.0, -1.0), 1.0) + 36.0;
}

/* the banner and mirror_objects of mirror row r at its far boundary, the objects of `pass` */
static void mir_objects(const cam *c, int r, int pass)
{
    int bnd = c->seg0 - r;
    double Pw = bnd * 64.0, l, rr;
    edges_at(c, bnd, &l, &rr);
    enh_bound b = { .s = bnd - 1, .k = r, .mirror = true, .depth = (r >> 1) + 7, .depth_banner = r + 5, .fade = 1.0 };
    b.z = 4.0 * mir_z(c, Pw);
    b.xl = mir_est_x(c, Pw, l);
    b.xr = mir_est_x(c, Pw, rr);
    b.xol = mir_est_x(c, Pw, 0x5DC - c->w);
    b.xor_ = mir_est_x(c, Pw, 0xCE4 + c->w);
    b.xfl = mir_est_x(c, Pw, (c->w - 0x177) * -4);
    b.xfr = mir_est_x(c, Pw, (c->w + 0x339) * 4);
    b.y = mir_est_y(c, Pw, SURFACE);
    b.ye = mir_est_y(c, Pw, EYE);
    b.y16 = c->compare ? 4 * b.ye - 3 * b.y : mir_est_y(c, Pw, 1600.0);
    /* row r - 1 (nearer); its banks: the original reads them forward (BL[seg + r - 1], RR 0ce4), the enhanced
     * mirror behind the car at that boundary (§3.7) */
    double Pq = c->compare ? Pw + 64.0 : fmin(Pw + 64.0, c->P - 64.0), yq = mir_est_y(c, Pq, SURFACE);
    int sq = clampi(c->compare ? c->seg0 + r - 1 : bnd + 1, 0, TRK_N - 1);
    double ybl = mir_est_y(c, Pq, SURFACE - TRKS(TRK_BL, sq)), ybr = mir_est_y(c, Pq, SURFACE - TRKS(TRK_BR, sq));
    b.flat_l = c->compare ? ybl == yq : fabs(ybl - yq) < 0.5;
    b.flat_r = c->compare ? ybr == yq : fabs(ybr - yq) < 0.5;
    b.dash = mir_dash(c, r, Pw);
    enh_objects(&b, c->mode, c->compare, pass);
}

/* A picture rectangle of page 2's mirror store (mirror_bg_prepare: frame 2005 at (232, 102), sky 2063 below it) at
 * (dx, dy) logical, opaque */
static void mir_pic(int sx, int sy, int w, int h, double dx, double dy)
{
    const u8 *px;
    int iw;
    if (enh_sheet(ds_far(DS_page2_ptr), sx, sy, w, h, &px, &iw)) ras_image(px, iw, h, dx, dy, dx + iw, dy + h, 200.0);
}

static void mirror(const cam *c0)
{
    cam c = *c0;
    c.mirror = true;
    c.synth = INT_MAX;          /* the mirror reads the table as the original (no table end behind the car) */
    c.hold = INFINITY;
    /* mirror_draw_backdrop: the frame (rows 20..44) and its mount (rows 18, 19), then the horizon strip of the sky
     * picture above the far row, inside the mirror's clip */
    mir_pic(232, 104, 80, 25, 240, 20);
    mir_pic(264, 102, 16, 2, 272, 18);
    ras_clip(MIR_X0, MIR_Y0, 320, MIR_Y1);
    double Pfar = 64.0 * (c.seg0 - 7);
    mir_pic(232, 129, 80, 2, 240, proj_y(&c, Pfar, SURFACE) - 2.0);
    bool split = !c.compare;
    for (int r = 7; r >= 1; r--) {
        int bnd = c.seg0 - r, s = bnd - 1;
        double Pb = 64.0 * bnd, Pa = 64.0 * (bnd + 1);
        if (!c.compare) Pa = fmin(Pa, c.P - 64.0);
        if (Pa <= Pb) continue;
        double ya = proj_y(&c, Pa, SURFACE), yb = proj_y(&c, Pb, SURFACE);
        int n = c.compare ? 1 : clampi((int)ceil(fabs(ya - yb)), 1, 4);
        sect p[5];
        for (int i = 0; i <= n; i++) p[i] = section(&c, Pa + (Pb - Pa) * i / n);
        if (split) mir_objects(&c, r, ENH_OBJ_OUT);
        mir_walls(&c, s, p, n, true);
        mir_walls(&c, s, p, n, false);
        mir_band(&c, s, r, p, n);
        mir_objects(&c, r, split ? ENH_OBJ_IN : ENH_OBJ_ALL);
    }
    /* mirror_finish: the frame lines (view clip) */
    ras_clip(0, ENH_VIEW_Y0, 320, ENH_VIEW_Y1);
    ras_rect(0xF0, MIR_Y0, 0xF1, MIR_Y1, 2);
    ras_rect(0xF1, MIR_Y0, 0xF2, MIR_Y1, 4);
    ras_rect(0x13E, MIR_Y0, 0x13F, MIR_Y1, 4);
    ras_rect(0x13F, MIR_Y0, 0x140, MIR_Y1, 2);
}

/* ---------------------------------------------------------------- view */

static cam cam_of(const enh_view *v)
{
    const enh_snap *b = v->b;
    cam c = {
        .P = v->P, .cx = v->cam_x, .cy = v->cam_y, .hgt = v->cur_height, .heading = v->heading,
        .bounce = v->bounce_y, .mode = b->mode, .seg0 = (int)floor(v->P / 64.0), .racing = b->racing != 0,
        .compare = v->compare, .start_seg = b->start_seg, .w = b->mode == 0x10 ? 200 : 400,
        .far_z = 4.0 + (FAR_Z - 4.0) * enh.draw_scale, .rows = v->rows, .synth = INT_MAX, .hold = INFINITY,
    };
    if (c.racing && !c.compare && b->track_end > 0) {          /* modes 0, 1: the finish at track_end 0x17C */
        c.synth = b->track_end;
        c.hold = fmax((b->track_end - 1) * 64.0, c.P);
    }
    return c;
}

/* Compare aid (SR_ENH_COMPARE=3): the original's front rows next to the float projection of the same boundaries
 * (rows 8..14 from its row tables, rows 1..7, which the mirror projection overwrites, through road_proj_x / _y),
 * then the mirror's rows 0..7 from the tables. */
void enh_road_debug_rows(const enh_view *v, FILE *f)
{
    cam c = cam_of(v);
    int q = (int)fmod(c.P, 64.0) >> 3;
    for (int k = 1; k < 8; k++) {           /* rows 1..7 recomputed with the original's own projection */
        int s = c.seg0 + k;
        s16 z = (s16)(12 - q + 8 * (k - 1)), dx = (s16)(DSS(DS_heading) - TRKS(TRK_YAW, s));
        s16 dy = (s16)(DSS(DS_cur_height) - TRKS(TRK_HGT, s));
        double l, r;
        edges_at(&c, s, &l, &r);
        sect p = section(&c, s * 64.0);
        fprintf(f, "  row %2d orig xl %4d xr %4d y %3d | float xl %7.2f xr %7.2f y %6.2f\n", k,
                road_proj_x(z, (s16)l, dx), road_proj_x(z, (s16)r, dx), road_proj_y(z, 2000, dy),
                p.xl - XOFF, p.xr - XOFF, p.y - YOFF);
    }
    cam m = c;                              /* the mirror's rows 0..7: the original's tables hold them now */
    m.mirror = true;
    for (int k = 0; k < 8; k++) {
        sect p = section(&m, (c.seg0 - k) * 64.0);
        fprintf(f, "  mirror row %d orig xl %4d xr %4d xol %4d xor %4d y %3d | float xl %7.2f xr %7.2f xol %7.2f xor %7.2f"
                " y %6.2f\n", k, DSS((u16)(DS_row_xl + 2 * k)), DSS((u16)(DS_row_xr + 2 * k)),
                DSS((u16)(DS_row_xol + 2 * k)), DSS((u16)(DS_row_xor + 2 * k)), DSS((u16)(DS_row_y + 2 * k)),
                p.xl - XOFF, p.xr - XOFF, p.xol - XOFF, p.xor_ - XOFF, p.y - YOFF);
    }
    for (int k = 8; k <= 14; k++) {
        double Pw = k == 14 ? c.P + (FAR_Z - 4.0) * 8.0 : (c.seg0 + k) * 64.0;
        sect p = section(&c, Pw);
        fprintf(f, "  row %2d orig xl %4d xr %4d y %3d | float xl %7.2f xr %7.2f y %6.2f\n", k,
                DSS((u16)(DS_row_xl + 2 * k)), DSS((u16)(DS_row_xr + 2 * k)), DSS((u16)(DS_row_y + 2 * k)),
                p.xl - XOFF, p.xr - XOFF, p.y - YOFF);
    }
}

bool enh_road_render(const enh_view *v)
{
    const enh_snap *b = v->b;
    if (!b || (b->mode != 0 && b->mode != 1) || b->fallen) return false;   /* S2.3: modes 0 and 1 */
    if (!ras_begin(enh.res_scale * enh.aa)) return false;
    cam c = cam_of(v);
    sky(&c);
    road(&c);
    mirror(&c);
    return true;
}
