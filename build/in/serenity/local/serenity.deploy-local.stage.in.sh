#!/usr/bin/env bash
set -ex

mkdir -p {{LOCAL_DEPLOY_STAGE_DIR}}/config

cp serenity.launcher.config.json {{LOCAL_DEPLOY_STAGE_DIR}}/config/
cp serenity.worker-loader.config.json {{LOCAL_DEPLOY_STAGE_DIR}}/config/
cp serenity.worker-process.config.json {{LOCAL_DEPLOY_STAGE_DIR}}/config/
cp serenity.docker-compose.ymlpart {{LOCAL_DEPLOY_STAGE_DIR}}/
