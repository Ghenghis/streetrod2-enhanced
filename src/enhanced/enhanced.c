/* Street Rod 2 Enhanced: options, hooks, compose, written mask and layers (ENHANCED.md §2, §2.1, §3.9).
 * S2.1: the host and coverage foundation. The output frame is the EGA picture scaled nearest-neighbour; the
 * enhanced view (later subtasks) is drawn by enh_compose where coverage allows. */
#include "enhanced.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void enh_init(void)
{
    if (enh.classic) return;
    while (enh.res_scale * enh.aa > 16 && enh.aa > 1) enh.aa--;       /* res_scale * aa <= 16 (§3.9) */
}

bool enh_hires(void) { return !enh.classic; }

/* ---------------------------------------------------------------- race state, layers */

#define PAGE_BYTES  (EGA_BYTES_PER_LINE * 200)      /* 8000: one bit per pixel, as a plane */
#define PAGE1_OFF   0x2000u                         /* A200:0000 */
#define PAGE2_OFF   0x4000u                         /* A400:0000 */

static struct {
    bool overlay;           /* a race of an enabled mode is running */
    bool bg;                /* inside a race frame, background layer */
    bool draws_view;        /* the enhanced view is drawn over the EGA picture (none yet: S2.1) */
    int shown_page;         /* page (1, 2) presented by road_page_flip with a frame, 0 = none */
    u8 written[2][PAGE_BYTES];  /* per race page: bit set = the original's pixel shows (§2.1) */
} rs;

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
    rs.bg = false;
    rs.shown_page = 0;
    memset(rs.written, 0xFF, sizeof rs.written);
    enh.track = rs.overlay;
}

void enh_race_end(void)
{
    rs.overlay = false;
    rs.bg = false;
    rs.shown_page = 0;
    enh.track = false;
}

void enh_frame_begin(void) { if (rs.overlay) rs.bg = true; }
void enh_layer_fg(void) { rs.bg = false; }
void enh_layer_bg(void) { if (rs.overlay) rs.bg = true; }

void enh_frame_presented(int page)
{
    if (rs.overlay) rs.shown_page = (page == 1 || page == 2) ? page : 0;
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

bool enh_compose(bool (*ega_compose)(u32 *xrgb), u32 *out)
{
    bool changed = ega_compose(pic);
    /* While the enhanced view is drawn it changes on every call: the host then presents at the display rate (§2).
     * Until a subtask draws the view (S2.3) the output is the scaled EGA picture and changes with it only. */
    if (!changed && !(rs.overlay && rs.draws_view)) return false;
    int n = enh.res_scale, w = 320 * n;
    for (int y = 0; y < 200; y++) {
        u32 *row = out + (size_t)y * n * w;
        const u32 *src = pic + y * 320;
        for (int x = 0; x < 320; x++)
            for (int k = 0; k < n; k++) *row++ = src[x];
        row -= w;
        for (int k = 1; k < n; k++) memcpy(row + (size_t)k * w, row, (size_t)w * sizeof *row);
    }
    return true;
}
