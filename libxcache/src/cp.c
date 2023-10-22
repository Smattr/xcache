#include "cp.h"
#include "debug.h"
#include <assert.h>
#include <stddef.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

int cp(int dst, int src) {

  assert(dst >= 0);
  assert(src >= 0);

  int rc = 0;

  // learn the size of the source
  struct stat st;
  if (ERROR(fstat(src, &st) < 0)) {
    rc = errno;
    goto done;
  }

  // perform the copy
  size_t size = st.st_size;
  while (size > 0) {
    ssize_t bytes = sendfile(dst, src, NULL, size);
    if (ERROR(bytes < 0)) {
      rc = errno;
      goto done;
    }

    assert((size_t)bytes <= size);
    size -= (size_t)bytes;
  }

done:
  return rc;
}
