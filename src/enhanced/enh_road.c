/* Street Rod 2 Enhanced: float road geometry and sky of the normal renderer (ENHANCED.md §3.2, §3.5).
 * S2.3 (Milestone A): the town cruise and the street drag race (road modes 0, 1) at the original draw distance
 * (14 segments). The road is projected in float with the formulas of road_proj_x / road_proj_y (race_road.c) at the
 * interpolated view, each segment cut into slices so that YAW / HGT / edges vary continuously, and emitted far to
 * near in the original's painter's order (road_draw_front: town walls, then the band of each row). Sprites, banners,
 * shapes and the opponent are the original's pixels (passthrough brackets in road_draw_front); the mirror is the
 * original's until S2.6.
 *
 * World: a position Pw along the road in sub-steps (64 per segment), lateral x and height y in road units (road
 * 0x5DC..0xCE4, surface y 2000, eye 0x76C). Depth z = (Pw - P) / 8 + 4, so segment seg + k lies at the original's row
 * depth 8k + 4 - substep / 8 (without the original's truncation of substep / 8; the compare quantizes P). Row 0
 * (z = 4) is the near clip; the far end is z = 116 (the original pins row 14 there). Segment s carries the flags
 * L/R/C[s] for the band from boundary s - 1 to boundary s, and the edges, YAW and HGT of boundary s. */
#include "enh_internal.h"
#include "enhanced.h"

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
#define FAR_Z   116.0           /* row 14 */
#define SURFACE 2000.0
#define EYE     1900.0          /* 0x76C */

typedef struct {
    double P;                   /* player position, sub-steps */
    double cx, cy, hgt, heading, bounce;
    int mode, seg0;             /* seg0 = floor(P / 64) */
    bool racing, compare;
    int start_seg;
    double w;                   /* outer band width: 400 (200 Mulholland) */
} cam;

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static u16 flag(u16 arr, int s) { return TRKW(arr, clampi(s, 0, TRK_N - 1)); }

static double proj_x(const cam *c, double Pw, double xw)
{
    double z = (Pw - c->P) / 8.0 + 4.0;
    double dx = c->heading - enh_track_yaw(Pw);
    return 160.0 + dx - 4.0 * (c->cx - xw) / z + XOFF;
}

static double proj_y(const cam *c, double Pw, double yw)
{
    double z = (Pw - c->P) / 8.0 + 4.0;
    double dy = (c->mode & 6) ? 0.0 : c->hgt - enh_track_hgt(Pw);
    return 2.0 * dy + 4.0 * (yw - c->cy) / z + c->bounce + 60.0 + YOFF;
}

/* road edges of boundary s (edges_of in race_road.c: C & 0xE0) */
static void edges_at(int s, double *xl, double *xr)
{
    u16 cm = flag(TRK_C, s) & 0xE0;
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
    edges_at((int)s1 - 1, &l0, &r0);
    edges_at((int)s1, &l1, &r1);
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
    u16 F = flag(left ? TRK_L : TRK_R, s), Fm1 = flag(left ? TRK_L : TRK_R, s - 1);
    u16 C = flag(TRK_C, s), Cm1 = flag(TRK_C, s - 1);
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
    u16 C = flag(TRK_C, s), L = flag(TRK_L, s), R = flag(TRK_R, s);
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

/* One segment s (band kband of the original) between Pa (near) and Pb (far), in slices. */
static void segment(const cam *c, int s, int kband, double Pa, double Pb)
{
    sect p[9];
    double ya = proj_y(c, Pa, SURFACE), yb = proj_y(c, Pb, SURFACE);
    int n = clampi((int)ceil(fabs(ya - yb) / 2.0), 1, 8);
    for (int i = 0; i <= n; i++) p[i] = section(c, Pa + (Pb - Pa) * i / n);
    if (!c->racing) {
        town_walls(c, s, p, n, true);
        town_walls(c, s, p, n, false);
    }
    band(c, s, kband, p, n);
}

static void road(const cam *c)
{
    /* segments seg + 1 .. seg + 14; the last reaches out to z = 116 (the original pins row 14 there) */
    double Pnear = c->P, Pfar = c->P + (FAR_Z - 4.0) * 8.0;
    int s_far = c->seg0 + 14, s_near = c->seg0 + 1;
    for (int s = s_far; s >= s_near; s--) {
        double Pa = fmax((s - 1) * 64.0, Pnear), Pb = s == s_far ? Pfar : fmin(s * 64.0, Pfar);
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

/* ---------------------------------------------------------------- view */

/* Compare aid (SR_ENH_COMPARE=3): the original's front rows next to the float projection of the same boundaries
 * (rows 8..14 from its row tables, rows 1..7, which the mirror projection overwrites, through road_proj_x / _y). */
void enh_road_debug_rows(const enh_view *v, FILE *f)
{
    const enh_snap *b = v->b;
    cam c = {
        .P = v->P, .cx = v->cam_x, .cy = v->cam_y, .hgt = v->cur_height, .heading = v->heading,
        .bounce = v->bounce_y, .mode = b->mode, .seg0 = (int)floor(v->P / 64.0), .racing = b->racing != 0,
        .compare = v->compare, .start_seg = b->start_seg, .w = b->mode == 0x10 ? 200 : 400,
    };
    int q = (int)fmod(c.P, 64.0) >> 3;
    for (int k = 1; k < 8; k++) {           /* rows 1..7 recomputed with the original's own projection */
        int s = c.seg0 + k;
        s16 z = (s16)(12 - q + 8 * (k - 1)), dx = (s16)(DSS(DS_heading) - TRKS(TRK_YAW, s));
        s16 dy = (s16)(DSS(DS_cur_height) - TRKS(TRK_HGT, s));
        double l, r;
        edges_at(s, &l, &r);
        sect p = section(&c, s * 64.0);
        fprintf(f, "  row %2d orig xl %4d xr %4d y %3d | float xl %7.2f xr %7.2f y %6.2f\n", k,
                road_proj_x(z, (s16)l, dx), road_proj_x(z, (s16)r, dx), road_proj_y(z, 2000, dy),
                p.xl - XOFF, p.xr - XOFF, p.y - YOFF);
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
    cam c = {
        .P = v->P, .cx = v->cam_x, .cy = v->cam_y, .hgt = v->cur_height, .heading = v->heading,
        .bounce = v->bounce_y, .mode = b->mode, .seg0 = (int)floor(v->P / 64.0), .racing = b->racing != 0,
        .compare = v->compare, .start_seg = b->start_seg, .w = b->mode == 0x10 ? 200 : 400,
    };
    sky(&c);
    road(&c);
    return true;
}
