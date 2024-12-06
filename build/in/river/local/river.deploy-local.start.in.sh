#!/usr/bin/env bash
set -ex

pushd {{LOCAL_DEPLOY_RUN_DIR}}

echo "Starting River..."
docker-compose start river

echo "Connecting to River..."
until curl -f http://localhost:{{ESTATE_RIVER_LISTEN_PORT}}
do
    echo "Waiting for River to come online..."
    sleep 1
done