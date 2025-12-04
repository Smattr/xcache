/// @file
/// @brief Provoke insertion of 10 entries into `getenv`-looked-up set
///
/// See test.py::test_bug_set_size

#include <stdio.h>
#include <stdlib.h>

int main(void) {
  for (size_t i = 0; i < 10; ++i) {
    const char name[] = {'F', 'O', 'O', '0' + (char)i, '\0'};
    const char *const value = getenv(name);
    printf("$%s = %s\n", name, value == NULL ? "<NULL>" : value);
  }
  return 0;
}
