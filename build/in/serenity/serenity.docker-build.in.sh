#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"

rm -rf ./stage
mkdir -p ./stage/lib ./stage/bin

# TODO: only copy the deps that Serenity actually needs
cp ../native/build/lib/* ./stage/lib/
cp ../native/build/bin/{{ESTATE_SERENITY_DAEMON}} ./stage/bin/
cp ./healthy.sh ./stage/bin/

cp serenity.Dockerfile ./stage/Dockerfile

pushd ./stage > /dev/null

docker build -t "{{ESTATE_SERENITY_DOCKER_IMAGE}}" .