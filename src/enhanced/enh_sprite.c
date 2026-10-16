/* Street Rod 2 Enhanced: road-side objects of the normal renderer at continuous scale (ENHANCED.md §3.6).
 * S2.5 (modes 0 and 1, pulled forward before S2.4): the calls of front_objects (race_road.c) for one boundary,
 * re-made from the track flags at the interpolated view: SR2_SHP format-A shapes (trees, posts, signs' shapes, banner
 * text, hydrants), the location sign and billboard pictures, the banner frame. Positions are the original's row values
 * (enh_bound, from enh_road.c), sizes follow the original's LOD (shapes) or sign size (pictures) at the original's
 * row depths, interpolated in between: the shape of the next larger LOD is mapped onto the interpolated bounding box,
 * a picture is the sheet's next larger size stretched (the sheet's own smaller sizes are the mip levels, so a
 * picture is never shrunk by more than one size step). When comparing (SR_ENH_COMPARE >= 2) the LOD and the byte
 * placement of pictures are the original's, so the compare tool can match positions and pixels.
 *
 * Painter's order (the fix for trees showing in front of buildings): the original draws all objects of a row after
 * the row's walls, so an object standing behind the building line (the outer edge XOL / XOR and beyond: trees,
 * lamp posts, the far trees of flag 2) is painted over the wall of its own segment, which is nearer. Those objects
 * (ENH_OBJ_OUT) are drawn before the segment's walls in the town; the objects between the road and the building line
 * (ENH_OBJ_IN) after the band, as the original. The street drag race has no walls (one pass, the original's order). */
#include "enh_internal.h"
#include "enhanced.h"

#include <math.h>
#include <string.h>

#include "game/race_int.h"
#include "platform/ega.h"

typedef struct {
    const enh_bound *b;
    int mode, pass;
    bool compare;
} ctx;

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static double clampd(double v, double lo, double hi) { return v < lo ? lo : v > hi ? hi : v; }
static double half(const ctx *c, double v) { return c->compare ? floor(v / 2) : v / 2; }
static bool drawn(const ctx *c, bool out) { return c->pass == ENH_OBJ_ALL || c->pass == (out ? ENH_OBJ_OUT : ENH_OBJ_IN); }

/* ---------------------------------------------------------------- SR2_SHP format A (shape_draw, race_gfx.c) */

typedef struct {
    FarPtr sp;                  /* spans {s8 dx, s8 dy, s8 w} */
    u8 col[6], cnt[6];
    int groups;
    double x0, x1, y0, y1;      /* bounding box of the pixels (side 1, relative to the anchor) */
} lod_t;

static int lod_count(int type)
{
    FarPtr set = ds_far((u16)(DS_shp_a_ptrs + 4 * type));
    return far_is_null(set) ? 0 : (s16)rd16(set.seg, set.off);
}

static bool lod_read(int type, int l, lod_t *o)
{
    FarPtr set = ds_far((u16)(DS_shp_a_ptrs + 4 * type));
    if (far_is_null(set) || l < 0 || l >= (s16)rd16(set.seg, set.off)) return false;
    FarPtr recs = far_rd(set.seg, (u16)(set.off + 2));
    u16 r = (u16)(recs.off + l * 16);
    memset(o, 0, sizeof *o);
    o->sp = far_make(rd16(recs.seg, (u16)(r + 14)), rd16(recs.seg, (u16)(r + 12)));
    o->x0 = o->y0 = 1e9;
    o->x1 = o->y1 = -1e9;
    FarPtr sp = o->sp;
    for (int g = 0; g < 6; g++) {
        o->col[g] = rd8(recs.seg, (u16)(r + 2 * g));
        o->cnt[g] = rd8(recs.seg, (u16)(r + 2 * g + 1));
        if (!o->cnt[g]) break;
        o->groups++;
        for (int n = o->cnt[g]; n; n--, sp.off = (u16)(sp.off + 3)) {
            int dx = (s8)rd8(sp.seg, sp.off), dy = (s8)rd8(sp.seg, (u16)(sp.off + 1)), w = (s8)rd8(sp.seg, (u16)(sp.off + 2));
            int a = dx < dx + w ? dx : dx + w, b = (dx > dx + w ? dx : dx + w) + 1;
            o->x0 = fmin(o->x0, a); o->x1 = fmax(o->x1, b);
            o->y0 = fmin(o->y0, dy); o->y1 = fmax(o->y1, dy + 1);
        }
    }
    return o->groups > 0;
}

/* The LOD the original uses at row depth `depth` (scenery_draw_shape: 9 - depth, clamped) and the continuous one. */
static double lod_at(const ctx *c, int off, int maxlod)
{
    if (c->compare) return clampi(9 - (c->b->k + off), 0, maxlod);
    return clampd(9.0 - (c->b->rc + off), 0, maxlod);
}

