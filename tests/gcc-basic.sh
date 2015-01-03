#!/bin/bash -e

# Test we can cache a simple invocation of GCC.

CACHE=$(mktemp -d)
SCRATCH=$(mktemp -d)

cd ${SCRATCH}
cat - >main.c <<EOT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    printf("hello world\n");
    return 0;
}
EOT

# We should fail to cache it the first time around.
xcache --cache-dir ${CACHE} --no-getenv -v -v -v gcc main.c 2>&1 | grep "Failed to locate cache entry"

xcache --cache-dir ${CACHE} --no-getenv -v -v -v gcc main.c 2>&1 | grep "Found matching cache entry"
