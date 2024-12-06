#!/usr/bin/env bash
set -ex
cd "$(dirname "$0")" || exit 1
source ./.util-preamble.sh

if [ -z "${ESTATE_KEYS_DIR}" ]; then
	error "Missing ESTATE_KEYS_DIR environment variable"
	exit 1
fi

gcloud auth activate-service-account --key-file="${ESTATE_KEYS_DIR}/${ESTATE_GCP_KEY_FILE}"
cat "${ESTATE_KEYS_DIR}/${ESTATE_GCP_KEY_FILE}" | docker login -u _json_key --password-stdin "https://${ESTATE_GCR_HOST}"