#!/usr/bin/env bash
set -ex

if [ -d {{LOCAL_DEPLOY_RUN_DIR}} ]; then
    pushd {{LOCAL_DEPLOY_RUN_DIR}} > /dev/null
    docker-compose stop
    docker-compose down --remove-orphans --volumes
    popd > /dev/null
    rm -rf {{LOCAL_DEPLOY_RUN_DIR}}
fi

rm -rf {{LOCAL_DEPLOY_STAGE_DIR}}