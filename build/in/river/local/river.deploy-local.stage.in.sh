#!/usr/bin/env bash
set -ex

mkdir -p {{LOCAL_DEPLOY_STAGE_DIR}}/config

cp river.config.json {{LOCAL_DEPLOY_STAGE_DIR}}/config/
cp river.docker-compose.ymlpart {{LOCAL_DEPLOY_STAGE_DIR}}/