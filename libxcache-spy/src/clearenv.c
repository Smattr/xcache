#include "../../common/proccall.h"
#include "call.h"
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

/// handle to libc’s `clearenv`
static int (*real_clearenv)(void);

/// `clearenv` wrapper
int clearenv(void) {

  // is this the first time we have been called?
  if (real_clearenv == NULL) {
    real_clearenv = dlsym(RTLD_NEXT, "clearenv");
    if (real_clearenv == NULL) {
      // it is unclear what to do here, but this seems best
      return ENOSYS;
    }
  }

  const int ret = real_clearenv();

  call(CALL_CLEARENV, (void *)(intptr_t)ret);

  return ret;
}
