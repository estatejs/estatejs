#!/usr/bin/env bash
set -ex
cd "$(dirname "$0")"
docker tag "{{ESTATE_RIVER_DOCKER_IMAGE}}" "{{ESTATE_GCR_HOST}}/{{ESTATE_GCP_PROJECT}}/{{ESTATE_RIVER_DOCKER_IMAGE}}"
docker push "{{ESTATE_GCR_HOST}}/{{ESTATE_GCP_PROJECT}}/{{ESTATE_RIVER_DOCKER_IMAGE}}"