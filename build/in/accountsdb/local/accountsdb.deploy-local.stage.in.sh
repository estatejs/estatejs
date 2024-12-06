#!/usr/bin/env bash
set -ex

mkdir -p {{LOCAL_DEPLOY_STAGE_DIR}}/
cp ./accountsdb.docker-compose.ymlpart {{LOCAL_DEPLOY_STAGE_DIR}}/