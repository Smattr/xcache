#pragma once

#include "../../common/compiler.h"

INTERNAL char *xstrdup(const char *s);

INTERNAL void xasprintf(char **restrict strp, const char *restrict fmt, ...);
