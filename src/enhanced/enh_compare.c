/* Street Rod 2 Enhanced: the compare tool (ENHANCED.md §5, acceptance §3.10 item 3).
 * SR_ENH_COMPARE=2 (with the deterministic clock of =1): at every road_page_flip of a race drawn enhanced, the frame's
 * snapshot is rendered at the configured resolution without interpolation (motion delay 0) and with the player position
 * quantized as the original's row depths (substep / 8 truncated), box-filtered back to 320 x 200 (the most frequent
 * index per pixel) and laid over the original's page where the original's pixels show (passthrough sprites, mirror).
 * The colour edges of both pictures are then matched: every horizontal edge (between x - 1 and x on a row) and vertical
 * edge of the original must have an edge with the same two colours within +-1 px in the enhanced picture, and the
 * other way round. Rows from the top of the ground (DS:975E; the town: the view top) to the split; the sky panorama,
 * which scrolls at 1/8 of the original rate, is above them.
 * Output (SR_ENH_COMPARE_LOG=file, default stderr), one line per frame and one per race:
 *   K frame mode seg substep  h orig_matched/orig_edges enh_matched/enh_edges  v ...
 *   K race frames N  h ... v ...  worst frame F pct
 * SR_ENH_COMPARE=3 also lists the unmatched edges of each frame (first 12). SR_ENH_COMPARE_DIR=dir writes every
 * SR_ENH_COMPARE_EVERY-th (default 25) compared frame as cmpNNNNN.bmp: original, enhanced, difference. */
#include "enh_internal.h"
#include "enhanced.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "game/race_int.h"
#include "platform/ega.h"

#define ROWS (ENH_VIEW_Y1 - ENH_VIEW_Y0)

static struct {
    int level;
    FILE *log;
    const char *dir;
    int every;
    unsigned frames, dumped;
    unsigned long hn, hm, hen, hem, vn, vm, ven, vem;   /* race totals */
    double worst;
    unsigned worst_frame;
    unsigned long miss_y[ROWS], miss_pair[16][16];  /* unmatched edges by row and colour pair (SR_ENH_COMPARE=3) */
} cmp;

void enh_compare_init(int level)
{
    cmp.level = level;
    const char *f = SDL_getenv("SR_ENH_COMPARE_LOG");
    cmp.log = f && *f ? fopen(f, "w") : NULL;
    if (!cmp.log) cmp.log = stderr;
    cmp.dir = SDL_getenv("SR_ENH_COMPARE_DIR");
    const char *e = SDL_getenv("SR_ENH_COMPARE_EVERY");
    cmp.every = e && atoi(e) > 0 ? atoi(e) : 25;
    fprintf(cmp.log, "# K frame mode seg substep  h orig_matched/orig_edges enh_matched/enh_edges  v (same)\n");
}

bool enh_mirror_px(int x, int y)
{
    return (x >= 0xF0 && y >= 0x14 && y < 0x2D) || (x >= 0x110 && x < 0x120 && y >= 0x12 && y < 0x14);
}

static u8 page_px(int page, int x, int y)
{
    u16 o = (u16)((page == 1 ? 0x2000 : 0x4000) + y * EGA_BYTES_PER_LINE + x / 8);
    int b = 7 - x % 8;
    return (u8)((ega_plane(0)[o] >> b & 1) | (ega_plane(1)[o] >> b & 1) << 1 | (ega_plane(2)[o] >> b & 1) << 2
              | (ega_plane(3)[o] >> b & 1) << 3);
}

static u8 O[ROWS][320], E[ROWS][320], X[ROWS][320];     /* original, enhanced, excluded */

static bool px_is(u8 (*P)[320], int x, int y, u8 c)
{
    return x >= 0 && y >= 0 && x < 320 && y < ROWS && !X[y][x] && P[y][x] == c;
}

/* An edge of A between pixels p (colour a) and its neighbour q (colour b) is matched in B by any edge p'-q' of the
 * same colours (either orientation) with p' within 1 px of p and q' within 1 px of q: an edge within +-1 px. */
