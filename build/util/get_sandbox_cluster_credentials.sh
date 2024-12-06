#!/usr/bin/env bash
set -e
cd "$(dirname "$0")" || exit 1
export VALID_BUILD_TARGETS="production test"
source ./.util-preamble.sh

if [ -z "${ESTATE_KEYS_DIR}" ]; then
	error "Missing ESTATE_KEYS_DIR environment variable"
	exit 1
fi

gcloud auth activate-service-account --key-file="${ESTATE_KEYS_DIR}/${ESTATE_GCP_KEY_FILE}"
gcloud container clusters get-credentials "${ESTATE_SANDBOX_CLUSTER_NAME}" --project "${ESTATE_GCP_PROJECT}" --zone "${ESTATE_GCP_ZONE}"