#!/bin/bash -e

# Test for obvious invalid memory accesses.

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
valgrind --leak-check=yes xcache --cache-dir ${CACHE} -v -v -v gcc main.c 2>&1 | { ! grep "Invalid read of size"; }
