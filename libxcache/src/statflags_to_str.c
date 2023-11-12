#include "debug.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

char *statflags_to_str(long flags) {

  char *buffer = NULL;
  size_t buffer_size = 0;
  FILE *stream = NULL;
  char *ret = NULL;

  stream = open_memstream(&buffer, &buffer_size);
  if (stream == NULL)
    goto done;

  const char *separator = "";
  long recognised = 0;

#define DO(flag)                                                               \
  do {                                                                         \
    if (flags & (flag)) {                                                      \
      if (fprintf(stream, "%s%s", separator, #flag) < 0) {                     \
        goto done;                                                             \
      }                                                                        \
      separator = "|";                                                         \
    }                                                                          \
    recognised |= (flag);                                                      \
  } while (0)

  DO(AT_EMPTY_PATH);
  DO(AT_NO_AUTOMOUNT);
  DO(AT_SYMLINK_NOFOLLOW);

#undef DO

  const long unrecognised = flags & ~recognised;
  if (unrecognised != 0) {
    if (fprintf(stream, "%s%ld", separator, unrecognised) < 0)
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
