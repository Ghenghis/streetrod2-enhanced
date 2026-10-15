/* Street Rod 2 Enhanced: options on top of the faithful port (see ENHANCED.md).
 * Every edit to engine code outside src/enhanced/ is marked with an ENH: comment.
 * --classic switches every enhancement off: behaviour and picture as the faithful release. */
#ifndef ENHANCED_H
#define ENHANCED_H

#include <stdbool.h>

typedef struct {
    bool classic;           /* --classic: all enhancements off */
} enh_options;

extern enh_options enh;

/* Parses one command-line argument; returns true when it was an Enhanced option. */
bool enh_parse_arg(const char *arg);

#endif
