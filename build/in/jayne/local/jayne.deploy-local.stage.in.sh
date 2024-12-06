#!/usr/bin/env bash
set -ex

mkdir -p {{LOCAL_DEPLOY_STAGE_DIR}}/config {{LOCAL_DEPLOY_STAGE_DIR}}/secrets

cp jayne.config.json {{LOCAL_DEPLOY_STAGE_DIR}}/config/
cp jayne.serenity-client.config.json {{LOCAL_DEPLOY_STAGE_DIR}}/config/
cp jayne.secrets.json {{LOCAL_DEPLOY_STAGE_DIR}}/secrets/
cp {{ESTATE_KEYS_DIR}}/{{ESTATE_FIREBASE_KEY_FILE}} {{LOCAL_DEPLOY_STAGE_DIR}}/secrets/jayne.{{ESTATE_FIREBASE_KEY_FILE}}
cp jayne.docker-compose.ymlpart {{LOCAL_DEPLOY_STAGE_DIR}}/