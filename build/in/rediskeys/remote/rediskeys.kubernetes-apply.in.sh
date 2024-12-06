#!/usr/bin/env bash
set -ex
kubectl apply -k .
k_wait_rollout statefulsets/rediskeys-statefuleset 300s