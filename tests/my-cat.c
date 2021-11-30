/// \file
///
/// our own implementation of `cat`
///
/// We could use the system’s binary for testing, but we do not know its exact
/// implementation. That is, which version of `cat` is installed. By carrying
/// our own implementation, we know exactly what system calls it is making and
/// thus what xcache needs to replicate.

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  assert(argc > 1);
  for (int i = 1; i < argc; ++i) {

    FILE *in = fopen(argv[i], "r");
    if (in == NULL) {
      fprintf(stderr, "failed to open %s: %s\n", argv[i], strerror(errno));
      return EXIT_FAILURE;
    }

    while (true) {
      char buffer[BUFSIZ];
      size_t r = fread(buffer, sizeof(char), sizeof(buffer), in);

      (void)fwrite(buffer, sizeof(char), r, stdout);

      if (r < sizeof(buffer)) {
        if (feof(in))
          break;
        fprintf(stderr, "error reading %s\n", argv[i]);
        return EXIT_FAILURE;
      }
    }

    (void)fclose(in);
  }

  return EXIT_SUCCESS;
}
