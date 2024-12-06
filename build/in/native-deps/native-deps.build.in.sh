#!/usr/bin/env bash
set -ex

if [ ! -d {{ESTATE_NATIVE_DEPS_DIR}} ]; then
    FILE_NAME="/tmp/$(uuidgen).tgz"
    wget -q -O ${FILE_NAME} {{ESTATE_NATIVE_DEPS_URL}} > /dev/null
    mkdir -p {{ESTATE_NATIVE_DEPS_DIR}}
    tar zxvf ${FILE_NAME} -C {{ESTATE_NATIVE_DEPS_DIR}} > /dev/null
else
    echo '{{ESTATE_NATIVE_DEPS_DIR}} already present'
fi