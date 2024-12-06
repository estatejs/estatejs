#!/usr/bin/env bash
set -ex

rm -f "test.stamp"
rm -f "client/build/test.stamp"
rm -f "tools/build/test.stamp"

echo "Installing node modules in test/setup"
pushd "{{ESTATE_PLATFORM_DIR}}/js/test/setup" > /dev/null
pnpm i
pnpm link "{{ESTATE_FRAMEWORK_DIR}}/tools/dist"
popd > /dev/null

echo "Running setup in test/setup (Tools test 1 of 2)"
pushd "{{ESTATE_PLATFORM_DIR}}/js/test/setup/scripts" > /dev/null
./setup.sh
popd > /dev/null

echo "Installing node modules in test/project/client"
pushd "{{ESTATE_PLATFORM_DIR}}/js/test/project/client" > /dev/null
# NOTE: if I don't remove this folder pnpm will cache the generated client
# because in the test environment the worker version is always re-starting
# at 0 which causes the file-based package.json link
rm -rf node_modules
pnpm i
pnpm link "{{ESTATE_FRAMEWORK_DIR}}/client/dist"
popd > /dev/null
