#include "../../common/proccall.h"
#include "call.h"
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

/// handle to libc’s `setenv`
static int (*real_setenv)(const char *name, const char *value, int overwrite);

/// `setenv` wrapper
int setenv(const char *name, const char *value, int overwrite) {

  // is this the first time we have been called?
  if (real_setenv == NULL) {
    real_setenv = dlsym(RTLD_NEXT, "setenv");
    if (real_setenv == NULL) {
      // it is unclear what to do here, but this seems best
      return ENOSYS;
    }
  }

  const int ret = real_setenv(name, value, overwrite);

  const setenv_t payload = {
      .name = (uintptr_t)name, .overwrite = overwrite, .ret = ret};
  call(CALL_SETENV, &payload);

  return ret;
}
