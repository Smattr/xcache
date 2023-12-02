#include "call.h"
#include "../../common/proccall.h"
#include <assert.h>
#include <sys/ioctl.h>

void call(unsigned long callno, const char *arg) {

  // message the tracer
  int rc __attribute__((unused)) = ioctl(XCACHE_FILENO, callno, arg);

  // We expect `ioctl` to fail because we called it on a pipe. We were not
  // calling it for its actual effects but rather the side effect of our parent
  // (the tracer) seeing the call and acting on it.
  assert(rc != 0);
}
