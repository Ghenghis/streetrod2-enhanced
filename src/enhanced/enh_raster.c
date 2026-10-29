/* Street Rod 2 Enhanced: sample buffer, polygon fill, resolve through the live palette (ENHANCED.md §3.4).
 * The buffer covers the road view rows ENH_VIEW_Y0 .. ENH_VIEW_Y1 - 1 (the original's view clip) at S = res_scale * aa
 * samples per logical pixel, one palette index per sample. Coordinates given to the fill functions are logical
 * (320 x 200 screen) floats: pixel (x, y) covers [x, x + 1) x [y, y + 1); a sample is inside a polygon when its centre
 * is. Fills are cut to the clip rectangle (ras_clip; the view, or the mirror) and written through a screen door of
 * coverage ras_alpha (1 = opaque; the fade-in of far objects). The resolve averages the aa x aa samples of an output pixel in linear light. */
#include "enh_internal.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "platform/ega.h"

static struct {
    int S, w, h;            /* samples per logical pixel, buffer size in samples */
    u8 *buf;
    int ci0, ci1, cj0, cj1; /* clip rectangle in samples (ras_clip) */
    int alpha;              /* screen-door coverage 0..16 (ras_alpha), 16 = opaque */
} rb;

/* 4 x 4 ordered-dither thresholds: a sample is written when its threshold is below the coverage (screen door; the
 * resolve's aa x aa average turns it into partial coverage) */
static const u8 bayer[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };

bool ras_begin(int S)
{
    int w = 320 * S, h = (ENH_VIEW_Y1 - ENH_VIEW_Y0) * S;
    if (S != rb.S || !rb.buf) {
        u8 *b = realloc(rb.buf, (size_t)w * h);
        if (!b) return false;
        rb.buf = b;
        rb.S = S;
        rb.w = w;
        rb.h = h;
    }
    ras_clip(0, ENH_VIEW_Y0, 320, ENH_VIEW_Y1);
    rb.alpha = 16;
    return true;
}

void ras_clip(double x0, double y0, double x1, double y1)
{
    int S = rb.S;
    rb.ci0 = (int)lround(x0 * S); rb.ci1 = (int)lround(x1 * S);
    rb.cj0 = (int)lround((y0 - ENH_VIEW_Y0) * S); rb.cj1 = (int)lround((y1 - ENH_VIEW_Y0) * S);
    if (rb.ci0 < 0) rb.ci0 = 0;
    if (rb.ci1 > rb.w) rb.ci1 = rb.w;
    if (rb.cj0 < 0) rb.cj0 = 0;
    if (rb.cj1 > rb.h) rb.cj1 = rb.h;
}

void ras_alpha(double a) { rb.alpha = a >= 1.0 ? 16 : a <= 0.0 ? 0 : (int)lround(a * 16.0); }

static bool row_in(int j) { return j >= rb.cj0 && j < rb.cj1; }

int ras_S(void) { return rb.S; }
u8 *ras_row(int j) { return rb.buf + (size_t)j * rb.w; }

static void span(int j, double xa, double xb, u8 c)
{
    if (!row_in(j) || !rb.alpha) return;
    int i0 = (int)ceil(xa - 0.5), i1 = (int)ceil(xb - 0.5);      /* centres i + 0.5 in [xa, xb) */
    if (i0 < rb.ci0) i0 = rb.ci0;
    if (i1 > rb.ci1) i1 = rb.ci1;
    if (i1 <= i0) return;
    u8 *d = rb.buf + (size_t)j * rb.w;
    if (rb.alpha >= 16) { memset(d + i0, c, (size_t)(i1 - i0)); return; }
    const u8 *t = bayer[j & 3];
    for (int i = i0; i < i1; i++) if (t[i & 3] < rb.alpha) d[i] = c;
}

