#include "../../common/proccall.h"
#include "call.h"
#include <dlfcn.h>
#include <stdlib.h>

/// handle to libc’s `getenv`
static char *(*real_getenv)(const char *name);

/// `getenv` wrapper
char *getenv(const char *name) {

  // is this the first time we have been called?
  if (real_getenv == NULL) {
    real_getenv = dlsym(RTLD_NEXT, "getenv");
    if (real_getenv == NULL) {
      // it is unclear what to do here, but this seems best
      return NULL;
    }
  }

  // tell our tracer about this call
  call(CALL_GETENV, name);

  return real_getenv(name);
}
