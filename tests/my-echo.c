/// \file
///
/// our own implementation of `echo`
///
/// We could use the system’s binary for testing, but we do not know its exact
/// implementation. That is, which version of `echo` is installed. By carrying
/// our own implementation, we know exactly what system calls it is making and
/// thus what xcache needs to replicate.

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  const char *sep = "";
  for (int i = 1; i < argc; ++i) {
    printf("%s%s", sep, argv[i]);
    sep = " ";
  }
  printf("\n");
  return EXIT_SUCCESS;
}
