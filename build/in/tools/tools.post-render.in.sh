#!/usr/bin/env bash
set -xe
rm -f {{ESTATE_FRAMEWORK_DIR}}/tools/config.*.stamp
rm -f {{ESTATE_FRAMEWORK_DIR}}/tools/src/config.ts
rm -f {{ESTATE_FRAMEWORK_DIR}}/tools/src/service-notice.ts
rm -f {{ESTATE_FRAMEWORK_DIR}}/tools/version.json
rm -f {{ESTATE_FRAMEWORK_DIR}}/tools/package-config.json
cp {{RENDER_AREA_DIR}}/tools.config.ts {{ESTATE_FRAMEWORK_DIR}}/tools/src/config.ts
cp {{RENDER_AREA_DIR}}/package-config.json {{ESTATE_FRAMEWORK_DIR}}/tools/package-config.json
cp {{RENDER_AREA_DIR}}/tools.service-notice.ts {{ESTATE_FRAMEWORK_DIR}}/tools/src/service-notice.ts
cp {{RENDER_DIR}}/rendered.stamp {{ESTATE_FRAMEWORK_DIR}}/tools/config.{{BUILD_TARGET}}.{{BUILD_TYPE_L}}.stamp
cp {{RENDER_AREA_DIR}}/version.json {{ESTATE_FRAMEWORK_DIR}}/tools/version.json