void ras_rect(double x0, double y0, double x1, double y1, u8 c)
{
    if (!rb.buf) return;
    double S = rb.S;
    int j0 = (int)ceil((y0 - ENH_VIEW_Y0) * S - 0.5), j1 = (int)ceil((y1 - ENH_VIEW_Y0) * S - 0.5);
    if (j0 < 0) j0 = 0;
    if (j1 > rb.h) j1 = rb.h;
    for (int j = j0; j < j1; j++) span(j, x0 * S, x1 * S, c);
}

/* A w x h index picture stretched over [x0, x1) x [y0, y1) (logical coordinates), rows below `ymax` cut; nearest
 * texel per sample (the caller picks a picture at most one size step larger than the target, so no mips are needed). */
static void image(const u8 *px, int w, int h, double x0, double y0, double x1, double y1, double ymax, int trans);

void ras_image(const u8 *px, int w, int h, double x0, double y0, double x1, double y1, double ymax)
{
    image(px, w, h, x0, y0, x1, y1, ymax, -1);
}

void ras_sprite(const u8 *px, int w, int h, double x0, double y0, double x1, double y1, double ymax)
{
    image(px, w, h, x0, y0, x1, y1, ymax, ENH_TRANSPARENT);
}

static void image(const u8 *px, int w, int h, double x0, double y0, double x1, double y1, double ymax, int trans)
{
    if (!rb.buf || w <= 0 || h <= 0 || x1 <= x0 || y1 <= y0) return;
    double S = rb.S, sl = x0 * S, sr = x1 * S, st = (y0 - ENH_VIEW_Y0) * S, sb = (y1 - ENH_VIEW_Y0) * S;
    double sm = (ymax - ENH_VIEW_Y0) * S;
    int i0 = (int)ceil(sl - 0.5), i1 = (int)ceil(sr - 0.5), j0 = (int)ceil(st - 0.5), j1 = (int)ceil(fmin(sb, sm) - 0.5);
    if (i0 < rb.ci0) i0 = rb.ci0;
    if (i1 > rb.ci1) i1 = rb.ci1;
    if (j0 < rb.cj0) j0 = rb.cj0;
    if (j1 > rb.cj1) j1 = rb.cj1;
    double du = w / (sr - sl), dv = h / (sb - st);
    for (int j = j0; j < j1; j++) {
        int v = (int)((j + 0.5 - st) * dv);
        if (v < 0 || v >= h) continue;
        const u8 *src = px + (size_t)v * w, *t = bayer[j & 3];
        u8 *out = rb.buf + (size_t)j * rb.w;
        for (int i = i0; i < i1; i++) {
            int u = (int)((i + 0.5 - sl) * du);
            if (u >= 0 && u < w && t[i & 3] < rb.alpha && src[u] != trans) out[i] = src[u];
        }
    }
}

/* A convex polygon of n points (x, y pairs, logical coordinates); a non-convex one fills its rows' extents. */
void ras_poly(const double *xy, int n, u8 c)
{
    if (!rb.buf || n < 3 || n > 16) return;
    double S = rb.S, px[16], py[16], ymin = 1e30, ymax = -1e30;
    for (int k = 0; k < n; k++) {
        px[k] = xy[2 * k] * S;
        py[k] = (xy[2 * k + 1] - ENH_VIEW_Y0) * S;
        if (!isfinite(px[k]) || !isfinite(py[k])) return;
        if (py[k] < ymin) ymin = py[k];
        if (py[k] > ymax) ymax = py[k];
    }
    int j0 = (int)ceil(ymin - 0.5), j1 = (int)ceil(ymax - 0.5);
    if (j0 < 0) j0 = 0;
    if (j1 > rb.h) j1 = rb.h;
    for (int j = j0; j < j1; j++) {
        double yc = j + 0.5, xa = 1e30, xb = -1e30;
        for (int k = 0; k < n; k++) {
            int m = k + 1 == n ? 0 : k + 1;
            double y0 = py[k], y1 = py[m];
            if (y0 == y1 || yc < fmin(y0, y1) || yc >= fmax(y0, y1)) continue;
            double x = px[k] + (yc - y0) * (px[m] - px[k]) / (y1 - y0);
            if (x < xa) xa = x;
            if (x > xb) xb = x;
        }
        if (xb > xa) span(j, xa, xb, c);
    }
}

