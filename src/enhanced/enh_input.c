/* Street Rod 2 Enhanced: input options (S3.1): bindable shift keys, gamepad shoulder / trigger / Start extras. */
#include "enhanced.h"

#include <SDL3/SDL.h>

#include "platform/platform.h"

int enh_gear_sc(int dir)
{
    if (enh.classic) return dir > 0 ? 0x1E : 0x2C;
    if (!enh.gear_keys) return 0;
    return dir > 0 ? enh.key_shift_up : enh.key_shift_down;
}

int enh_pad_button(int button, bool down)
{
    if (enh.classic || !enh.pad_extras) return 0;
    if (button == SDL_GAMEPAD_BUTTON_START) return 0x01;                /* Esc */
    if (down && button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) kbd_push_shift_dir(1);
    if (down && button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) kbd_push_shift_dir(-1);
    return 0;
}

void enh_pad_axes(int rt, int lt, short *ay)
{
    if (enh.classic || !enh.pad_extras) return;
    if (rt > 8000) *ay = -32768;                                        /* gas: stick forward */
    else if (lt > 8000) *ay = 32767;                                    /* brake */
}
