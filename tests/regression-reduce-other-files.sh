#!/bin/bash

# This checks for a previous problem where the cache had a default size limit
# and an implicit assumption that xcache knew of all the files in the cache.
# This caused xcache to think it could not possibly fail to reduce the cache
# size below the limit.
#
# Fixed in c1cdf2f.

set -e

CACHE=$(mktemp -d)

# Create a 20MB untracked file in the cache directory
head -c 20971520 /dev/zero >${CACHE}/garbage
xcache --cache-dir ${CACHE} -v -v -v echo hello
