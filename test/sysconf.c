/// @file
/// @brief Retrieve and print a `sysconf` value

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "%s: missing argument\n", argv[0]);
    return EXIT_FAILURE;
  }

  int name;
  if (strcmp(argv[1], "_SC_PAGESIZE") == 0) {
    name = _SC_PAGESIZE;
  } else {
    fprintf(stderr, "%s: unsupported name \"%s\"\n", argv[0], argv[1]);
    return EXIT_FAILURE;
  }

  const long value = sysconf(name);
  printf("%s: %s = %ld\n", argv[0], argv[1], value);

  return 0;
}
