#!/bin/bash -e

# Basic test of caching `echo`.

CACHE=$(mktemp -d)

xcache --cache-dir ${CACHE} -v -v -v echo "hello world" 2>&1 | grep "Failed to locate cache entry"
xcache --cache-dir ${CACHE} -v -v -v echo "hello world" 2>&1 | grep "Found matching cache entry"
