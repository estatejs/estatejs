#!/usr/bin/env bash
set -ex
kubectl apply -k .
k_wait_deployment river-deployment 300s
k_wait_ingress river-ingress 1800 "{{ESTATE_RIVER_IP}}"