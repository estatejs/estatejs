#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"

rm -rf stage
mkdir -p ./stage/bin
mkdir ./stage/lib

cp -rv ./build/bin/* ./stage/bin/

cp -fv ../native/build/client-lib/* ./stage/lib/

# TODO: only copy the deps that Serenity Client actually needs
cp -fv ../native/build/lib/* ./stage/lib/

cp jayne.Dockerfile ./stage/Dockerfile
cd stage

docker build -t "{{ESTATE_JAYNE_DOCKER_IMAGE}}" .