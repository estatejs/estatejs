#!/usr/bin/env bash
set -ex
cd "$(dirname "$0")"

mkdir -p ./build/cmake

pushd ./build/cmake > /dev/null

cmake \
    -DCMAKE_BUILD_TYPE={{CPP_BUILD_TYPE}} \
    {{ESTATE_ROOT_DIR}}

cmake --build . --target all -- -j{{CORE_COUNT}}

popd > /dev/null

mkdir -p ./build/bin

# the Serenity daemon
cp -fv ./build/cmake/platform/native/daemon/serenity/{{ESTATE_SERENITY_DAEMON}} ./build/bin/

# the River daemon
cp -fv ./build/cmake/platform/native/daemon/river/{{ESTATE_RIVER_DAEMON}} ./build/bin/

# Contract & Unit tests
cp -fv ./build/cmake/platform/native/tests/tests ./build/tests

# Serenity client shared lib
mkdir -p ./build/client-lib
cp -fv ./build/cmake/platform/native/lib/serenity-client/libestate-serenity-client.so ./build/client-lib/

# Deps
# TODO: change this to have per-output runtime dependencies
mkdir -p ./build/lib
cp -fv {{ESTATE_NATIVE_DEPS_DIR}}/boost/lib/libboost_chrono.so ./build/lib/
cp -fv {{ESTATE_NATIVE_DEPS_DIR}}/boost/lib/libboost_timer.so ./build/lib/
cp -fv {{ESTATE_NATIVE_DEPS_DIR}}/boost/lib/libboost_thread.so ./build/lib/
cp -fv {{ESTATE_NATIVE_DEPS_DIR}}/boost/lib/libboost_system.so ./build/lib/
cp -fv {{ESTATE_NATIVE_DEPS_DIR}}/boost/lib/libboost_filesystem.so ./build/lib/
cp -fv {{ESTATE_NATIVE_DEPS_DIR}}/boost/lib/libboost_regex.so ./build/lib/
cp -fv {{ESTATE_NATIVE_DEPS_DIR}}/boost/lib/libboost_program_options.so ./build/lib/
cp -fv {{ESTATE_NATIVE_DEPS_DIR}}/rocksdb/cmake-build-release/librocksdb.so ./build/lib/
cp -fv {{ESTATE_NATIVE_DEPS_DIR}}/redis-plus-plus/cmake-build-release/libredis++.so ./build/lib/
cp -fv {{ESTATE_NATIVE_DEPS_DIR}}/hiredis/libhiredis.so ./build/lib/