/// @file
///
/// a program that uses stdout

#include <stdio.h>

int main(void) {
  // omit trailing newline to also check the tracer is not relying on flushing
  printf("hello\nworld");
  return 0;
}
