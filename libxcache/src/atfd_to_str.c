#include "debug.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

char *atfd_to_str(int fd) {

  char *buffer = NULL;
  size_t buffer_size = 0;
  FILE *stream = NULL;
  char *ret = NULL;

  stream = open_memstream(&buffer, &buffer_size);
  if (stream == NULL)
    goto done;

  if (fd == AT_FDCWD) {
    if (fputs("AT_FDCWD", stream) < 0)
      goto done;
  } else {
    if (fprintf(stream, "%d", fd) < 0)
      goto done;
  }

  fclose(stream);
  stream = NULL;
  ret = buffer;
  buffer = NULL;

done:
  if (stream != NULL)
    (void)fclose(stream);
  free(buffer);

  return ret;
}
