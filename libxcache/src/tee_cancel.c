#include "tee_t.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

void tee_cancel(tee_t *tee) {

  if (tee == NULL)
    return;

  (void)tee_join(tee);

  if (tee->copy_path != NULL)
    (void)unlink(tee->copy_path);
  free(tee->copy_path);

  free(tee);
}
