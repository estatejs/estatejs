#!/usr/bin/env bash
set -ex
echo "Running cleanup in test/setup (Tools test 2 of 2)"
pushd "{{ESTATE_PLATFORM_DIR}}/js/test/setup/scripts" > /dev/null
./cleanup.sh
popd > /dev/null