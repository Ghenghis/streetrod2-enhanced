#include "enhanced.h"

#include <string.h>

enh_options enh;

bool enh_parse_arg(const char *arg)
{
    if (!strcmp(arg, "--classic")) { enh.classic = true; return true; }
    return false;
}
