#include "debug.h"
#include "find_me.h"
#include <assert.h>
#include <errno.h>
#include <link.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/// state passed between `find_asan` and the callback, `is_asan`
typedef struct {
  char **path; ///< found absolute path to libasan
  int rc;      ///< result on completion
} state_t;

/// is the given header libasan?
///
/// @param info Information about this header from `dl_iterate_phdr`
/// @param size Byte size of `*info`
/// @param data State passed from `find_asan`
/// @return >0 if the header is libasan, <0 on error
static int is_asan(struct dl_phdr_info *info, size_t size, void *data) {
  assert(info != NULL);
  assert(data != NULL);

  state_t *const st = data;

  // fail if we seem to be running on a different platform than we expect
  if (size <
      offsetof(struct dl_phdr_info, dlpi_name) + sizeof(info->dlpi_name)) {
    st->rc = ENOSYS;
    return -1;
  }

  // only accept absolute paths
  const char *const name = info->dlpi_name;
  if (name[0] != '/')
    return 0;

  // extract the last path component
  const char *const last_slash = strrchr(name, '/');
  assert(last_slash != NULL);
  const char *const stem = last_slash + 1;

  // does it looks like “libasan.so*”?
  const char PREFIX[] = "libasan.so";
  if (strncmp(stem, PREFIX, strlen(PREFIX)) != 0)
    return 0;
  const char *trailer = stem + strlen(PREFIX);

  // the remainder should be a SONAME suffix, if anything
  if (*trailer == '.') {
    ++trailer;
  } else if (*trailer != '\0') {
    return 0;
  }
  while (*trailer >= '0' && *trailer <= '9')
    ++trailer;
  if (*trailer != '\0')
    return 0;

  // we found it
  *st->path = strdup(name);
  if (ERROR(*st->path == NULL)) {
    st->rc = ENOMEM;
    goto done;
  }
  st->rc = 0;

done:
  return 1;
}

int find_asan(char **asan) {
  assert(asan != NULL);

  *asan = NULL;
  state_t state = {.path = asan, .rc = ENOENT};

  (void)dl_iterate_phdr(is_asan, &state);

  return state.rc;
}
