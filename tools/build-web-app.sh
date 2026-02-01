#!/bin/bash

set -eu

html_dir=resources/html

# Verify

if [ ! -d $html_dir ]; then
    echo "Error: $html_dir not found."
    exit 1
fi

# Build

build_dir=$(mktemp -d)

emcmake cmake -S . -B $build_dir -GNinja -DCMAKE_BUILD_TYPE=Release
cmake --build $build_dir

# Deploy

app_dir=$(mktemp -d)

cp $build_dir/apps/portable-glapd/{glapd-web.js,glapd-web.wasm} $app_dir
cp $html_dir/* $app_dir

echo "App is in $app_dir"