#!/usr/bin/env bash
set -ex

pushd {{LOCAL_DEPLOY_RUN_DIR}}

echo "Starting Doc-Site..."
docker-compose start doc-site

echo "Connecting to Doc-Site..."
until curl -f http://localhost:{{ESTATE_DOCSITE_LISTEN_PORT}}
do
    echo "Waiting for Doc-Site to come online..."
    sleep 1
done