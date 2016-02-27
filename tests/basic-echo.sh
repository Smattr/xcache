#!/bin/bash -e

# Try to trigger an issue that used to happen where the Debug build of xcache
# would core dump when running in its own directory retrieving a cached entry
# for anything. The underlying problem was that one of the columns of the 'env'
# database table was expected to be `SQLITE_TEXT` and was reporting as
# `SQLITE_NULL`.

cd $(dirname $(which xcache))

xcache --cache-dir $(pwd) echo "hello world"
xcache --cache-dir $(pwd) echo "hello world"
