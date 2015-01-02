#ifndef _XCACHE_HOOK_H_
#define _XCACHE_HOOK_H_

#include "collection/dict.h"
#include "trace.h"

int hook_create(target_t *target);

int hook_close(target_t *target);

#endif
