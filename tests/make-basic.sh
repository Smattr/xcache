#!/bin/bash -e

# Test we can cache a simple invocation of GNU Make.

CACHE=$(mktemp -d)
SCRATCH=$(mktemp -d)

cd ${SCRATCH}
cat - >Makefile <<EOT
default:
	echo hello world
EOT

# We should fail to cache it the first time around.
xcache --cache-dir ${CACHE} -v -v -v make 2>&1 | grep "Failed to locate cache entry"

xcache --cache-dir ${CACHE} -v -v -v make 2>&1 | grep "Found matching cache entry"
