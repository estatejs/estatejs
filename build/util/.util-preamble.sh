#!/usr/bin/env bash
set -e

source "./.functions.sh"

if [ ! "${#}" == 2 ]; then
  error "Exactly 2 arguments required."
  if [ -z "${VALID_BUILD_TARGETS}" ]; then
    echo "Usage: ${0} <build_target> <lowercase build_type>"
  else
    echo "Usage: ${0} <$(join_by " | " ${VALID_BUILD_TARGETS})> <lowercase build_type>"
  fi
  exit 1
elif [ -n "${VALID_BUILD_TARGETS}" ] && ! list_includes "${VALID_BUILD_TARGETS}" "${1}"; then
  error "Invalid build target specified."
  echo "Usage: ${0} <$(join_by " | " ${VALID_BUILD_TARGETS})> <lowercase build_type>"
  exit 1
fi

BUILD_TARGET=$1
BUILD_TYPE_L=$2

if [ ! -f "../out/${BUILD_TARGET}.${BUILD_TYPE_L}/source.conf" ]; then
  error "Must render ${BUILD_TARGET} ${BUILD_TYPE_L} first"
  exit 1
fi

source "../out/${BUILD_TARGET}.${BUILD_TYPE_L}/source.conf"
export RENDER_DIR=$(realpath "../out/${BUILD_TARGET}.${BUILD_TYPE_L}")