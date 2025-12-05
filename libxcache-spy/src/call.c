#include "call.h"
#include "../../common/proccall.h"
#include <assert.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <threads.h>

void call(unsigned long callno, const void *arg) {

  // message the tracer
  int rc __attribute__((unused)) = ioctl(XCACHE_FILENO, callno, arg);

  // We expect `ioctl` to fail because we called it on an invalid file
  // descriptor. We were not calling it for its actual effects but rather the
  // side effect of our parent (the tracer) seeing the call and acting on it.
  assert(rc != 0);
}

/// how many `call_off`s have we seen with no matching `call_on`?
static thread_local size_t call_offs;

void call_off(void) {
  if (call_offs == 0)
    call(CALL_OFF, NULL);
  ++call_offs;
}

void call_on(void) {
  assert(call_offs > 0);
  --call_offs;
  if (call_offs == 0)
    call(CALL_ON, NULL);
}

/// how many `call_rng_off`s have we seen with no matching `call_rng_on`?
static thread_local size_t call_rng_offs;

void call_rng_off(void) {
  if (call_rng_offs == 0)
    call(CALL_RNG_OFF, NULL);
  ++call_rng_offs;
}

void call_rng_on(void) {
  assert(call_rng_offs > 0);
  --call_rng_offs;
  if (call_rng_offs == 0)
    call(CALL_RNG_ON, NULL);
}
