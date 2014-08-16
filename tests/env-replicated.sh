#!/bin/bash

# Test that we can correctly replicate `env`. This was an issue for a while.
# Fixed in 341e3e24ab79. Note that below we need to grep out the value of '_'
# which takes on the absolute path of the current executable.

CACHE=$(mktemp -d)
SCRATCH=$(mktemp -d)

env | grep -v '^_=' >${SCRATCH}/bare.txt
xcache --cache-dir ${CACHE} --no-getenv env | grep -v '^_=' >${SCRATCH}/uncached.txt
xcache --cache-dir ${CACHE} --no-getenv env | grep -v '^_=' >${SCRATCH}/cached.txt

diff ${SCRATCH}/bare.txt ${SCRATCH}/uncached.txt >/dev/null
if [ $? -ne 0 ]; then
    echo "env differs when being traced" >&2
    exit 1
fi

diff ${SCRATCH}/uncached.txt ${SCRATCH}/cached.txt >/dev/null
if [ $? -ne 0 ]; then
    echo "env differs when cached to when being traced" >&2
    exit 1
fi
