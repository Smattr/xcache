/// \file
///
/// a program that makes an untraceable syscall
///
/// The following makes a system call, `socket`, that we know Xcache should not
/// handle. It should be unable to cache executions of this program and should
/// cleanly detach and bail out when seeing it. An Xcached execution of this
/// program should never hit in the cache and be replayable.

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

int main(void) {

  // make an (ignored) socket call, just to do something we know the tracer
  // cannot handle
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  (void)sockfd;

  // output something the user should see
  printf("hello world\n");

  return 0;
}
