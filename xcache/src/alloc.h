#pragma once

#include "../../common/compiler.h"

INTERNAL char *xstrdup(const char *s);

INTERNAL int xasprintf(char **restrict strp, const char *restrict fmt, ...);
