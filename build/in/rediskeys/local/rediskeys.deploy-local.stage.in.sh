#!/usr/bin/env bash
set -ex

mkdir -p {{LOCAL_DEPLOY_STAGE_DIR}}

cp ./rediskeys.docker-compose.ymlpart {{LOCAL_DEPLOY_STAGE_DIR}}/