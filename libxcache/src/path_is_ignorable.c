#include "path.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

bool path_is_ignorable(const char *abs_path) {
  assert(abs_path != NULL);

  // we do not need to record access to things that are not real files (e.g.
  // “pipe:…”)
  if (abs_path[0] != '/')
    return true;

  // memfds show up as deleted for some reason
  const char MEMFD_PREFIX[] = "/memfd:";
  if (strncmp(abs_path, MEMFD_PREFIX, strlen(MEMFD_PREFIX)) == 0) {
    const char MEMFD_SUFFIX[] = " (deleted)";
    const size_t suffix_len = strlen(MEMFD_SUFFIX);
    const size_t len = strlen(abs_path);
    if (len >= suffix_len) {
      if (strcmp(&abs_path[len - suffix_len], MEMFD_SUFFIX) == 0)
        return true;
    }
  }

  // we have already recorded /proc/self/exe on startup
  if (strcmp(abs_path, "/proc/self/exe") == 0)
    return true;

  // we have effectively already recorded a read of /proc/self/cmdline through
  // the name of the command
  if (strcmp(abs_path, "/proc/self/cmdline") == 0)
    return true;

  // we have effectively already recorded /proc/self/maps through loading the
  // tracee
  if (strcmp(abs_path, "/proc/self/maps") == 0)
    return true;

  return false;
}
