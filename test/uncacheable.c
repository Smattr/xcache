/// @file
/// @brief a program that does something we cannot record

#include <sys/random.h>

int main(void) {
  char buffer[10];
  (void)getrandom(buffer, sizeof(buffer), 0);
  return 0;
}
