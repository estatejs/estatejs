#!/usr/bin/env bash
set -ex
kubectl apply -k .
k_wait_deployment jayne-deployment 300s
k_wait_ingress jayne-ingress 1800 "{{ESTATE_JAYNE_IP}}"