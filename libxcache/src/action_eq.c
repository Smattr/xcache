#include "action_t.h"
#include "hash_t.h"
#include <stdbool.h>
#include <string.h>

bool action_eq(const action_t a, const action_t b) {

  if (a.tag != b.tag)
    return false;
  if (strcmp(a.path, b.path) != 0)
    return false;
  if (a.err != b.err)
    return false;

  switch (a.tag) {

  case ACT_ACCESS:
    if (a.access.flags != b.access.flags)
      return false;
    break;

  case ACT_CREATE:
    if (a.create.mode != b.create.mode)
      return false;
    break;

  case ACT_READ:
    if (!hash_eq(a.read.hash, b.read.hash))
      return false;
    break;

  case ACT_STAT:
    if (a.stat.is_lstat != b.stat.is_lstat)
      return false;
    if (a.stat.mode != b.stat.mode)
      return false;
    if (a.stat.uid != b.stat.uid)
      return false;
    if (a.stat.gid != b.stat.gid)
      return false;
    if (a.stat.size != b.stat.size)
      return false;
    // TODO timespec
    break;

  case ACT_WRITE:
    break;
  }

  return true;
}
