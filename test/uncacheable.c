/// @file
/// @brief a program that does something we cannot record

#include <sys/syscall.h>
#include <unistd.h>

int main(void) {
  char buffer[10];
  (void)syscall(SYS_getrandom, buffer, sizeof(buffer), 0);
  return 0;
}
