#!/bin/bash

set -e
TEMP=$(mktemp -d)
trap "rm -rf $TEMP" EXIT

# Download and extract in temp (without changing working directory)
curl -sL https://github.com/datalek/boil/releases/latest/download/boil-node-22.tgz -o "$TEMP/archive.tgz"
# Extracting
tar -xzf "$TEMP/archive.tgz" -C "$TEMP"

# Redirect stdin to tty for interactive input
npx node "$TEMP/dist/boil.cjs" "$@" < /dev/tty
