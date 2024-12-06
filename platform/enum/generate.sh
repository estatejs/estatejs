#!/usr/bin/env bash
set -xe

python3 -m pip install pystache

python3 ./enum_gen.py ./enums.env \
    ../native/lib/runtime/include/estate/runtime/code.h \
    "${ESTATE_FRAMEWORK_DIR}/client/src/internal/code.ts"
