#!/usr/bin/env bash
set -e
cd "$(dirname "$0")" || exit 1
export VALID_BUILD_TARGETS="production test"
source ./.util-preamble.sh

IP_FILE="../in/ip.${BUILD_TARGET}.conf"

if [ -f ${IP_FILE} ]; then
  REUSED_IP_ADDRESSES=true

  echo "NOTE: Re-using IP addresses found in file ${IP_FILE}."
  echo "NOTE: To recreate them you must manually delete the above file as well as the GCP resources in the console under 'VPC network > External IP Addresses'"

  source "${IP_FILE}"

  echo -n "Reading static IP address for Jayne: "
  if [ -z ${ESTATE_JAYNE_IP} ]; then
    error "Jayne IP was empty"
    exit 1
  fi
  JAYNE_IP="${ESTATE_JAYNE_IP}"
  success "Ok (${JAYNE_IP})"

  echo -n "Reading static IP address for River: "
  if [ -z ${ESTATE_RIVER_IP} ]; then
    error "River IP was empty"
    exit 1
  fi
  RIVER_IP="${ESTATE_RIVER_IP}"
  success "Ok (${RIVER_IP})"
else
  REUSED_IP_ADDRESSES=false

  echo -n "Creating static IP address for Jayne: "
  if ! bash gcloud compute addresses create jayne-ip --global --project "${ESTATE_GCP_PROJECT}" > "${RENDER_DIR}/create-ip-jayne.log" 2>&1; then
    error "Failed"
    error "====== create-ip-jayne.log ======"
    cat "${RENDER_DIR}/create-ip-jayne.log"
    exit 1
  fi
  JAYNE_IP=$(gcloud compute addresses describe jayne-ip --global --project "${ESTATE_GCP_PROJECT}" | grep -E -o "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)")
  success "Ok (${JAYNE_IP})"

  echo -n "Creating static IP address for River: "
  if ! bash gcloud compute addresses create river-ip --global --project "${ESTATE_GCP_PROJECT}" > "${RENDER_DIR}/create-ip-river.log" 2>&1; then
    error "Failed"
    error "====== create-ip-river.log ======"
    cat "${RENDER_DIR}/create-ip-river.log"
    exit 1
  fi
  RIVER_IP=$(gcloud compute addresses describe river-ip --global --project "${ESTATE_GCP_PROJECT}" | grep -E -o "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)")
  success "Ok (${RIVER_IP})"

  echo -n "Writing IP env file: "
  echo "###############################################################################" > "${IP_FILE}"
  echo "# Source file: ip.${BUILD_TARGET}.conf" >> "${IP_FILE}"
  echo "ESTATE_JAYNE_IP=\"${JAYNE_IP}\"" >> "${IP_FILE}"
  echo "ESTATE_RIVER_IP=\"${RIVER_IP}\"" >> "${IP_FILE}"
  success "Ok"
fi

echo -n "Creating cluster: "
if ! bash gcloud beta container --project "${ESTATE_GCP_PROJECT}" clusters create "${ESTATE_SANDBOX_CLUSTER_NAME}" \
--zone "${ESTATE_GCP_ZONE}" --no-enable-basic-auth --release-channel "regular" \
--machine-type "${ESTATE_SANDBOX_CLUSTER_MACHINE_TYPE}" --image-type "COS_CONTAINERD" --disk-type "pd-standard" --disk-size "${ESTATE_SANDBOX_CLUSTER_NODE_DISK_SIZE_GB}" \
--metadata disable-legacy-endpoints=true --scopes "https://www.googleapis.com/auth/devstorage.read_only",\
"https://www.googleapis.com/auth/logging.write","https://www.googleapis.com/auth/monitoring",\
"https://www.googleapis.com/auth/servicecontrol","https://www.googleapis.com/auth/service.management.readonly",\
"https://www.googleapis.com/auth/trace.append" --num-nodes "${ESTATE_SANDBOX_CLUSTER_NODE_COUNT}" --enable-stackdriver-kubernetes --enable-ip-alias \
--network "projects/${ESTATE_GCP_PROJECT}/global/networks/default" \
--subnetwork "projects/${ESTATE_GCP_PROJECT}/regions/${ESTATE_GCP_REGION}/subnetworks/default" \
--default-max-pods-per-node "110" --no-enable-master-authorized-networks \
--addons HorizontalPodAutoscaling,HttpLoadBalancing,GcePersistentDiskCsiDriver --enable-autoupgrade --enable-autorepair \
--max-surge-upgrade 1 --max-unavailable-upgrade 0 --enable-shielded-nodes --node-locations "${ESTATE_GCP_ZONE}" > "${RENDER_DIR}/create-cluster.log" 2>&1; then
  error "Failed"
  error "====== create-cluster.log ======"
  cat "${RENDER_DIR}/create-cluster.log"
  exit 1
fi
success "Ok"

echo -n "Removing outdated render directory: "
rm -rf "${RENDER_DIR}"
success "Ok"

echo "================================================================================"
echo "Next Steps:"
echo "1) Run ./render ${BUILD_TARGET} ${BUILD_TYPE_L} all"
echo "2) Run ./build ${BUILD_TARGET} ${BUILD_TYPE_L} all"
echo "3) Run ./deploy ${BUILD_TARGET} ${BUILD_TYPE_L} all"
if [ "${REUSED_IP_ADDRESSES}" == false ]; then
  echo "4) Since DNS for all environments is managed by the production GCP project, create or update the A records there to point to the newly allocated static IP addresses:"
  echo " ${ESTATE_JAYNE_FQDN} -> ${JAYNE_IP}"
  echo " ${ESTATE_RIVER_FQDN} -> ${RIVER_IP}"
  echo "5) Run util/dns_up.sh ${BUILD_TARGET} ${BUILD_TYPE_L} to verify DNS has propagated. This may take a while to succeed."
  echo "6) Run util/cert_status.sh ${BUILD_TARGET} ${BUILD_TYPE_L} until it returns 'Active.' This may also take a while to succeed."
  echo "7) Run util/test ${BUILD_TARGET} ${BUILD_TYPE_L} to run tests."
else
  echo "4) Run util/cert_status.sh ${BUILD_TARGET} ${BUILD_TYPE_L} until it returns 'Active.' This may also take a while to succeed."
  echo "5) Run util/test ${BUILD_TARGET} ${BUILD_TYPE_L} to run tests."
fi
