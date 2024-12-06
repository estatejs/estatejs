#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"

rm -rf ./stage
mkdir -p ./stage/lib ./stage/bin

cp ../native/build/bin/{{ESTATE_RIVER_DAEMON}} ./stage/bin/

#TODO: only copy the deps that River actually needs
cp ../native/build/lib/* ./stage/lib/

cp river.Dockerfile ./stage/Dockerfile

pushd ./stage > /dev/null

docker build -t "{{ESTATE_RIVER_DOCKER_IMAGE}}" .