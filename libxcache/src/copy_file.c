#include "copy_file.h"
#include "debug.h"
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

  if (ERROR(src == NULL))
    return EINVAL;

  if (ERROR(dst == NULL))
    return EINVAL;

  int in = -1;
  int out = -1;
  int rc = -1;

  // open the source file
  in = open(src, O_RDONLY);
  if (ERROR(in < 0)) {
    rc = errno;
    goto done;
  }

  // learn the size of the source file
  struct stat st;
  if (ERROR(fstat(in, &st) != 0)) {
    rc = errno;
    goto done;
  }

  // open the destination file
  out = open(dst, O_WRONLY);
  if (ERROR(out < 0)) {
    rc = errno;
    goto done;
  }

  // perform the copy
  size_t size = st.st_size;
  while (size > 0) {

    ssize_t bytes = sendfile(out, in, NULL, size);
    if (ERROR(bytes < 0)) {
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