static bool edge_near(u8 (*B)[320], int px, int py, int qx, int qy, u8 a, u8 b)
{
    static const int nb[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    for (int ey = -1; ey <= 1; ey++)
        for (int ex = -1; ex <= 1; ex++) {
            int x = px + ex, y = py + ey;
            if (!px_is(B, x, y, a)) continue;
            for (int k = 0; k < 4; k++) {
                int x2 = x + nb[k][0], y2 = y + nb[k][1];
                if (abs(x2 - qx) <= 1 && abs(y2 - qy) <= 1 && px_is(B, x2, y2, b)) return true;
            }
        }
    return false;
}

/* The boundary of colour a in B near p, towards q: a pixel of colour a within 1 px of p with a 4-neighbour of another
 * colour within 1 px of q. */
static bool boundary_near(u8 (*B)[320], int px, int py, int qx, int qy, u8 a)
{
    static const int nb[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    for (int ey = -1; ey <= 1; ey++)
        for (int ex = -1; ex <= 1; ex++) {
            int x = px + ex, y = py + ey;
            if (!px_is(B, x, y, a)) continue;
            for (int k = 0; k < 4; k++) {
                int x2 = x + nb[k][0], y2 = y + nb[k][1];
                if (abs(x2 - qx) <= 1 && abs(y2 - qy) <= 1 && x2 >= 0 && y2 >= 0 && x2 < 320 && y2 < ROWS
                    && !X[y2][x2] && B[y2][x2] != a) return true;
            }
        }
    return false;
}

/* An edge a|b at p|q is within +-1 px in B: the same edge, or (a textured picture behind one side, e.g. the rooftop
 * panorama above a building) the boundaries of both colours. */
static bool matched(u8 (*B)[320], int px, int py, int qx, int qy, u8 a, u8 b)
{
    return edge_near(B, px, py, qx, qy, a, b)
        || (boundary_near(B, px, py, qx, qy, a) && boundary_near(B, qx, qy, px, py, b));
}

static void count(u8 (*A)[320], u8 (*B)[320], int y0, int dx, int dy, unsigned *n, unsigned *m, bool list,
                  const char *what)
{
    int shown = 0;
    for (int y = y0 + dy; y < ENH_SPLIT_Y - ENH_VIEW_Y0; y++)
        for (int x = dx; x < 320; x++) {
            u8 a = A[y - dy][x - dx], b = A[y][x];
            if (a == b || X[y][x] || X[y - dy][x - dx]) continue;
            (*n)++;
            if (matched(B, x - dx, y - dy, x, y, a, b)) { (*m)++; continue; }
            cmp.miss_y[y]++;
            cmp.miss_pair[a][b]++;
            if (list && shown++ < 12)
                fprintf(cmp.log, "  %s edge %d->%d at x %d y %d unmatched\n", what, a, b, x, y + ENH_VIEW_Y0);
        }
}

static void dump(unsigned frame, const u8 pal[17])
{
    char path[512];
    snprintf(path, sizeof path, "%s/cmp%05u.bmp", cmp.dir, frame);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    int w = 320, h = 3 * ROWS, bpr = w * 3;
    u8 hdr[54] = { 'B', 'M' };
    u32 size = 54u + (u32)bpr * h;
    memcpy(hdr + 2, &size, 4);
    hdr[10] = 54; hdr[14] = 40;
    memcpy(hdr + 18, &w, 4);
    memcpy(hdr + 22, &h, 4);
    hdr[26] = 1; hdr[28] = 24;
    fwrite(hdr, 1, 54, f);
    u8 row[320 * 3];
    for (int r = h - 1; r >= 0; r--) {                  /* bottom-up: original on top, enhanced, difference */
        int panel = r / ROWS, y = r % ROWS;
        for (int x = 0; x < 320; x++) {
            u32 c;
            if (panel == 0) c = enh_pal_rgb(pal[O[y][x]]);
            else if (panel == 1) c = enh_pal_rgb(pal[E[y][x]]);
            else c = X[y][x] ? 0x202020 : O[y][x] != E[y][x] ? 0xFF2020 : (enh_pal_rgb(pal[O[y][x]]) >> 2 & 0x3F3F3F);
            row[3 * x] = (u8)c; row[3 * x + 1] = (u8)(c >> 8); row[3 * x + 2] = (u8)(c >> 16);
        }
        fwrite(row, 1, sizeof row, f);
    }
    fclose(f);
    snprintf(path, sizeof path, "%s/cmp%05u.idx", cmp.dir, frame);     /* raw indices: O, E, X (84 x 320 each) */
    if ((f = fopen(path, "wb"))) {
        fwrite(O, 1, sizeof O, f);
        fwrite(E, 1, sizeof E, f);
        fwrite(X, 1, sizeof X, f);
        fclose(f);
    }
}

void enh_compare_frame(const enh_snap *s, int page)
{
    if (cmp.level < 2 || (page != 1 && page != 2)) return;
    enh_view v;
    memset(&v, 0, sizeof v);
    v.a = v.b = s;
    v.x = v.w = 1.0;
    v.P = s->seg * 64.0 + (s->substep & ~7);                /* the original's row depths: substep / 8 truncated */
    v.heading = s->heading;
    v.rel_heading = s->heading - enh_track_yaw(v.P);
    v.cam_x = s->cam_x;
    v.cam_y = s->cam_y;
    v.cur_height = s->cur_height;
    v.bounce_y = s->bounce_y;
    v.compare = true;
    if (!enh_road_render(&v)) return;
    if (cmp.level >= 3) enh_road_debug_rows(&v, cmp.log);
    ras_downscale(&E[0][0]);
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < 320; x++) {
            int sy = y + ENH_VIEW_Y0;
            O[y][x] = page_px(page, x, sy);
            X[y][x] = enh_mirror_px(x, sy);
            if (X[y][x] || enh_written(page, x, sy)) E[y][x] = O[y][x];
        }
    int top = s->racing ? DSS(DS_ground_top_y) : ENH_VIEW_Y0;
    if (top < ENH_VIEW_Y0) top = ENH_VIEW_Y0;
    int y0 = top - ENH_VIEW_Y0;
    unsigned hn = 0, hm = 0, hen = 0, hem = 0, vn = 0, vm = 0, ven = 0, vem = 0;
    bool list = cmp.level >= 3;
    count(O, E, y0, 1, 0, &hn, &hm, list, "h orig");
    count(E, O, y0, 1, 0, &hen, &hem, list, "h enh");
    count(O, E, y0, 0, 1, &vn, &vm, list, "v orig");
    count(E, O, y0, 0, 1, &ven, &vem, list, "v enh");
    fprintf(cmp.log, "K %u %d %d %d  h %u/%u %u/%u  v %u/%u %u/%u\n", s->frame, s->mode, s->seg, s->substep,
            hm, hn, hem, hen, vm, vn, vem, ven);
    cmp.hn += hn; cmp.hm += hm; cmp.hen += hen; cmp.hem += hem;
    cmp.vn += vn; cmp.vm += vm; cmp.ven += ven; cmp.vem += vem;
    unsigned tn = hn + hen + vn + ven, tm = hm + hem + vm + vem;
    double pct = tn ? 100.0 * tm / tn : 100.0;
    if (!cmp.frames || pct < cmp.worst) { cmp.worst = pct; cmp.worst_frame = s->frame; }
    if (cmp.dir && *cmp.dir && cmp.frames % (unsigned)cmp.every == 0) dump(cmp.dumped++, s->pal);
    cmp.frames++;
}

void enh_compare_race_end(void)
{
    if (cmp.level < 2 || !cmp.frames) return;
    fprintf(cmp.log, "K race frames %u  h %lu/%lu %lu/%lu  v %lu/%lu %lu/%lu  worst frame %u %.2f%%\n", cmp.frames,
            cmp.hm, cmp.hn, cmp.hem, cmp.hen, cmp.vm, cmp.vn, cmp.vem, cmp.ven, cmp.worst_frame, cmp.worst);
    if (cmp.level >= 3) {
        fprintf(cmp.log, "K unmatched by row:");
        for (int y = 0; y < ROWS; y++) if (cmp.miss_y[y]) fprintf(cmp.log, " %d:%lu", y + ENH_VIEW_Y0, cmp.miss_y[y]);
        fprintf(cmp.log, "\nK unmatched by colours:");
        for (int a = 0; a < 16; a++)
            for (int b = 0; b < 16; b++)
                if (cmp.miss_pair[a][b] > 50) fprintf(cmp.log, " %d>%d:%lu", a, b, cmp.miss_pair[a][b]);
        fprintf(cmp.log, "\n");
    }
    fflush(cmp.log);
    unsigned dumped = cmp.dumped;
    int level = cmp.level, every = cmp.every;
    FILE *log = cmp.log;
    const char *dir = cmp.dir;
    memset(&cmp, 0, sizeof cmp);
    cmp.level = level; cmp.every = every; cmp.log = log; cmp.dir = dir; cmp.dumped = dumped;
}