static double bbox_h(int type, double lam)
{
    lod_t a, b;
    int l0 = (int)floor(lam), l1 = (int)ceil(lam);
    if (!lod_read(type, l0, &a)) return 0;
    if (!lod_read(type, l1, &b)) b = a;
    double t = lam - l0;
    return (a.y1 - a.y0) + ((b.y1 - b.y0) - (a.y1 - a.y0)) * t;
}

/* scenery_draw_shape / scenery_draw_hydrant at anchor (x, y) (the original's integer row values, continuous) */
static void shape(const ctx *c, double x, double y, int off, int side, int type, bool noclamp, int maxlod, bool out,
                  int kind)
{
    if (!drawn(c, out)) return;
    if (!noclamp && y > 0x65) y = 0x65;
    if (c->compare) enh_compare_obj(false, kind, type, c->b->k + off, side, x, y, 0);
    int n = lod_count(type);
    if (clampi(9 - (c->b->k + off), 0, maxlod) >= n) return;     /* the original draws nothing at this row */
    double lam = fmin(lod_at(c, off, maxlod), n - 1);
    int l0 = (int)floor(lam), l1 = (int)ceil(lam);
    lod_t A, B;
    if (!lod_read(type, l0, &A)) return;
    if (!lod_read(type, l1, &B)) { B = A; l1 = l0; }
    double t = lam - l0;
    double tx0 = A.x0 + (B.x0 - A.x0) * t, tx1 = A.x1 + (B.x1 - A.x1) * t;
    double ty0 = A.y0 + (B.y0 - A.y0) * t, ty1 = A.y1 + (B.y1 - A.y1) * t;
    double sx = B.x1 > B.x0 ? (tx1 - tx0) / (B.x1 - B.x0) : 1, sy = B.y1 > B.y0 ? (ty1 - ty0) / (B.y1 - B.y0) : 1;
    FarPtr sp = B.sp;
    for (int g = 0; g < B.groups; g++)
        for (int k = B.cnt[g]; k; k--, sp.off = (u16)(sp.off + 3)) {
            int dx = (s8)rd8(sp.seg, sp.off), dy = (s8)rd8(sp.seg, (u16)(sp.off + 1)), w = (s8)rd8(sp.seg, (u16)(sp.off + 2));
            int a = dx < dx + w ? dx : dx + w, b = (dx > dx + w ? dx : dx + w) + 1;
            double xa = tx0 + (a - B.x0) * sx, xb = tx0 + (b - B.x0) * sx;
            double ya = ty0 + (dy - B.y0) * sy, yb = ya + sy;
            if (side < 1) { double t0 = 1 - xb; xb = 1 - xa; xa = t0; }  /* mirrored: x - dx - w .. x - dx */
            ras_rect(x + xa, y + ya, x + xb, y + yb, B.col[g]);
        }
    if (c->compare) {
        /* the continuous size at this depth against the original's LOD (within half a LOD step) */
        double lc = fmin(clampd(9.0 - ((c->b->z - 4.0) / 8.0 + 7.0 / 16.0 + off), 0, maxlod), n - 1);
        double h = bbox_h(type, lam), hc = bbox_h(type, lc);
        double step = fmax(fabs(bbox_h(type, fmin(lam + 1, n - 1)) - h), fabs(h - bbox_h(type, fmax(lam - 1, 0))));
        enh_compare_size(fabs(hc - h), 0.5 * step + 0.5);
    }
}

/* ---------------------------------------------------------------- pictures (scenery_draw_sign_picture) */

static u8 img[200 * 320];

/* A w x h pixel rectangle at (sx, sy) of the picture or page `d`, whole bytes as the blit (opaque). */
static bool sheet_read(FarPtr d, int sx, int sy, int *w, int h)
{
    if (far_is_null(d)) return false;
    FarPtr pl = desc_planes(d);
    bool vram = desc_type(d) == DESC_TYPE_EGA;
    int stride = vram ? EGA_BYTES_PER_LINE : desc_stride(d), dh = desc_h(d);
    int bx = sx >> 3, bw = *w >> 3;
    if (bx + bw > stride) bw = stride - bx;
    if (sy + h > dh) h = dh - sy;
    if (bw <= 0 || h <= 0 || bw * 8 > 320 || h > 200) return false;
    *w = bw * 8;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < *w; i++) {
            u16 o = (u16)(pl.off + (sy + j) * stride + bx + (i >> 3));
            int bit = 7 - (i & 7), v = 0;
            for (int k = 0; k < 4; k++) {
                u8 byte = vram ? ega_plane(k)[o] : rd8(pl.seg, (u16)(o + k * stride * dh));
                v |= (byte >> bit & 1) << k;
            }
            img[j * *w + i] = (u8)v;
        }
    return true;
}

