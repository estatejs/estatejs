#!/usr/bin/env bash
set -xe
rm -f {{ESTATE_DOCSITE_DIR}}/config.*.stamp
rm -f {{ESTATE_DOCSITE_DIR}}/src/config.js
cp {{RENDER_AREA_DIR}}/doc-site.config.js {{ESTATE_DOCSITE_DIR}}/src/config.js
cp {{RENDER_DIR}}/rendered.stamp {{ESTATE_DOCSITE_DIR}}/config.{{BUILD_TARGET}}.{{BUILD_TYPE_L}}.stamp