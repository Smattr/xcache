#include "../../common/proccall.h"
#include "call.h"
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

/// handle to libc’s `putenv`
static int (*real_putenv)(char *string);

/// `putenv` wrapper
int putenv(char *string) {

  // is this the first time we have been called?
  if (real_putenv == NULL) {
    real_putenv = dlsym(RTLD_NEXT, "putenv");
    if (real_putenv == NULL) {
      // it is unclear what to do here, but this seems best
      return ENOSYS;
    }
  }

  const int ret = real_putenv(string);

  const putenv_t payload = {.string = (uintptr_t)string, .ret = ret};
  call(CALL_PUTENV, &payload);

  // Ideally we would now have some way to learn of later modifications to
  // `string`, which the caller is technically allowed to do. But it is not
  // obvious how to reasonably do this.

  return ret;
}