/* ---------------------------------------------------------------- colour */

u32 enh_pal_rgb(u8 v) { return ega_palette[(v & 7) | (v >> 1 & 8)]; }     /* 200-line attribute decoding */

static u16 lin_of[256];             /* sRGB byte -> linear 0..4095 */
static u8 srgb_of[4096];            /* linear -> sRGB byte */

static void gamma_init(void)
{
    static bool done;
    if (done) return;
    done = true;
    for (int i = 0; i < 256; i++) {
        double c = i / 255.0;
        c = c <= 0.04045 ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4);
        lin_of[i] = (u16)lround(c * 4095.0);
    }
    for (int i = 0; i < 4096; i++) {
        double c = i / 4095.0;
        c = c <= 0.0031308 ? c * 12.92 : 1.055 * pow(c, 1 / 2.4) - 0.055;
        srgb_of[i] = (u8)lround(c * 255.0);
    }
}

/* Output pixels of rows ENH_VIEW_Y0 * rs .. ENH_SPLIT_Y * rs - 1 whose logical pixel has cover[] set (cover: 320 per
 * logical row from ENH_VIEW_Y0) get the aa x aa samples averaged in linear light through palette `pal`. */
void ras_resolve(const u8 pal[17], const u8 *cover, u32 *out, int rs, int aa)
{
    if (!rb.buf || rb.S != rs * aa) return;
    gamma_init();
    u32 rgb[16];
    u16 lr[16], lg[16], lb[16];
    for (int i = 0; i < 16; i++) {
        rgb[i] = enh_pal_rgb(pal[i]);
        lr[i] = lin_of[rgb[i] >> 16 & 0xFF];
        lg[i] = lin_of[rgb[i] >> 8 & 0xFF];
        lb[i] = lin_of[rgb[i] & 0xFF];
    }
    int ow = 320 * rs, n = aa * aa;
    for (int oy = ENH_VIEW_Y0 * rs; oy < ENH_SPLIT_Y * rs; oy++) {
        const u8 *cov = cover + (oy / rs - ENH_VIEW_Y0) * 320;
        u32 *o = out + (size_t)oy * ow;
        int sj = (oy - ENH_VIEW_Y0 * rs) * aa;
        for (int ox = 0; ox < ow; ox++) {
            if (!cov[ox / rs]) continue;
            if (aa == 1) { o[ox] = rgb[ras_row(sj)[ox] & 15]; continue; }
            u32 r = 0, g = 0, b = 0;
            for (int dj = 0; dj < aa; dj++) {
                const u8 *s = ras_row(sj + dj) + ox * aa;
                for (int di = 0; di < aa; di++) {
                    u8 c = s[di] & 15;
                    r += lr[c]; g += lg[c]; b += lb[c];
                }
            }
            o[ox] = (u32)srgb_of[r / n] << 16 | (u32)srgb_of[g / n] << 8 | srgb_of[b / n];
        }
    }
}

/* The view at logical pixels, rows ENH_VIEW_Y0 .. Y1 - 1: the sample at each pixel's centre. (The most frequent index
 * of the S x S block gives the same edges on large areas but noise on the textured sky panorama.) */
void ras_downscale(u8 *idx)
{
    if (!rb.buf) return;
    int S = rb.S;
    for (int y = 0; y < ENH_VIEW_Y1 - ENH_VIEW_Y0; y++) {
        const u8 *s = ras_row(y * S + S / 2);
        for (int x = 0; x < 320; x++) idx[y * 320 + x] = s[x * S + S / 2] & 15;
    }
}
