#include "page_size.h"
#include <assert.h>
#include <stddef.h>
#include <unistd.h>

size_t page_size(void) {

  static size_t cached;

  if (cached == 0) {
    long s = sysconf(_SC_PAGESIZE);
    assert(s > 0);
    cached = (size_t)s;
  }

  return cached;
}
