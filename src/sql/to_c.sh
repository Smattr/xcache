#!/bin/bash

if [ $# -ne 3 ]; then
    echo "Usage: $0 input variable output" >&2
    exit 1
fi

{
    echo "const char *$2 ="
    sed 's/\(.*\)/\"\1\\n\"/g' "$1"
    echo ";"
} >"$3"
