#!/bin/bash

set -e
TEMP=$(mktemp -d)
trap "rm -rf $TEMP" EXIT
cd "$TEMP"

# Downloading
curl -sL https://github.com/datalek/boil/releases/latest/download/boil-node-22.tgz -o archive.tgz

# Extracting
tar -xzf archive.tgz
# Redirect stdin to tty for interactive input
npx node dist/boil.cjs "$@" < /dev/tty
