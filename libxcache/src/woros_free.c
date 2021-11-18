#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "woros.h"

void woros_free(woros_t *woros) {

  if (woros == NULL)
    return;

  for (size_t i = 0; i < woros->size; ++i) {
    for (size_t j = 0; j < sizeof(woros->base[i].fds); ++j) {
      if (woros->base[i].fds[j] > 0)
        (void)close(woros->base[i].fds[j]);
    }
  }

  free(woros->base);

  memset(woros, 0, sizeof(*woros));
}
