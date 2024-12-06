#!/usr/bin/env bash
set -xe
rm -f {{ESTATE_FRAMEWORK_DIR}}/client/config.*.stamp
rm -f {{ESTATE_FRAMEWORK_DIR}}/client/src/internal/config.ts
rm -f {{ESTATE_FRAMEWORK_DIR}}/client/version.json
rm -f {{ESTATE_FRAMEWORK_DIR}}/client/package-config.json
cp {{RENDER_AREA_DIR}}/client.config.ts {{ESTATE_FRAMEWORK_DIR}}/client/src/internal/config.ts
cp {{RENDER_AREA_DIR}}/package-config.json {{ESTATE_FRAMEWORK_DIR}}/client/package-config.json
cp {{RENDER_DIR}}/rendered.stamp {{ESTATE_FRAMEWORK_DIR}}/client/config.{{BUILD_TARGET}}.{{BUILD_TYPE_L}}.stamp
cp {{RENDER_AREA_DIR}}/version.json {{ESTATE_FRAMEWORK_DIR}}/client/version.json