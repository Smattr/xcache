/// @file
///
/// a program that uses stderr

#include <stdio.h>

int main(void) {
  // omit trailing newline to also check the tracer is not relying on flushing
  fprintf(stderr, "hello\nworld");
  return 0;
}
