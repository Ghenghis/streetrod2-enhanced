/* Optional four-times-resolution paint for the native parked garage body. */
#include "enh_body.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "enhanced.h"
#include "game/ds_draw.h"
#include "game/ds_ui.h"
#include "platform/ega.h"
#include "platform/video.h"

enum { ART_SCALE = 4, TRANSPARENT = 255 };
static struct {
    SDL_Surface *image;
    int member, width, height, x, y, rows;
    bool captured, parked;
    u8 owner[200][320];
} body = {.member = -1};

static const u32 *art_row(int y)
{
    return (const u32 *)((const u8 *)body.image->pixels + y * body.image->pitch);
}

void enh_body_clear(void) { body.captured = body.parked = false; }

void enh_body_shutdown(void)
{
    enh_body_clear();
    SDL_DestroySurface(body.image);
    body.image = NULL;
    body.member = -1;
}

static SDL_Surface *load_art(int member, bool restored)
{
    char *path = NULL;
    if (SDL_asprintf(&path, "%s/%03d%s.bmp", enh.body_art, member, restored ? "-restored" : "") < 0)
        return NULL;
    SDL_Surface *loaded = SDL_LoadBMP(path);
    SDL_free(path);
    SDL_Surface *converted = loaded ? SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_ARGB8888) : NULL;
    SDL_DestroySurface(loaded);
    return converted;
}

static bool capture_art(FarPtr sprite)
{
    if (!body.image) return false;
    const int w = desc_w(sprite), h = desc_h(sprite), stride = desc_stride(sprite);
    FarPtr mask = desc_mask(sprite), bits = desc_planes(sprite);
    if (w < 1 || w > 320 || h < 1 || h > 200 || far_is_null(mask) ||
        body.image->w != ART_SCALE * w || body.image->h != ART_SCALE * h) return false;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int off = y * stride + x / 8;
            u8 bit = (u8)(0x80 >> (x % 8));
            bool solid = (rd8(mask.seg, (u16)(mask.off + off)) & bit) == 0;
            for (int dy = 0; dy < ART_SCALE; ++dy) {
                const u32 *ink = art_row(y * ART_SCALE + dy) + x * ART_SCALE;
                for (int dx = 0; dx < ART_SCALE; ++dx)
                    if (ink[dx] >> 24 != (solid ? 255u : 0u)) return false;
            }
            u8 index = 0;
            for (int plane = 0; plane < 4; ++plane)
                if (rd8(bits.seg, (u16)(bits.off + off + plane * h * stride)) & bit) index |= 1 << plane;
            body.owner[y][x] = solid ? index : TRANSPARENT;
        }
    }
    body.width = w;
    body.height = h;
    return true;
}

void enh_body_capture(int member, FarPtr sprite)
{
    if (!enh.body_art || enh.classic || member < 0) return;
    if (member == body.member) {
        body.captured = capture_art(sprite);
        return;
    }
    enh_body_shutdown();
    body.member = member;
    if (enh.body_art_restored) {
        body.image = load_art(member, true);
        if (capture_art(sprite)) {
            body.captured = true;
            return;
        }
        SDL_DestroySurface(body.image);
    }
    body.image = load_art(member, false);
    body.captured = capture_art(sprite);
}

void enh_body_park(int x, int y, int rows)
{
    if (!body.captured || rows <= 0 || rows > body.height) return;
    body.x = x;
    body.y = y;
    body.rows = rows;
    body.parked = true;
    ega_touch();
}

static u8 active_index(u16 page, int x, int y)
{
    unsigned off = (page + y * EGA_BYTES_PER_LINE + x / 8) & (EGA_PLANE_SIZE - 1);
    u8 bit = (u8)(0x80 >> (x % 8)), index = 0;
    for (int plane = 0; plane < 4; ++plane)
        if (ega_plane(plane)[off] & bit) index |= 1 << plane;
    return index;
}

static u32 live_paint(u32 ink, u32 color)
{
    /* Blue is the authored paint key. Use continuous chroma so legacy
     * dithered palette slots do not appear as a checkerboard over the art. */
    const int red = (ink >> 16) & 255, green = (ink >> 8) & 255, blue = ink & 255;
    int weight = 6 * (blue - red - 10);
    if (weight <= 0) return ink;
    if (weight > 255) weight = 255;
    int light = (3 * red + 6 * green + blue) / 10;
    u32 result = 0;
    for (unsigned shift = 0; shift <= 16; shift += 8) {
        int c = (color >> shift) & 255, k = (ink >> shift) & 255;
        int shade = c * light / 160 + (255 - c) * light * light / (255 * 272);
        if (shade > 255) shade = 255;
        result |= (u32)((shade * weight + k * (255 - weight) + 127) / 255) << shift;
    }
    return result;
}

void enh_body_compose(u32 *out, int scale)
{
    int depth = DSS(DS_ui_depth);
    u16 page = ega_get_start();
    if (!body.parked || DSS(DS_location) != 1 || page != 0 || depth <= 0 || !ega_get_palette_reg(15) ||
        DSS((u16)(DS_ui_stack + (depth - 1) * 2)) != 0) return;
    u8 paint = ega_get_palette_reg(6);
    u32 color = ega_palette[(paint & 7) | (paint >> 1 & 8)];
    const int pitch = 320 * scale;
    int visible = 0;
    for (int y = 0; y < body.rows; ++y) {
        int sy = body.y + y;
        if (sy < 0 || sy >= 200) continue;
        for (int x = 0; x < body.width; ++x) {
            int sx = body.x + x;
            u8 owner = body.owner[y][x];
            if (sx < 0 || sx >= 320 || owner == TRANSPARENT || active_index(page, sx, sy) != owner) continue;
            ++visible;
            for (int dy = 0; dy < scale; ++dy) {
                const u32 *ink = art_row(y * ART_SCALE + dy * ART_SCALE / scale) + x * ART_SCALE;
                u32 *dst = out + (sy * scale + dy) * pitch + sx * scale;
                for (int dx = 0; dx < scale; ++dx) {
                    u32 pixel = ink[dx * ART_SCALE / scale] & 0xffffff;
                    dst[dx] = live_paint(pixel, color);
                }
            }
        }
    }
    if (visible && getenv("SR_BODY_TRACE"))
        fprintf(stderr, "body: lib2=%d x=%d y=%d rows=%d pixels=%d\n",
                body.member, body.x, body.y, body.rows, visible);
}
