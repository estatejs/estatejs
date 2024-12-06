#!/usr/bin/env bash
set -ex
cd "$(dirname "$0")"


cd {{ESTATE_DOCSITE_DIR}}
pnpm i > /dev/null

# TODO: Fix doc-site's warnings causing me to set CI=false
CI=false ./build.sh {{BUILD_TARGET}} {{BUILD_TYPE_L}}

cd {{RENDER_AREA_DIR}}
rm -rf ./build
mkdir ./build

cp -rfv {{ESTATE_DOCSITE_DIR}}/dist/* ./build/
cp -rfv {{ESTATE_DOCSITE_DIR}}/firebase.in.json ./build/firebase.json