#!/usr/env/bin bash
set -ex

if [ -d {{LOCAL_DEPLOY_RUN_DIR}} ]; then
    error "already running"
    exit 1
fi

if [ ! -d {{LOCAL_DEPLOY_STAGE_DIR}} ]; then
    error "nothing staged for local deployment"
    exit 1
fi

mkdir -p {{LOCAL_DEPLOY_RUN_DIR}}

cp -rv {{LOCAL_DEPLOY_STAGE_DIR}}/config {{LOCAL_DEPLOY_RUN_DIR}}/
cp -rv {{LOCAL_DEPLOY_STAGE_DIR}}/secrets {{LOCAL_DEPLOY_RUN_DIR}}/
cp deploy-local.docker-compose.yml {{LOCAL_DEPLOY_RUN_DIR}}/docker-compose.yml

for i in {{LOCAL_DEPLOY_STAGE_DIR}}/*.ymlpart; do
    cat ${i} >> {{LOCAL_DEPLOY_RUN_DIR}}/docker-compose.yml
done

pushd {{LOCAL_DEPLOY_RUN_DIR}} > /dev/null

docker-compose up --no-start