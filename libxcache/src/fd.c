#include "fd.h"
#include "debug.h"
#include "list.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/// deallocate a file descriptor
static void fd_free(fd_t *fd) {

  if (fd == NULL)
    return;

  free(fd->path);
  free(fd);
}

/// allocate a new file descriptor
static fd_t *fd_new(const char *path) {
  assert(path != NULL);

  fd_t *f = NULL;
  fd_t *ret = NULL;

  f = calloc(1, sizeof(*f));
  if (ERROR(f == NULL))
    goto done;

  f->path = strdup(path);
  if (ERROR(f->path == NULL))
    goto done;

  ret = f;
  f = NULL;

done:
  fd_free(f);

  return ret;
}

/// deallocate a file descriptor table
static void fds_free(fds_t *fd) {
  if (fd == NULL)
    return;

  LIST_FREE(&fd->fds, fd_free);
  free(fd);
}

fds_t *fds_new(void) {

  fds_t *f = NULL;
  fds_t *ret = NULL;

  f = calloc(1, sizeof(*f));
  if (ERROR(f == NULL))
    goto done;

  f->ref_count = 1;

  ret = f;
  f = NULL;

done:
  fds_free(f);

  return ret;
}

fds_t *fds_acquire(fds_t *fd) {
  assert(fd != NULL);

  ++fd->ref_count;

  return fd;
}

fds_t *fds_release(fds_t *fd) {

  if (fd == NULL)
    return NULL;

  assert(fd->ref_count > 0);

  --fd->ref_count;
  if (fd->ref_count == 0)
    fds_free(fd);

  return NULL;
}

int fd_open(fds_t *table, int fd, const char *path) {
  assert(table != NULL);
  assert(fd >= 0);
  assert(path != NULL);

  int rc = 0;

  // do we need to enlarge the file descriptor table?
  while ((size_t)fd >= LIST_SIZE(&table->fds)) {
    if (ERROR((rc = LIST_PUSH_BACK(&table->fds, NULL))))
      goto done;
  }

  // close any previous entry
  fd_close(table, fd);

  *LIST_AT(&table->fds, (size_t)fd) = fd_new(path);
  if (ERROR(*LIST_AT(&table->fds, (size_t)fd) == NULL)) {
    rc = ENOMEM;
    goto done;
  }

done:
  return rc;
}

fd_t *fd_at(fds_t *table, int fd) {
  assert(table != NULL);

  if (fd < 0)
    return NULL;

  if ((size_t)fd >= LIST_SIZE(&table->fds))
    return NULL;

  return *LIST_AT(&table->fds, (size_t)fd);
}

void fd_close(fds_t *table, int fd) {
  assert(table != NULL);
  assert(fd >= 0);

  if ((size_t)fd >= LIST_SIZE(&table->fds))
    return;

  fd_free(*LIST_AT(&table->fds, (size_t)fd));
  *LIST_AT(&table->fds, (size_t)fd) = NULL;
}
