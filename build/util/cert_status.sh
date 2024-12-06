#!/usr/bin/env bash
set -e
cd "$(dirname "$0")" || exit 1
export VALID_BUILD_TARGETS="production test"
source ./.util-preamble.sh

if ! bash ./get_sandbox_cluster_credentials.sh "${BUILD_TARGET}" "${BUILD_TYPE_L}" > /dev/null 2>&1; then
  error "Failed to set sandbox cluster credentials"
  exit 1
fi

echo -n "River (${ESTATE_RIVER_SERVICE_URL}): "
RIVER_STATUS=$(kubectl describe managedcertificate jayne-managedcertificate | perl -ne 'print /Certificate Status:\s+(\w+)/')
if [ "${RIVER_STATUS}" == "Active" ]; then
  success "Active"
else
  warning "${RIVER_STATUS}"
fi

echo -n "Jayne (${ESTATE_JAYNE_SERVICE_URL}): "
JAYNE_STATUS=$(kubectl describe managedcertificate river-managedcertificate | perl -ne 'print /Certificate Status:\s+(\w+)/')
if [ "${JAYNE_STATUS}" == "Active" ]; then
  success "Active"
else
  warning "${JAYNE_STATUS}"
fi