static void sign(const ctx *c, double x, double y, int align, int kind, bool out)
{
    if (!drawn(c, out)) return;
    const enh_bound *b = c->b;
    if (c->compare) enh_compare_obj(false, 2, kind, b->k, align, x, y, 0);
    FarPtr src;
    int sy;
    if (kind == 1) {
        if (DSS(DS_driver_id) == -2) { sy = 100; src = ds_far(DS_page0_ptr); }
        else                         { sy = 0;   src = ds_far(DS_page2_ptr); }
    } else if (kind == 2 || kind == 3) { sy = 0; src = ds_far((u16)(DS_billboard_pics + 4 * (kind - 2))); }
    else return;
    double lam = c->compare ? clampi(9 - b->k, 0, 6) : clampd(9.0 - b->rc, 0, 6);
    int l0 = (int)floor(lam), l1 = (int)ceil(lam);
    double t = lam - l0;
    u16 e0 = (u16)(DS_sign_sizes + 6 * l0), e1 = (u16)(DS_sign_sizes + 6 * l1);
    double w = DSS((u16)(e0 + 2)) + (DSS((u16)(e1 + 2)) - DSS((u16)(e0 + 2))) * t;
    double h = DSS((u16)(e0 + 4)) + (DSS((u16)(e1 + 4)) - DSS((u16)(e0 + 4))) * t;
    double wb = (DSS((u16)(e0 + 2)) >> 3) * 8 + ((DSS((u16)(e1 + 2)) >> 3) * 8 - (DSS((u16)(e0 + 2)) >> 3) * 8) * t;
    int iw = DSS((u16)(e1 + 2)), ih = DSS((u16)(e1 + 4));
    if (!sheet_read(src, DSS(e1), sy, &iw, ih)) return;
    double x0, y0;
    if (c->compare) {
        int xi = (int)lround(x), wi = (int)w, hi = (int)h;
        x0 = (double)(((xi - (align == 1 ? wi : align == 0 ? wi >> 1 : 0))) >> 3) * 8;  /* the blit's byte column */
        y0 = lround(y) - (align == 0 ? hi / 2 : hi);
        wb = iw;
    } else {
        x0 = x - (align == 1 ? w : align == 0 ? w / 2 : 0);
        y0 = y - (align == 0 ? h / 2 : h);
    }
    ras_image(img, iw, ih, x0, y0, x0 + wb, y0 + h, 0x65);              /* blit_clip_y: rows < YMAX */
}

/* ---------------------------------------------------------------- banner (scenery_draw_banner) */

static void banner(const ctx *c, u16 flags, bool cross)
{
    const enh_bound *b = c->b;
    double xl = b->xl, xr = b->xr, ytop = b->y16, ybot = b->y;
    if (c->compare) enh_compare_obj(false, 3, flags >> 12, b->k, 0, xl, ytop, xr);
    double n = b->dash + 1, y0 = fmax(ytop, 0x12), y1 = fmin(ybot, 0x65) + 1;
    ras_rect(xl - n, y0, xl, y1, 10);                                   /* posts: road_vline at xl - si, xr + si */
    ras_rect(xr + 1, y0, xr + 1 + n, y1, 10);
    ras_rect(xl, ytop, xr + 1, ytop + n, 10);                           /* the bar: rows ytop .. ytop + si - 1 */
    double mid = half(c, xl + xr);
    if (flags & 0x1000) { shape(c, mid, ytop, 0, 1, 0x0D, false, 6, false, 0); return; }
    if (!(flags & 0x2000)) {
        if (!cross) { shape(c, mid, ytop, 0, 1, (flags & 0x4000) ? 0x0C : 0x0E, false, 6, false, 0); return; }
        sign(c, mid, ytop, 0, (flags & 0x4000) ? 2 : 3, false);
    } else sign(c, mid, ytop, 0, 1, false);
}

/* ---------------------------------------------------------------- front_objects (race_road.c), modes 0 and 1 */

static u16 flag(u16 arr, int s) { return TRKW(arr, clampi(s, 0, TRK_N - 1)); }

