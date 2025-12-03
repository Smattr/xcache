#include "../../common/proccall.h"
#include "call.h"
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

/// handle to libc’s `unsetenv`
static int (*real_unsetenv)(const char *name);

/// `unsetenv` wrapper
int unsetenv(const char *name) {

  // is this the first time we have been called?
  if (real_unsetenv == NULL) {
    real_unsetenv = dlsym(RTLD_NEXT, "unsetenv");
    if (real_unsetenv == NULL) {
      // it is unclear what to do here, but this seems best
      return ENOSYS;
    }
  }

  const int ret = real_unsetenv(name);

  const unsetenv_t payload = {.name = (uintptr_t)name, .ret = ret};
  call(CALL_UNSETENV, &payload);

  return ret;
}
