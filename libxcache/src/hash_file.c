#include "debug.h"
#include "hash_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int hash_file(const char *path, hash_t *hash) {

  assert(path != NULL);
  assert(hash != NULL);

  int fd = -1;
  void *base = MAP_FAILED;
  size_t size = 0;
  int rc = 0;

  fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    rc = errno;
    goto done;
  }

  struct stat st;
  if (ERROR(fstat(fd, &st) < 0)) {
    rc = errno;
    goto done;
  }
  size = st.st_size;

  if (S_ISDIR(st.st_mode)) {
    rc = EISDIR;
    goto done;
  }

  base = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (ERROR(base == MAP_FAILED)) {
    rc = errno;
    goto done;
  }

  *hash = hash_data(base, size);

done:
  if (base != MAP_FAILED)
    (void)munmap(base, size);
  if (fd >= 0)
    (void)close(fd);

  return rc;
}
