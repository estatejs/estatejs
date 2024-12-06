#!/usr/bin/env bash
set -ex
kubectl apply -k .
k_wait_deployment accountsdb-deployment 300s