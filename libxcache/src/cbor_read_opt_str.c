#include "cbor.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>

int cbor_read_opt_str(FILE *stream, char **value) {

  assert(stream != NULL);
  assert(value != NULL);

  *value = NULL;
  int rc = 0;

  // peek the stream to see if we have a null
  {
    int c = getc(stream);
    if (ERROR(c < 0)) {
      rc = EIO;
      goto done;
    }
    if (c == 0xf6) // null
      goto done;
    if (ERROR(ungetc(c, stream) == EOF)) {
      rc = EIO;
      goto done;
    }
  }

  if (ERROR((rc = cbor_read_str(stream, value))))
    goto done;

done:
  return rc;
}
