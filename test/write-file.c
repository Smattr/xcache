/// @file
///
/// a program that writes content to a file

#include <stdio.h>
#include <stdlib.h>

int main(void) {

  FILE *f = fopen("foo", "w");
  if (f == NULL) {
    fprintf(stderr, "failed to open foo\n");
    return EXIT_FAILURE;
  }

  if (fprintf(f, "hello world") < 0) {
    fprintf(stderr, "failed to write content\n");
    return EXIT_FAILURE;
  }

  fclose(f);

  return EXIT_SUCCESS;
}
