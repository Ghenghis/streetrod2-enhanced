#ifndef ENH_BODY_H
#define ENH_BODY_H

#include "mem.h"

void enh_body_capture(int member, FarPtr sprite);
void enh_body_park(int x, int y, int rows);
void enh_body_clear(void);
void enh_body_shutdown(void);
void enh_body_compose(u32 *out, int scale);

#endif