void enh_objects(const enh_bound *b, int mode, bool compare, int pass)
{
    ctx cc = { .b = b, .mode = mode, .pass = pass, .compare = compare }, *c = &cc;
    int s = b->s, M = mode;
    u16 L = flag(TRK_L, s), R = flag(TRK_R, s), C = flag(TRK_C, s);
    if (b->k >= 11 || ((C & 4) && (C & 1))) return;
    bool cross = C & 1;
    int cpu = DSS(DS_cpu_speed), t = M == 0x10 ? 0x15 : 4, u = M == 0x10 ? 0x16 : 0xB;
    double Y = b->y;
    enum { IN = 0, OUT = 1 };
#define SHAPE(x, off, side, type, noclamp, out) shape(c, (x), Y, (off), (side), (type), (noclamp), 6, (out), 0)
    if (M != 0x10 && (L & 0x20) && cpu > 3) SHAPE(b->xl, 0, -1, 0, false, IN);
    if (L & 0x100) {
        if (cross) SHAPE(b->xl, 0, -1, 4, true, IN);
        else if (L & 0x200) SHAPE(2 * b->xol - b->xl, 2, -1, t, true, OUT);
        else SHAPE(b->xol, 0, -1, t, true, OUT);
    }
    if (!(M & 7) && (L & 0x800)) SHAPE(b->xl, 0, 1, 3, false, IN);
    if (!(M & 7) && (L & 0x1000)) SHAPE(b->xl, 0, 1, 6, false, IN);
    if (!(M & 6) && (L & 1)) {
        if (!cross) { if (b->flat_l) SHAPE(b->xl, 0, 1, 1, true, IN); }
        else sign(c, b->xl, half(c, Y + b->ye), 1, 1, IN);
    }
    if (cpu > 3) {
        if ((M & 7) || b->flat_l) {
            if (L & 0x40) SHAPE(b->xol, 0, -1, 0xA, false, OUT);
            if (L & 0x80) {
                if (!cross && (L & 2)) SHAPE(half(c, b->xfl + 0), 3, -1, u, false, OUT);
                else SHAPE(b->xol, 0, -1, u, false, OUT);
            }
        }
        if ((M & 7) || b->flat_r) {
            if (R & 0x40) SHAPE(b->xor_, 0, 1, 0xA, false, OUT);
            if (R & 0x80) {
                if (!cross && (R & 2)) SHAPE(half(c, b->xfr + 0x13F), 3, 1, u, false, OUT);
                else SHAPE(b->xor_ + half(c, b->xor_ - b->xr), 0, 1, u, false, OUT);
            }
        }
    }
    if (!cross) {
        if (cpu > 3) {
            if ((M & 1) || b->flat_l) {
                if (L & 4) SHAPE(b->xl - 2, 0, -1, 7, true, IN);
                if (L & 8) SHAPE(b->xl - 2, 0, -1, 8, true, IN);
            }
            if ((M & 1) || b->flat_r) {
                if (R & 4) SHAPE(b->xr + 2, 0, 1, 7, true, IN);
                if (R & 8) SHAPE(b->xr + 2, 0, 1, 8, true, IN);
            }
        }
    } else {
        if (L & 0x40) shape(c, b->xl, Y, 0, 1, 2, true, 7, IN, 1);      /* hydrants (set 2, 8 LODs) */
        if (R & 0x40) shape(c, b->xr, Y, 0, -1, 2, true, 7, IN, 1);
    }
    if (!cross && (C & 2) && (M != 0x10 || b->flat_r) && !(M & 6)) {
        double span = b->xr - b->xl;
        SHAPE(b->xor_ - (compare ? floor(span / 8) : span / 8), 0, 1, 9, false, IN);
    }
    if (M != 0x10 && (R & 0x20)) SHAPE(b->xr, 0, 1, 0, false, IN);
    if (R & 0x100) {
        if (cross) SHAPE(b->xr, 0, 1, 4, true, IN);
        else if (R & 0x200) SHAPE(2 * b->xor_ - b->xr, 3, 1, t, true, OUT);
        else SHAPE(b->xor_, 0, 1, t, true, OUT);
    }
    if (R & 0x2000) SHAPE(b->xr, 0, 1, 0x12, false, IN);
    if (R & 0x4000) SHAPE(b->xr, 0, 1, 0x13, false, IN);
    if (R & 0x800) SHAPE(b->xr, 0, 1, 3, false, IN);
    if (R & 0x1000) SHAPE(b->xr, 0, -1, 6, false, IN);
    if (!(M & 6) && (R & 1)) {
        if (!cross) { if (b->flat_r) SHAPE(b->xr, 0, -1, 1, true, IN); }
        else sign(c, b->xr, half(c, Y + b->ye), -1, 1, IN);
    }
    if ((C & 0xF000) && !(M & 0x16) && drawn(c, IN)) banner(c, (u16)(C & 0xF000), cross);
    if (C & 0x100) SHAPE(b->xr, 0, 1, 5, false, IN);
    if (C & 0x800) SHAPE(b->xr, 0, 1, 0xF, false, IN);
    if (C & 0x200) SHAPE(b->xr, 0, 1, 0x10, false, IN);
    if (C & 0x400) SHAPE(b->xr, 0, 1, 0x11, false, IN);
#undef SHAPE
}
