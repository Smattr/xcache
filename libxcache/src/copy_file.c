#include "copy_file.h"
#include "macros.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int copy_file(const char *src, const char *dst) {

  if (src == NULL)
    return EINVAL;

  if (dst == NULL)
    return EINVAL;

  int in = -1;
  int out = -1;
  int rc = -1;

  in = open(src, O_RDONLY);
  if (in < 0) {
    rc = errno;
    goto done;
  }

  struct stat st;
  if (fstat(in, &st) != 0) {
    rc = errno;
    goto done;
  }

  out = open(dst, O_WRONLY);
  if (out < 0) {
    rc = errno;
    goto done;
  }

  size_t size = st.st_size;
  while (size > 0) {

    ssize_t bytes = sendfile(out, in, NULL, size);
    if (bytes < 0) {
      rc = errno;
      goto done;
    }

    assert((size_t)bytes <= size);
    size -= (size_t)bytes;
  }

  rc = 0;

done:
  if (LIKELY(out >= 0))
    (void)close(out);
  if (LIKELY(in >= 0))
    (void)close(in);

  return rc;
}
