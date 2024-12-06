#!/usr/bin/env bash
set -ex

pushd {{LOCAL_DEPLOY_RUN_DIR}}

echo "Starting RedisKeys..."
docker-compose start rediskeys

echo "Connecting to Redis..."
until ${UTIL_DIR}/redis-cli PING "pong"
do
    echo "Waiting for Redis to come online..."
    sleep 1
done