#include "debug.h"
#include "hash_t.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int hash_file(const char *path, hash_t *hash) {

  assert(path != NULL);
  assert(hash != NULL);

  int fd = -1;
  FILE *f = NULL;
  void *base = MAP_FAILED;
  size_t size = 0;
  char *buffer = NULL;
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

  if (size > 0) {
    base = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ERROR(base == MAP_FAILED)) {
      // if we could not mmap the file, try reading it into memory as a fallback
      f = fdopen(fd, "r");
      if (ERROR(f == NULL)) {
        rc = errno;
        goto done;
      }
      fd = -1;
      buffer = malloc(size);
      if (ERROR(buffer == NULL)) {
        rc = ENOMEM;
        goto done;
      }
      if (ERROR(fread(buffer, size, 1, f) != 1)) {
        rc = EIO;
        goto done;
      }
      *hash = hash_data(buffer, size);
    } else {
      *hash = hash_data(base, size);
    }
  } else {
    *hash = hash_data(NULL, 0);
  }

done:
  free(buffer);
  if (f != NULL)
    (void)fclose(f);
  if (base != MAP_FAILED)
    (void)munmap(base, size);
  if (fd >= 0)
    (void)close(fd);

  return rc;
}
