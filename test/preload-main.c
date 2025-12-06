/// @file
/// @brief Call a function that may be interposed on
///
/// Xcache uses `$LD_PRELOAD` to inject its spy into the tracee. However it is
/// possible the user is also using `$LD_PRELOAD` to inject something else into
/// the tracee (and, inadvertently, xcache itself). This preload of the users
/// should be retained even when xcache uses `$LD_PRELOAD`.
///
/// This program calls an externally provided function that we expect to be
/// overridden by something `$LD_PRELOAD`ed (preload.c). The test for this,
/// test.py::test_previous_ld_preload, involves setting `$LD_PRELOAD` at the top
/// level. So the function we override must also be something that xcache itself
/// is not using.

#include <math.h>

int main(int argc, __attribute__((unused)) char **argv) {
  return (int)(cos(argc) * 10);
}
