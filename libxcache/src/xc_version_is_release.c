#include <stdbool.h>
#include <stddef.h>
#include <xcache/version.h>

bool xc_version_is_release(const char *version) {

  if (version == NULL)
    return false;

  // treat anything v\d+.\d+.\d+ as a release version

  if (*version != 'v')
    return false;
  ++version;

  // scan major component
  if (*version < '0' || *version > '9')
    return false;
  while (*version >= '0' && *version <= '9')
    ++version;

  if (*version != '.')
    return false;
  ++version;

  // scan minor component
  if (*version < '0' || *version > '9')
    return false;
  while (*version >= '0' && *version <= '9')
    ++version;

  if (*version != '.')
    return false;
  ++version;

  // scan patch component
  if (*version < '0' || *version > '9')
    return false;
  while (*version >= '0' && *version <= '9')
    ++version;

  if (*version != '\0')
    return false;

  return true;
}
