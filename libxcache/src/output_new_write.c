#include "debug.h"
#include "output.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

int output_new_write(output_t *output, const char *path, mode_t mode,
                     bool is_creat_excl) {

  assert(output != NULL);
  assert(path != NULL);

  *output = (output_t){0};
  output_t o = {0};
  int rc = 0;

  o.tag = OUT_WRITE;

  o.path = strdup(path);
  if (ERROR(o.path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  o.write.mode = mode;
  o.write.is_creat_excl = is_creat_excl;

  *output = o;
  o = (output_t){0};

done:
  output_free(o);

  return rc;
}
