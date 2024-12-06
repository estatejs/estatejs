#!/usr/bin/env bash
set -ex

pushd {{LOCAL_DEPLOY_RUN_DIR}}

echo "Starting Jayne..."
docker-compose start jayne

echo "Connecting to Jayne..."
until curl -f http://localhost:{{ESTATE_JAYNE_LISTEN_PORT}}
do
    echo "Waiting for Jayne to come online..."
    sleep 1
done

echo "Cleaning development certificates on Jayne..."
docker-compose exec -T jayne /usr/bin/dotnet dev-certs https --clean

echo "Trusting development certificates on Jayne..."
docker-compose exec -T jayne /usr/bin/dotnet dev-certs https --trust