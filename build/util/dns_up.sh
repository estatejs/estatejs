#!/usr/bin/env bash
set -e
cd "$(dirname "$0")" || exit 1
export VALID_BUILD_TARGETS="production test"
source ./.util-preamble.sh

if [ -d "/mnt/c" ]; then
  echo "Since you're using WSL you'll need to manually flush the DNS cache from Windows."
  echo "1. Press Win+R, type cmd and press enter"
  echo "2. In the command prompt window, type ipconfig /flushdns and press enter"
  read -p "Press enter to continue when this has been completed."
else
  echo -n "Flushing DNS cache: "
  sudo systemd-resolve --flush-caches
  success "Ok"
fi

function check_resolved {
  local HOSTNAME=$1
  local EXPECTED_IP=$2
  echo -n "Checking ${HOSTNAME}: "
  if ! IP=$(ping -c 1 "${HOSTNAME}" | head -n-5 | grep -E -o "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"); then
    error "Not resolved"
    exit 1
  fi
  if [ "${IP}" != "${EXPECTED_IP}" ]; then
    error "Incorrect (${IP} != ${EXPECTED_IP})"
    exit 1
  fi
  success "Ok"
}

check_resolved "${ESTATE_JAYNE_FQDN}" "${ESTATE_JAYNE_IP}"
check_resolved "${ESTATE_RIVER_FQDN}" "${ESTATE_RIVER_IP}"
