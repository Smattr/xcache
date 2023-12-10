#include "output_t.h"
#include <stdbool.h>
#include <string.h>

bool output_eq(const output_t a, const output_t b) {

  if (a.tag != b.tag)
    return false;

  if (strcmp(a.path, b.path) != 0)
    return false;

  switch (a.tag) {

  case OUT_CHMOD:
    if (a.chmod.mode != b.chmod.mode)
      return false;
    break;

  case OUT_CHOWN:
    if (a.chown.uid != b.chown.uid)
      return false;
    if (a.chown.gid != b.chown.gid)
      return false;
    break;

  case OUT_MKDIR:
    if (a.mkdir.mode != b.mkdir.mode)
      return false;
    break;

  case OUT_WRITE:
    // ignore cached copy
    break;
  }

  return true;
}
