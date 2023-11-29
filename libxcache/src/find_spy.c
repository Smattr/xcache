#include "debug.h"
#include "find_me.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int find_spy(char **spy) {

  assert(spy != NULL);

  *spy = NULL;
  char *me = NULL;
  char *try = NULL;
  char *exe = NULL;
  int rc = 0;

  // attempt 1: via finding ourselves, which may work if we are an SO
  do {
    if (ERROR(find_lib(&me)))
      break;

    DEBUG("libxcache.so found at %s", me);

    const char libxcache_so[] = "libxcache.so";
    assert(strlen(me) > strlen(libxcache_so));

    // construct a path to where we think the spy lives
    if (ERROR(asprintf(&try, "%.*slibxcache-spy.so",
                       (int)(strlen(me) - strlen(libxcache_so)), me) < 0)) {
      rc = ENOMEM;
      goto done;
    }

    // did we find it?
    if (ERROR(access(try, F_OK) < 0))
      break;

    DEBUG("found libxcache-spy at %s", try);
    *spy = try;
    try = NULL;
    goto done;

  } while (0);

  free(try);
  try = NULL;

  if (ERROR((rc = find_exe(&exe))))
    goto done;

  DEBUG("containing program found at %s", exe);

  // attempt 2: via our containing program, which may work if we are installed
  do {
    const char suffix[] = "/bin/xcache";
    if (ERROR(strlen(exe) < strlen(suffix)))
      break;
    if (ERROR(strcmp(&exe[strlen(exe) - strlen(suffix)], suffix) != 0))
      break;

    if (ERROR(asprintf(&try, "%.*s/lib/libxcache-spy.so",
                       (int)(strlen(exe) - strlen(suffix)), exe) < 0)) {
      rc = ENOMEM;
      goto done;
    }

    // did we find it?
    if (ERROR(access(try, F_OK) < 0))
      break;

    DEBUG("found libxcache-spy at %s", try);
    *spy = try;
    try = NULL;
    goto done;

  } while (0);

  // attempt 3: via a build directory, which may work if we are not installed
  do {
    const char suffix[] = "/xcache/xcache";
    if (ERROR(strlen(exe) < strlen(suffix)))
      break;
    if (ERROR(strcmp(&exe[strlen(exe) - strlen(suffix)], suffix) != 0))
      break;

    if (ERROR(asprintf(&try, "%.*s/libxcache-spy/libxcache-spy.so",
                       (int)(strlen(exe) - strlen(suffix)), exe) < 0)) {
      rc = ENOMEM;
      goto done;
    }

    // did we find it?
    if (ERROR(access(try, F_OK) < 0))
      break;

    DEBUG("found libxcache-spy at %s", try);
    *spy = try;
    try = NULL;
    goto done;
  } while (0);

  // all our efforts failed
  rc = ENOENT;

done:
  free(exe);
  free(try);
  free(me);

  return rc;
}
