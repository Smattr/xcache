#!/bin/bash -e

CACHE=$(mktemp -d)
SCRATCH=$(mktemp -d)

cd ${SCRATCH}
cat - >Main.hs <<EOT
main = putStrLn "hello world"
EOT

timeout 5s xcache --cache-dir ${CACHE} -v -v -v ghc Main.hs
