#!/bin/bash

set -e
TEMP=$(mktemp -d)
trap "rm -rf $TEMP" EXIT
cd "$TEMP"

# Downloading
curl -sL https://github.com/datalek/boil/releases/latest/download/boil-node-22.tgz -o archive.tgz

# Extracting
tar -xzf archive.tgz
npx node dist/boil.cjs "$@"
