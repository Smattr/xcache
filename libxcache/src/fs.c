#include "fs.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static void fs_free(fs_t *fs) {
  if (fs == NULL)
    return;
  free(fs->cwd);
  free(fs);
}

fs_t *fs_new(const char *cwd) {
  assert(cwd != NULL);

  fs_t *f = NULL;
  fs_t *ret = NULL;

  f = calloc(1, sizeof(*f));
  if (ERROR(f == NULL))
    goto done;

  if (ERROR(fs_chdir(f, cwd) < 0))
    goto done;

  ret = f;
  f = NULL;

done:
  fs_free(f);

  return ret;
}

fs_t *fs_acquire(fs_t *fs) {
  assert(fs != NULL);

  ++fs->ref_count;

  return fs;
}

fs_t *fs_release(fs_t *fs) {
  assert(fs != NULL);
  assert(fs->ref_count > 0);

  --fs->ref_count;
  if (fs->ref_count == 0)
    fs_free(fs);

  return NULL;
}

int fs_chdir(fs_t *fs, const char *cwd) {
  assert(fs != NULL);
  assert(cwd != NULL);

  char *const copy = strdup(cwd);
  if (ERROR(copy == NULL))
    return ENOMEM;

  free(fs->cwd);
  fs->cwd = copy;

  return 0;
}
