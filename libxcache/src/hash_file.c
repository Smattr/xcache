#include "debug.h"
#include "macros.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <xcache/hash.h>

int xc_hash_file(xc_hash_t *hash, const char *path) {

  if (ERROR(hash == NULL))
    return EINVAL;

  if (ERROR(path == NULL))
    return EINVAL;

  int rc = -1;
  void *base = MAP_FAILED;
  size_t size = 0;

  int fd = open(path, O_RDONLY);
  if (ERROR(fd < 0)) {
    rc = errno;
    goto done;
  }

  struct stat st;
  if (ERROR(fstat(fd, &st) != 0)) {
    rc = errno;
    goto done;
  }
  size = st.st_size;

  base = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (ERROR(base == MAP_FAILED)) {
    rc = errno;
    goto done;
  }

  rc = xc_hash_data(hash, base, size);

done:
  if (LIKELY(base != MAP_FAILED))
    (void)munmap(base, size);
  if (LIKELY(fd >= 0))
    (void)close(fd);

  return rc;
}
