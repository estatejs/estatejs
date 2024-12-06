#!/usr/bin/env bash
set -ex
cd "$(dirname "$0")"

cd {{ESTATE_FRAMEWORK_DIR}}/client
pnpm i > /dev/null
./build.sh {{BUILD_TARGET}} {{BUILD_TYPE_L}}

cd {{RENDER_AREA_DIR}}
rm -rf ./build
mkdir ./build
cp -rfv {{ESTATE_FRAMEWORK_DIR}}/client/dist/* ./build/