#include "path.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

bool path_is_cacheable(const char *abs_path) {
  assert(abs_path != NULL);
  assert(abs_path[0] == '/');

  // consider opening any device except well known ones unsupported
  if (strncmp(abs_path, "/dev/", strlen("/dev/")) == 0) {
    // note that the pre-opened FDs 0-2 are handled separately during spawn
    const char *SAFE[] = {"/dev/null"};
    for (size_t i = 0; i < sizeof(SAFE) / sizeof(SAFE[0]); ++i) {
      if (strcmp(abs_path, SAFE[i]) == 0)
        return true;
    }
    return false;
  }

  // consider opening any proc file unsupported (though see `path_is_ignorable`)
  if (strncmp(abs_path, "/proc/", strlen("/proc/")) == 0)
    return false;

  // consider opening any sysfs file unsupported
  if (strncmp(abs_path, "/sys/", strlen("/sys/")) == 0)
    return false;

  return true;
}
