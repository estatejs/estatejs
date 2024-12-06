#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"

rm -rf ./stage
mkdir -p ./stage/bin

cp -rfv build/* stage/bin/
cp doc-site.nginx.conf ./stage/nginx.conf
cp doc-site.Dockerfile ./stage/Dockerfile

pushd ./stage
docker build -t {{ESTATE_DOCSITE_DOCKER_IMAGE}} .