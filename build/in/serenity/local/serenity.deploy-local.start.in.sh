#!/usr/bin/env bash
set -ex

pushd {{LOCAL_DEPLOY_RUN_DIR}}

echo "Starting Serenity..."
docker-compose start serenity

echo "Connecting to Serenity..."
until docker-compose exec -T serenity bash /serenity/healthy.sh
do
    echo "Waiting for Serenity to come online..."
    sleep 1
done