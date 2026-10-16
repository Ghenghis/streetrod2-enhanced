/* Street Rod 2 Enhanced: options on top of the faithful port (see ENHANCED.md).
 * Every edit to engine code outside src/enhanced/ is marked with an ENH: comment.
 * --classic switches every enhancement off: behaviour and picture as the faithful release. */
#ifndef ENHANCED_H
#define ENHANCED_H

#include <stdbool.h>

#include "types.h"

/* Race road modes (DS_road_mode) as bits of enh_options.modes (--enh-modes). */
#define ENH_MODE_TOWN       0x01    /* mode 0: town cruise */
#define ENH_MODE_DRAG       0x02    /* mode 1: street drag race */
#define ENH_MODE_STRIP      0x04    /* modes 2, 4: drag strip */
#define ENH_MODE_MULHOLLAND 0x08    /* mode 0x10 */
#define ENH_MODE_AQUEDUCT   0x10    /* mode 8 */
#define ENH_MODE_ALL        0x1F

typedef struct {
    bool classic;           /* --classic: all enhancements off */
    int res_scale;          /* --res-scale 1..8: output frame 320*N x 200*N (default 4) */
    int aa;                 /* --aa 1..4: samples per axis, res_scale * aa <= 16 (default 2) */
    int motion_delay;       /* --motion-delay MS: interpolation delay, 0 = latest snapshot (default 110) */
    unsigned modes;         /* --enh-modes LIST: ENH_MODE_* drawn enhanced (default all) */
    bool track;             /* engine hook gate: the written mask is kept (a race with the overlay on) */
} enh_options;

extern enh_options enh;

/* Parses one command-line argument `arg` (its value, if any, is `val`, may be NULL). Returns the number of
 * arguments used (1 or 2), 0 when it is not an Enhanced option or its value is missing / invalid. */
int enh_parse_arg(const char *arg, const char *val);
/* After host_init: validates the options. */
void enh_init(void);
/* True when the output frame is the Enhanced one (320*res_scale x 200*res_scale). */
bool enh_hires(void);
/* Compose hook of platform/ega.c: `ega_compose` fills a 320x200 picture; the result is the output frame. */
bool enh_compose(bool (*ega_compose)(u32 *xrgb), u32 *out);

/* Race hooks (ENHANCED.md §2). */
void enh_race_begin(int road_mode);     /* race_drive.c, after shp_load_set() */
void enh_race_end(void);                /* race_drive.c, before arena_reset() at the end of race_run */
void enh_frame_begin(void);             /* first line of road_frame: start of the background layer */
void enh_layer_fg(void);                /* before road_fillmode_end(): the foreground layer starts */
void enh_layer_bg(void);                /* back to the background layer (passthrough brackets) */
void enh_frame_presented(int page);
/* around front_objects (road_draw_front): its pixels belong to the enhanced view where it draws the objects (S2.5),
 * else the original's pixels show (passthrough) */
void enh_objects_begin(void);
void enh_objects_end(void);
/* compare record of an original road-side object call (scenery_draw_*): kind 0 shape, 1 hydrant, 2 sign picture,
 * 3 banner; x2 = banner right edge */
void enh_obj_orig(int kind, int type, int x, int y, int depth, int side, int x2);     /* road_page_flip, before ega_show_page: page 1 or 2 shows this frame */
/* end of opponent_draw's choice: view 0 front / 1 mirror (level s >= 9), sprite level s, variant v, blitted or not */
void enh_opp_drawn(int view, int s, int v, bool drawn);

/* Written mask (ENHANCED.md §2.1), from vga_wr while enh.track: the four plane bytes at `off` before the write
 * (enh_vram_peek) and after it (enh_vram_write). */
u32 enh_vram_peek(u16 off);
void enh_vram_write(u16 off, u32 before);
/* True when pixel (x, y) of race page 1 or 2 was last written by the foreground (shows the original's pixel). */
bool enh_written(int page, int x, int y);

#endif
