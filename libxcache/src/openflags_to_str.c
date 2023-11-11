#include "debug.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

char *openflags_to_str(long flags) {

  char *buffer = NULL;
  size_t buffer_size = 0;
  FILE *stream = NULL;
  char *ret = NULL;

  stream = open_memstream(&buffer, &buffer_size);
  if (stream == NULL)
    goto done;

  if ((flags & O_RDWR) == O_RDWR) {
    if (fputs("O_RDWR", stream) < 0)
      goto done;
  } else if ((flags & O_WRONLY) == O_WRONLY) {
    if (fputs("O_WRONLY", stream) < 0)
      goto done;
  } else if ((flags & O_RDONLY) == O_RDONLY) {
    if (fputs("O_RDONLY", stream) < 0)
      goto done;
  } else {
    if (fprintf(stream, "%ld", flags) < 0)
      goto done;
    fclose(stream);
    stream = NULL;
    ret = buffer;
    buffer = NULL;
    goto done;
  }

  long recognised = O_RDWR | O_WRONLY | O_RDONLY;

#define DO(flag)                                                               \
  do {                                                                         \
    if (flags & (flag)) {                                                      \
      if (fprintf(stream, "|%s", #flag) < 0) {                                 \
        goto done;                                                             \
      }                                                                        \
    }                                                                          \
    recognised |= (flag);                                                      \
  } while (0)

  DO(O_APPEND);
  DO(O_ASYNC);
  DO(O_CLOEXEC);
  DO(O_CREAT);
  DO(O_DIRECT);
  DO(O_DIRECTORY);
  DO(O_DSYNC);
  DO(O_EXCL);
  DO(O_LARGEFILE);
  DO(O_NOATIME);
  DO(O_NOCTTY);
  DO(O_NOFOLLOW);
  DO(O_NONBLOCK);
  DO(O_NDELAY);
  DO(O_PATH);
  DO(O_SYNC);
  DO(O_TMPFILE);
  DO(O_TRUNC);

#undef DO

  const long unrecognised = flags & ~recognised;
  if (unrecognised != 0) {
    if (fprintf(stream, "|%ld", unrecognised) < 0)
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
