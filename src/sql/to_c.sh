#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Usage: $0 output inputs..." >&2
    exit 1
fi

OUTPUT=$1
shift

{
    while [ -n "$1" ]; do
        echo "$1" | sed 's/^.*\/\(.*\)\.sql$/const char *query_\1 =/g'
        sed 's/\(.*\)/\"\1\\n\"/g' "$1"
        echo ";"
        shift
    done
} >"${OUTPUT}"
