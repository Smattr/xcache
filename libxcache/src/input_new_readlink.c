#include "debug.h"
#include "hash_t.h"
#include "input_t.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/// `readlink`, but dynamically allocates
static int readlink_(const char *path, char **buf, size_t *bufsiz) {
  assert(path != NULL);
  assert(buf != NULL);
  assert(bufsiz != NULL);

  *buf = NULL;
  *bufsiz = 0;
  char *out = NULL;
  size_t size = BUFSIZ / 2;
  int rc = 0;

  while (true) {

    if (SIZE_MAX / 2 < size) {
      rc = EOVERFLOW;
      goto done;
    }

    // expand buffer
    {
      char *const o = realloc(out, size * 2);
      if (o == NULL) {
        rc = ENOMEM;
        goto done;
      }
      out = o;
      size *= 2;
    }

    // attempt to read link
    {
      const ssize_t written = readlink(path, out, size);
      if (written < 0) {
        rc = errno;
        goto done;
      }
      if ((size_t)written < size) {
        *bufsiz = (size_t)written;
        break;
      }
    }
  }

  *buf = out;
  out = NULL;

done:
  free(out);

  return rc;
}

int input_new_readlink(input_t *input, int expected_err, const char *path) {

  assert(input != NULL);
  assert(path != NULL);

  *input = (input_t){0};
  input_t i = {0};
  char *target = NULL;
  size_t target_size = 0;
  int rc = 0;

  i.tag = INP_READLINK;

  i.path = strdup(path);
  if (ERROR(i.path == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  i.err = readlink_(path, &target, &target_size);

  // if we saw a different error to the child, assume it did something
  // unsupported
  if (ERROR(i.err != expected_err)) {
    rc = ECHILD;
    goto done;
  }

  i.readlink.hash = hash_data(target, target_size);

  *input = i;
  i = (input_t){0};

done:
  free(target);
  input_free(i);

  return rc;
}
