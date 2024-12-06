#!/usr/bin/env bash
set -e
cd "$(dirname "$0")" || exit 1
source ./.util-preamble.sh

docker logout "https://${ESTATE_GCR_HOST}"